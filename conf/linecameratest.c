/*
 * DM9051 SPI Ethernet Driver (MAC + PHY)
 *
 * Copyright (C) axwdragon.com
 *  	  based on the code byBen Dooks <ben@simtec.co.uk>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/spi/spi.h>
#include <linux/string.h>
#include <linux/spinlock.h>
#include <linux/fs.h> 
#include <linux/file.h> 
#include <linux/mm.h> 
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/list.h>
#include <linux/wait.h>
#include <linux/delay.h>
#include <linux/atomic.h>

#define CAMNAME "linecam"
#define NUMBUFS 2
//#define READPERTIME 1
#define IOTYPE 'W'

#define LINECAMNONE_START _IO(IOTYPE,0x01)
#define LINECAMNONE_STOP  _IO(IOTYPE,0x02)
#define LINECAMGET_INDEX _IOR(IOTYPE,0x11, u32)
#define LINECAMPUT_INDEX _IOW(IOTYPE,0x21, u32)

/*
 * need to do:
 * readpertime as a param
 */

// add from first place, remove from head's prev place,
// so use  list_add, list_del
struct camnode {
	struct 	list_head list;
	int index; // start from 1
};

struct camlist {
	struct 	list_head list;
	spinlock_t camlock;
	wait_queue_head_t camqueue;
};

struct linecamdev
{
	struct cdev	*cdevptr;
	struct spi_device	*spidev;
	struct spi_message	spi_msg1;
	struct spi_transfer	spi_xfer1;
	struct camlist work_list;
	struct camlist free_list;
	u8 *sendbuf;
	u8 *mmapbuf;
};

static struct linecamdev *camdev;
static int read_lines = 1;
static int read_width = 1;
static int frame_head = 4;
static struct task_struct *ts = NULL;
static atomic_t runcatch = ATOMIC_INIT(0);
static struct camnode camnodes[NUMBUFS];
static wait_queue_head_t thread_closequeue;
static int after_stop = 0;
//static DECLARE_WAIT_QUEUE_HEAD(thread_closequeue);

static int mmap_thread(void *no_used) {
	struct camnode *tmpnode;
        struct spi_message      msg;
        struct spi_transfer     xfer;
	struct linecamdev *dev = camdev;
	int ret = 0;
	int iter;
	int bufferlen = read_lines * read_width + frame_head ;

	pr_info("start mmap\n");
	while (!kthread_should_stop()) {
//		pr_info("in should not stop\n");
		if (atomic_read(&runcatch)) { // begin catch
			spin_lock(&dev->free_list.camlock);
			if (list_empty(&dev->free_list.list)) {
	//			pr_info("start wait in mmap_thread\n");
				// if empty wait for completion for ready from mmap function
				spin_unlock(&dev->free_list.camlock);
				// unlock and wait may corrupt??

				wait_event_interruptible(dev->free_list.camqueue, !list_empty(&dev->free_list.list));

				spin_lock(&dev->free_list.camlock);   // if interrupted
				if (list_empty(&dev->free_list.list)) {
					pr_info("empty in freelist continue\n");
					spin_unlock(&dev->free_list.camlock);
					continue;
				}
				// have nodes, lock again
			}

			// retrieve from free_list a buffer
			tmpnode = list_entry(dev->free_list.list.prev, struct camnode, list);
			list_del(dev->free_list.list.prev);
			spin_unlock(&dev->free_list.camlock);
	//		pr_info("get node %d\n", tmpnode->index);

	//		memset(camdev->mmapbuf + bufferlen * (tmpnode->index-1), 0, bufferlen);
	//		ret = 0;
	//		for (iter = 0; iter < 1; iter++) {  // after modify one read once
				memset(&msg, 0, sizeof(msg));
				memset(&xfer, 0, sizeof(xfer));
				spi_message_init(&msg);
				spi_message_add_tail(&xfer, &msg);
				xfer.bits_per_word = 8;
				xfer.len = read_width * read_lines + frame_head; // whole len for dma
				xfer.tx_buf = camdev->sendbuf;
				xfer.rx_buf = camdev->mmapbuf + bufferlen * (tmpnode->index-1) + xfer.len * iter; // current pos

				ret = spi_sync(camdev->spidev, &msg);
				if (ret != 0) {
					break;
				} else {
	//				pr_info("success get iteration: %d\n", iter);
				}
	//		}
			// check for read error and format error
			if (ret != 0) { // if spi failed, add back to free_list
				pr_alert("can not spi_sync\n");
				spin_lock(&dev->free_list.camlock);
				// should not have problem here ??
				list_add(&tmpnode->list, &dev->free_list.list);
				spin_unlock(&dev->free_list.camlock);
			} else {
	//			pr_info("put node into work list start\n");
				// add into work list
				spin_lock(&dev->work_list.camlock);
				// should not have problem here ??
				list_add(&tmpnode->list, &dev->work_list.list);
				spin_unlock(&dev->work_list.camlock);
				wake_up_interruptible(&dev->work_list.camqueue);
			}
			mdelay(12);
		} else {
			set_current_state(TASK_INTERRUPTIBLE);
			schedule_timeout(2*HZ);
		}
	}
	pr_info("stop mmap\n");
	after_stop = 1;
	wake_up_interruptible(&thread_closequeue);
	return 0;
}

