/*                                                     
 * $Id: hello.c,v 1.5 2004/10/26 03:32:21 corbet Exp $ 
 */                                                    
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <asm/uaccess.h>   // copy_*_user funcs
#include <linux/slab.h>
#include <linux/sched.h>

MODULE_AUTHOR("muryliang");
MODULE_LICENSE("Dual BSD/GPL");

struct scull_qset {
	void **data;
	struct scull_qset *next;
};

struct scull_dev {
	struct scull_qset *data;  /* Pointer to first quantum set */
	int quantum;              /* the current quantum size */
	int qset;                 /* the current array size */
	unsigned long size;       /* amount of data stored here */
	unsigned int access_key;  /* used by sculluid and scullpriv */
	struct mutex mutex;     /* mutual exclusion semaphore     */
	struct cdev cdev;	  /* Char device structure		*/
};

static int scullmajor = 0; // default dynamic
static int scullminor = 0;
static int scullnr = 4;
static char *scullname = "scull";
static int scull_quantum = 4000;
static int scull_qset = 1000;
//static int num = sizeof(marr)/sizeof(int);
module_param(scullmajor, int, S_IRUGO);
module_param(scullminor, int, S_IRUGO);
module_param(scull_quantum, int, S_IRUGO);
module_param(scull_qset, int, S_IRUGO);
//module_param_array(marr, int, &num, S_IRUGO);

static struct scull_dev * scull_devices = NULL;

static void scull_cleanup_module(void);
static void scull_device_cleanup(struct scull_dev*);
static int scull_setup_cdev(struct scull_dev *dev, int idx);
static void scull_trim(struct scull_dev *);
struct scull_qset *scull_follow(struct scull_dev *dev, int n);

static loff_t scull_llseek (struct file * file, loff_t pos, int where) {
	struct scull_dev *dev = file->private_data;
	loff_t newpos;
	printk(KERN_ALERT "we are in %s\n", __FUNCTION__);

	switch(where) {
		case 0:
			newpos = pos;
			break;
		case 1:
			newpos = file->f_pos + pos;
			break;
		case 2:
			newpos = dev->size + pos;
			break;
		default:
			return -EINVAL;
	}
	if (newpos < 0) return -EINVAL;
	file->f_pos = newpos;
	return newpos;
}

static ssize_t scull_read (struct file * file, char __user * buf, size_t size, loff_t *pos) {
	struct scull_dev *dev = file->private_data;
	struct scull_qset *dptr;
	int quantum = dev->quantum, qset = dev->qset;
	int itemsize = quantum * qset;
	int item, rest, s_pos, q_pos;

	ssize_t retval = 0;

	if (mutex_lock_interruptible(&dev->mutex)) {
		printk(KERN_ALERT "can not lock mutex for pid %d\n", current->pid);
		return -ERESTARTSYS;
	}
	printk(KERN_ALERT "we are in %s\n", __FUNCTION__);
	// adjust read pos and size
	if (*pos >= dev->size) {
		printk(KERN_ALERT "pos >= size %ld\n", dev->size);
		goto out;
	}
	//this will limite size read after write
	if (*pos + size >= dev->size) {
		size = dev->size - *pos;
	}
	
	// calculate which item, which qset, which offset
	item = (long)*pos / itemsize;
	rest = (long)*pos % itemsize;
	s_pos = rest / quantum;
	q_pos = rest % quantum;
	dptr = scull_follow(dev, item);
	if (dptr == NULL || !dptr->data || !dptr->data[s_pos])
		goto out; // that piece of hole not filled

	// adjust to align one quantum
	if (size > quantum - q_pos)
		size = quantum - q_pos;

	if (copy_to_user(buf, dptr->data[s_pos] + q_pos, size)) {
		retval = -EFAULT;
		goto out;
	}
	*pos += size;
	retval = size;
	printk(KERN_ALERT "success read size %ld\n", size);

out:
	mutex_unlock(&dev->mutex);	
	return retval;
}

static ssize_t scull_write (struct file *file, const char __user *buf, size_t size, loff_t *pos) {
	struct scull_dev *dev = file->private_data;
	struct scull_qset *dptr;
	int quantum = dev->quantum, qset = dev->qset;
	int itemsize = quantum * qset;
	int item, rest, s_pos, q_pos;
	ssize_t retval = -ENOMEM;

	if (mutex_lock_interruptible(&dev->mutex)) {
		printk(KERN_ALERT "can not lock mutex in write\n");
		return -ERESTARTSYS;
	}

	printk(KERN_ALERT "we are in %s\n", __FUNCTION__);

	item = (long)*pos / itemsize;
	rest = (long)*pos % itemsize;
	s_pos = rest / quantum;
	q_pos = rest % quantum;

	dptr = scull_follow(dev, item);
	if (dptr == NULL)
		goto out;
	if (!dptr->data) {
		dptr->data = kmalloc(qset * sizeof(char*), GFP_KERNEL);
		if (!dptr->data) {
			printk(KERN_ALERT "can not alloc qsets\n");
			goto out;
		}
		memset(dptr->data, 0, qset * sizeof(char*));
	}
	if (!dptr->data[s_pos]) {
		dptr->data[s_pos] = kmalloc(quantum, GFP_KERNEL);
		if (!dptr->data[s_pos]) {
			printk(KERN_ALERT "can not alloc in a qset\n");
			goto out;
		}
	}

	if (size > quantum - q_pos)
		size = quantum - q_pos;

	if (copy_from_user(dptr->data[s_pos]+q_pos, buf, size)){
		retval = -EFAULT;
		goto out;
	}
	*pos += size;
	retval = size;

	// udpate dev size if needed
	if (dev->size < *pos)
		dev->size = *pos;

out:
	mutex_unlock(&dev->mutex);
	return retval;
}

