/*************************************************************************
	> File Name: pipe.c
	> Author: davee-x
	> Mail: davee.naughty@gmail.com 
	> Created Time: Thu 28 Mar 2019 01:38:56 PM CST
 ************************************************************************/

#include <linux/kernel.h>
#include <linux/moduleparam.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/seq_file.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/fcntl.h>

#include "scull.h"

struct scull_pipe {
	wait_queue_head_t inq, outq;
	char *buffer, *end;
	int buffersize;
	char *rp, *wp;
	int nreaders, nwriters;
	struct fasync_struct *async_queue;
	struct mutex mutex;
	struct cdev cdev;
};

static int scullnr = 3;
static int scull_p_buffer = 1000;
static dev_t scull_devno;
static int major;

static struct scull_pipe *scull_devices;

//static int scull_p_fasync(int fd, struct file *file, int mode);
static int spacefree(struct scull_pipe *dev);
static int scull_fasync(int fd, struct file *file, int mode);

static int scull_open(struct inode *inode, struct file *file) {
	struct scull_pipe *dev;
	dev = container_of(inode->i_cdev, struct scull_pipe, cdev);
	file->private_data = dev;

	if (mutex_lock_interruptible(&dev->mutex))
		return -ERESTARTSYS;
	if (! dev->buffer) {
		dev->buffer = kmalloc(scull_p_buffer, GFP_KERNEL);
		if (! dev->buffer){
			mutex_unlock(&dev->mutex);
			return -ENOMEM;
		}
	}
	dev->buffersize = scull_p_buffer;
	dev->end = dev->buffer + dev->buffersize;
	dev->rp = dev->wp = dev->buffer;
	/* use f_mode,not  f_flags: it's cleaner (fs/open.c tells why) */
	if (file->f_mode & FMODE_READ)
		dev->nreaders++;
	if (file->f_mode & FMODE_WRITE)
		dev->nwriters++;
	mutex_unlock(&dev->mutex);

	return nonseekable_open(inode, file);
}

static int scull_release(struct inode *inode, struct file *file) {
	struct scull_pipe *dev = file->private_data;

	scull_fasync(-1, file, 0);
	mutex_lock(&dev->mutex);
	if (file->f_mode & FMODE_READ)
		dev->nreaders--;
	if (file->f_mode & FMODE_WRITE)
		dev->nwriters--;
	if (dev->nreaders + dev->nwriters == 0) {
		kfree(dev->buffer);
		dev->buffer = NULL;
	}
	mutex_unlock(&dev->mutex);
	return 0;
}

// only read until end, not wrap in one read
static ssize_t scull_read(struct file *file, char __user *buf, size_t count, loff_t *pos) {
	struct scull_pipe *dev = file->private_data;

	if (mutex_lock_interruptible(&dev->mutex))
		return -ERESTARTSYS;

	// begin wait and check loop
	while (dev->rp == dev->wp) {
		mutex_unlock(&dev->mutex);
		if (file->f_flags & O_NONBLOCK)
			return -EAGAIN;
		PDEBUG("%s reading: going to sleep\n", current->comm);
		if (wait_event_interruptible(dev->inq, (dev->rp != dev->wp)))
				return -ERESTARTSYS;
		// check should be in mutex
		if (mutex_lock_interruptible(&dev->mutex))
			return -ERESTARTSYS;
	}

	if (dev->wp > dev->rp)
		count = min(count, (size_t)(dev->wp - dev->rp));
	else
		count = min(count, (size_t)(dev->end - dev->rp));
	if (copy_to_user(buf, dev->rp, count)) {
		mutex_unlock(&dev->mutex);
		return -EFAULT;
	}
	// do wrap if needed
	dev->rp += count;
	if (dev->rp == dev->end)
		dev->rp = dev->buffer;
	mutex_unlock(&dev->mutex);

	// wake up writer if has some one
	wake_up_interruptible(&dev->outq);
	PDEBUG("%s di read %li bytes\n", current->comm, count);
	return count;
}

// when rp == wp + 1: full
// when rp == wp: empty
static int spacefree(struct scull_pipe *dev) {
	if (dev->rp == dev->wp)
		return dev->buffersize - 1;
	return ((dev->rp + dev->buffersize - dev->wp) % dev->buffersize) -1;
}