static void cam_init_list(struct linecamdev *camdev, int first) {
	int i;

	if (!first) {
		while(!list_empty(&camdev->work_list.list)) {
			list_del(camdev->work_list.list.prev);
		}
		while(!list_empty(&camdev->free_list.list)) {
			list_del(camdev->free_list.list.prev);
		}
	} else {
		INIT_LIST_HEAD(&camdev->work_list.list);
		INIT_LIST_HEAD(&camdev->free_list.list);
		spin_lock_init(&camdev->work_list.camlock);
		spin_lock_init(&camdev->free_list.camlock);
		init_waitqueue_head(&camdev->work_list.camqueue);
		init_waitqueue_head(&camdev->free_list.camqueue);
	}
	// modified here
	for (i = 0; i < NUMBUFS ; ++i) {
		camnodes[i].index = i+1;
		camnodes[i].list.prev = &camnodes[i].list;
		camnodes[i].list.next = &camnodes[i].list;
		list_add(&camnodes[i].list, &camdev->free_list.list);
	}
}

static int chr_release (struct inode * ind, struct file * fs) {
	pr_info("start release linecamera\n");
	fs->private_data = NULL;
	return 0;
}

static int chr_open (struct inode *ind, struct file *fs) {
	pr_info("start open linecamera\n");
	fs->private_data = camdev;
	return 0;
}

static ssize_t chr_read (struct file *fptr, char __user * userbuf, size_t siz, loff_t *nouse) {
	return 0;
}

static ssize_t chr_write (struct file *fptr, const char __user *userbuf, size_t siz, loff_t *nouse) {
	return 0;
}

static int read_idx(struct file *fptr) {
	int ret = -1;
	struct linecamdev *dev = (struct linecamdev*)fptr->private_data;
	struct camnode *tmpnode;

//	pr_info("start read\n");
	spin_lock(&dev->work_list.camlock);
	while (list_empty(&dev->work_list.list)) {
//		pr_info("start wait in chr_read\n");
		// if empty wait for completion for ready from mmap function
		spin_unlock(&dev->work_list.camlock);

		wait_event_interruptible(dev->work_list.camqueue, !list_empty(&dev->work_list.list));
		// have nodes, lock again
		spin_lock(&dev->work_list.camlock);
		if (list_empty(&dev->work_list.list)) {
//			pr_info("error list work empty!!\n");
			continue;
		}
	}

	tmpnode = list_entry(dev->work_list.list.prev, struct camnode, list);
	list_del(dev->work_list.list.prev);
	spin_unlock(&dev->work_list.camlock);

	ret = tmpnode->index;
//	pr_info("return node %d\n", ret);
	return ret;
}