static long scull_ioctl (struct file *file, unsigned int cmd, unsigned long arg) {
	printk(KERN_ALERT "we are in %s\n", __FUNCTION__);
	return 0;
}

static int scull_open (struct inode *inode, struct file *file) {
	struct scull_dev *dev;

	printk(KERN_ALERT "we are in %s\n", __FUNCTION__);
	dev = container_of(inode->i_cdev, struct scull_dev, cdev);
	file->private_data = dev;

	if ((file->f_flags & O_ACCMODE) == O_WRONLY || 
			(file->f_flags & O_TRUNC) == O_TRUNC) {
		scull_trim(dev);
	}
	return 0;
}

static int scull_release (struct inode *inode, struct file *file) {
	printk(KERN_ALERT "we are in %s\n", __FUNCTION__);
	return 0;
}

static struct file_operations scull_ops  = {
	.owner = THIS_MODULE,
	.llseek = scull_llseek,
	.read = scull_read,
	.write = scull_write,
	.unlocked_ioctl = scull_ioctl,
	.open = scull_open,
	.release = scull_release,
};

static __init int scull_init(void)
{
	int result;
	int i;
	dev_t dev;

	// alloc dev numbers
	if (scullmajor) {
		dev = MKDEV(scullmajor, scullminor);
		result = register_chrdev_region(dev, scullnr, scullname);
	} else {
		result = alloc_chrdev_region(&dev, scullminor, scullnr, scullname);
		scullmajor = MAJOR(dev);
	}

	if (result < 0) {
		printk(KERN_ALERT "scull: can't get major %d\n",
				scullmajor);
		return result;
	}
	printk(KERN_ALERT "scull: success register %d\n",
				scullmajor);

	// alloc mem for private data
	scull_devices = kmalloc(scullnr * sizeof(struct scull_dev) ,GFP_KERNEL);
	if (!scull_devices) {
		result = -ENOMEM;
		scull_cleanup_module();
		return result;
	}
	memset(scull_devices, 0, scullnr * sizeof(struct scull_dev));
	for (i = 0; i < scullnr; i++) {
		scull_devices[i].quantum = scull_quantum;
		scull_devices[i].qset = scull_qset;
		mutex_init(&scull_devices[i].mutex);
		if ((result = scull_setup_cdev(&scull_devices[i], i)) < 0) {
			printk(KERN_ALERT "error init scull devices %d\n", i);
			return result;
		}
	}

	printk(KERN_ALERT "success add cdev struct\n");
	return 0;
}

struct scull_qset *scull_follow(struct scull_dev *dev, int n) {
	struct scull_qset *qs = dev->data;

	if (!qs) {
		qs = dev->data = kmalloc(sizeof(struct scull_qset),
				GFP_KERNEL);
		if (qs == NULL)
			return NULL;
		memset(qs, 0, sizeof(struct scull_qset));
	}
	while (n--) {
		if (!qs->next) {
			qs->next = kmalloc(sizeof(struct scull_qset), GFP_KERNEL);
			if (qs->next == NULL)
				return NULL;
			memset(qs->next, 0, sizeof(struct scull_qset));
		}
		qs = qs->next;
		continue;
	}
	return qs;
}

// delete all data in scull device and free unneeded struct
void scull_trim(struct scull_dev *dev) {
	struct scull_qset *next, *dptr;
	int qset = dev->qset;
	int i;
	printk(KERN_ALERT "trim over\n");

	for (dptr = dev->data; dptr; dptr = next) {
		if (dptr->data) {
			for (i = 0; i < qset; i++) 
				kfree(dptr->data[i]);
			kfree(dptr->data);
		}
		next = dptr->next;
		kfree(dptr);
	}
	dev->size = 0;
	dev->quantum = scull_quantum;
	dev->qset = scull_qset;
	dev->data = NULL;
}

// set up cdev for each scull_dev
int scull_setup_cdev(struct scull_dev *dev, int idx) {
	int result;
	int devno = MKDEV(scullmajor, scullminor + idx);
	cdev_init(&dev->cdev, &scull_ops);
	result = cdev_add(&dev->cdev, devno, 1);
	if (result < 0) {
		scull_cleanup_module();
		printk(KERN_ALERT "init can not add cdev\n");
		return result;
	}
	return 0;
}

// clean the scull devices all in one func
void scull_device_cleanup(struct scull_dev *dev) {
	int i;
	for (i = 0; i < scullnr; i++) {
		cdev_del(&dev->cdev);
	}
	printk(KERN_ALERT "deleting cdev over");
}

void scull_cleanup_module(void) {
	dev_t devno;

	if (scull_devices) {
		scull_device_cleanup(scull_devices);
		kfree(scull_devices);
		scull_devices = NULL;
	}
	devno = MKDEV(scullmajor, scullminor);
	unregister_chrdev_region(devno, scullnr);
}

static __exit void scull_exit(void)
{
	scull_cleanup_module();
	printk(KERN_ALERT "Goodbye, cruel world\n");
}

module_init(scull_init);
module_exit(scull_exit);