static int scull_getwritespace(struct scull_pipe *dev, struct file *file) {
	while (spacefree(dev) == 0) { // full
		DEFINE_WAIT(wait);
		mutex_unlock(&dev->mutex);
		if (file->f_flags & O_NONBLOCK)
			return -EAGAIN;
		PDEBUG("%s writing going to sleep\n", current->comm);
		prepare_to_wait(&dev->outq, &wait, TASK_INTERRUPTIBLE);
		if (spacefree(dev) == 0)
			schedule();
		finish_wait(&dev->outq, &wait);
		if (signal_pending(current))
			return -ERESTARTSYS;
		if (mutex_lock_interruptible(&dev->mutex))
			return -ERESTARTSYS;
	}
	return 0;
}

// only write until end, not wrap in one write
static ssize_t scull_write(struct file *file, const char __user *buf, size_t count, loff_t *pos) {
	struct scull_pipe *dev = file->private_data;
	int result;

	if (mutex_lock_interruptible(&dev->mutex))
		return -ERESTARTSYS;

	result = scull_getwritespace(dev, file);
	if (result)
		return result;
	count = min(count, (size_t)spacefree(dev));
	if (dev->wp >= dev->rp)
		count = min(count, (size_t)(dev->end - dev->wp));
	else 
		count = min(count, (size_t)(dev->rp - dev->wp - 1));
	PDEBUG("going to accept %li bytes to %p from %p\n", (long)count, dev->wp, buf);
	if (copy_from_user(dev->wp, buf, count)) {
		mutex_unlock(&dev->mutex);
		return -EFAULT;
	}
	dev->wp += count;
	if (dev->wp == dev->end)
		dev->wp = dev->buffer;
	mutex_unlock(&dev->mutex);

	wake_up_interruptible(&dev->inq);

	if (dev->async_queue)
		kill_fasync(&dev->async_queue, SIGIO, POLL_IN);
	PDEBUG("%s did write %li bytes\n", current->comm, count);
	return count;

}

static unsigned int scull_poll(struct file *filp, poll_table *wait) {
	struct scull_pipe *dev = filp->private_data;
	unsigned int mask = 0;

	mutex_lock(&dev->mutex);
	poll_wait(filp, &dev->inq, wait);
	poll_wait(filp, &dev->outq, wait);
	if (dev->rp != dev->wp)
		mask |= POLLIN | POLLRDNORM; // not empty,can read
	if (spacefree(dev)) 
		mask |= POLLOUT | POLLWRNORM; // can write
	mutex_unlock(&dev->mutex);
	return mask;
}

int scull_fasync(int fd, struct file *file, int mode) {
	struct scull_pipe *dev = file->private_data;
	PDEBUG("the fd passin is %d\n", fd);
	return fasync_helper(fd, file, mode, &dev->async_queue);
}

struct file_operations scull_ops = {
	.owner = THIS_MODULE,
	.open = scull_open,
	.read = scull_read,
	.write = scull_write,
	.release = scull_release,
	.poll = scull_poll,
	.fasync = scull_fasync,
};

static __init int pipe_init(void) {
	int i;	
	int ret;
	int devno;

	// register char dev
	ret = alloc_chrdev_region(&scull_devno, 0, scullnr, "mypipe");
	if (ret) {
		PDEBUG("can not get devno dynamically\n");
		return -EFAULT;
	}

	scull_devices = kmalloc(scullnr * sizeof(struct scull_pipe), GFP_KERNEL);
	if (scull_devices == NULL) {
		PDEBUG("can not alloc device\n");
		return -ENOMEM;
	}
	memset(scull_devices, 0, scullnr * sizeof(struct scull_pipe));
	for (i = 0; i < scullnr; i++) {
		init_waitqueue_head(&scull_devices[i].inq);
		init_waitqueue_head(&scull_devices[i].outq);
		mutex_init(&scull_devices[i].mutex);
		cdev_init(&scull_devices[i].cdev, &scull_ops);
		scull_devices[i].cdev.owner = THIS_MODULE;
		devno = MKDEV(MAJOR(scull_devno), MINOR(scull_devno) + i);
		ret = cdev_add (&scull_devices[i].cdev, devno, 1);		
		if (ret) {
			PDEBUG("error add cdev\n");
		}
	}

	PDEBUG("begin init pipe major %d\n", MAJOR(scull_devno));
	return 0;
}

static __exit void pipe_exit(void) {
	int i;
	PDEBUG("begin end pipe\n");
	for (i = 0; i < scullnr; i++) {
		cdev_del(&scull_devices[i].cdev);
		kfree(scull_devices[i].buffer);
	}
	kfree(scull_devices);
	scull_devices = NULL;
	unregister_chrdev_region(scull_devno, scullnr);
}

module_init(pipe_init);
module_exit(pipe_exit);