static int chr_mmap(struct file *fptr, struct vm_area_struct *vma) {
	struct linecamdev *dev = (struct linecamdev*)fptr->private_data;
	unsigned long pfn_start = (virt_to_phys(dev->mmapbuf) >> PAGE_SHIFT) + vma->vm_pgoff;

	pr_info("mmap size is %lu\n", vma->vm_end - vma->vm_start);
	if (remap_pfn_range(vma, vma->vm_start, pfn_start,
		vma->vm_end - vma->vm_start,
		vma->vm_page_prot)) {
		pr_info("can not remap_pfn_range\n");
			return EAGAIN;
	}
	return 0;
}

static long chr_ioctl(struct file *fptr, unsigned int cmd, unsigned long arg) {
	struct linecamdev *dev = (struct linecamdev*)fptr->private_data;
	long ret = 0;
	int *argp = (int*)arg;
	u8 *ptr;
	int bufferlen = read_lines * read_width + frame_head;

	switch (cmd) {
	case LINECAMNONE_START:
		pr_alert("start catch\n");
		if (atomic_read(&runcatch) == 1) {
			atomic_set(&runcatch, 0);
			set_current_state(TASK_INTERRUPTIBLE);
			pr_alert("before timeout\n");
			schedule_timeout(HZ); // in case not stop normally
			pr_alert("after timeout\n");
		}
		cam_init_list(dev, 0); // reinit list to starter status
		atomic_set(&runcatch, 1);
		break;
	case LINECAMNONE_STOP:
		pr_alert("stop catch\n");
		atomic_set(&runcatch, 0);
		break;
	case LINECAMGET_INDEX:
		if (!argp) {
			ret = -EINVAL;
		} else {
			ret = read_idx(fptr);
			if (ret > 0) {
				*argp = ret;
				ret = 0;
			}
		}
		break;
	case LINECAMPUT_INDEX:
		spin_lock(&dev->free_list.camlock);
		// should not have problem here ??
		list_add(&camnodes[*argp-1].list, &dev->free_list.list);
		spin_unlock(&dev->free_list.camlock);
		wake_up_interruptible(&dev->free_list.camqueue);
		break;
	default:
		pr_info("unknown command %d\n", cmd);
		break;
	}
	return ret;
}

static struct file_operations camops = {
	.open = chr_open,
	.release = chr_release,
	.read = chr_read,
	.write = chr_write,
	.mmap = chr_mmap,
	.unlocked_ioctl = chr_ioctl,
};

/* driver bus management functions */
static int camdev_probe(struct spi_device *spi)
{
	struct cdev * chrdev;
//	struct device_node *np = spi->dev.of_node;
	dev_t devnum;
	int ret = 0;
	int i = 0;

	pr_info("linecamera: %s,line=%d, busnumber %d\n", __func__,__LINE__, spi->master->bus_num);
	//pr_info("camdev max_speed_hz=%d,node_name=%s\n",spi->max_speed_hz,spi->dev.of_node->name);

	// alloc private structure
	camdev = (struct linecamdev*)kzalloc(sizeof(struct linecamdev), GFP_KERNEL);
	if (!camdev) {
		pr_info("can not alloc memory for camdev structure\n");
		ret = -1;
		goto ERROR3;
	}

	init_waitqueue_head(&thread_closequeue);
    // set and add NUMBUFS lists into free_list, index is 1, 2
	// may have proble assign lock
	cam_init_list(camdev, 1);

	camdev->mmapbuf = kzalloc((read_lines * read_width + frame_head)  * NUMBUFS, GFP_KERNEL);
	if (!camdev->mmapbuf) {
		pr_info("can not mmap allocal buf\n");
		goto ERROR3;
	}

	// register a chardeivce
	if (alloc_chrdev_region(&devnum, 0, 1, CAMNAME) != 0) {
		pr_info("can not alloc char dev number for linecamera\n");
		ret = -1;
		goto ERROR2;
	}

	chrdev = cdev_alloc();
	if (!chrdev)
	{
		pr_info("failed to alloc linecamera character device\n");
		ret = -ENOMEM;
		goto ERROR1;
	}
	cdev_init(chrdev, &camops);
	if (cdev_add(chrdev, devnum, 1) != 0) {
		pr_info("can not add\n");
		ret = -ENOMEM;
		goto ERROR1;
	}

	spi->bits_per_word = 8;

	camdev->cdevptr = chrdev;
	camdev->spidev = spi;

	if (read_lines == 1 || read_width == 1) {
		pr_info("forget to set read_width or read_lines in param!!\n");
		ret = -EFAULT;
		goto ERROR0;
	}
	camdev->sendbuf = (u8*)kzalloc(read_width * read_lines + frame_head , GFP_KERNEL);
	if (!camdev->sendbuf) {
		pr_info("can not allocate kernel buffer for linecamerea send\n");
		ret = -EFAULT;
		goto ERROR0;
	}

	camdev->sendbuf[0] = 0xa1;
    
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,9,0)
	spi_set_drvdata(spi, camdev);
#else
	dev_set_drvdata(&spi->dev, camdev);
#endif

	// start kthread
	ts = kthread_run(mmap_thread, NULL, "fill_mmap");	
	if (ts == NULL) {
		pr_info("can not start thread\n");
		ret = -4;
		goto ERRORM1;
	}

	return 0;
ERRORM1:
	    kfree(camdev->sendbuf);
ERROR0:
	    cdev_del(camdev->cdevptr);
ERROR1:
	    unregister_chrdev_region(devnum, 1);
ERROR2:
	    kfree(camdev);
ERROR3:
	    return ret;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,8,0)
static int camdev_remove(struct spi_device *spi)
#else
static int __devexit camdev_remove(struct spi_device *spi)
#endif
{
	struct linecamdev *priv = dev_get_drvdata(&spi->dev);

	if (ts != NULL) {
		kthread_stop(ts);
		pr_info(" before stop ts\n");
//		interruptible_sleep_on(&thread_closequeue);
		wait_event_interruptible(thread_closequeue, after_stop == 1);
		pr_info(" after stop ts\n");
		ts = NULL;
	}
	unregister_chrdev_region(priv->cdevptr->dev, 1);
	cdev_del(priv->cdevptr);
	kfree(priv->sendbuf);
	kfree(priv->mmapbuf);
	kfree(priv);

	return 0;
}

static const struct of_device_id camdev_of_match[] = {
       { .compatible = "cynovo,linecamera", },
        {}
};
MODULE_DEVICE_TABLE(of, camdev_of_match);

static struct spi_driver camdev_driver =
{
	.driver =
	{
		.name = "linecamera",
		.owner = THIS_MODULE,
		.of_match_table = camdev_of_match,
	},
	.probe = camdev_probe,
	.remove = camdev_remove,
};

static int __init camdev_init(void)
{
	return spi_register_driver(&camdev_driver);
}

static void __exit camdev_exit(void)
{
	spi_unregister_driver(&camdev_driver);
}

module_init(camdev_init);
module_exit(camdev_exit);

module_param(read_lines, int, 0644);
module_param(read_width, int, 0644);
module_param(frame_head, int, 0644);
MODULE_PARM_DESC(read_lines, "how many lines will be read, also line width should be readlines");
MODULE_PARM_DESC(read_width, "how many pixel will be read one time, actually is width");
MODULE_PARM_DESC(frame_head, "how many bytes will be read as frame head");
MODULE_DESCRIPTION("LineCamera test driver");
MODULE_AUTHOR("cynovo");
MODULE_LICENSE("GPL");
