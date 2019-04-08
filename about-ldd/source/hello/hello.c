/*************************************************************************
	> File Name: hello.c
	> Author: davee-x
	> Mail: davee.naughty@gmail.com 
	> Created Time: Mon 08 Apr 2019 01:52:10 PM CST
 ************************************************************************/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kernel.h>
//#include <linux/kobject.h>

struct bd {
	int count;
	struct cdev cdev;
	struct kobject kobj;
};

static struct bd mbd;
static dev_t devno;
static char *name = "bhello";
static char *cname = "chelo";
static char *dname = "dhello";
static struct class *classhello;
static struct device *devicehello;

static ssize_t hello_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count) {
	return count;
}

static ssize_t hello_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
	return sprintf(buf, "hello show\n");
}

static struct kobj_attribute mattr = __ATTR(hello_value, 0600, hello_show, hello_store);

static int open(struct inode *inode, struct file *file) {
	pr_info("we are open\n");
	return 0;
}

static int release(struct inode *inode, struct file *file) {
	return 0;
}

static ssize_t read(struct file *file, char __user *buf, size_t siz, loff_t *off) {
	return -EFAULT;
}

static ssize_t write(struct file *file, const char __user *buf, size_t siz, loff_t *off) {
	return -EFAULT;
}

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = open,
	.release = release,
	.read = read,
	.write = write,
};

static int bstart(void) {
	pr_info("start\n");

	if (alloc_chrdev_region(&devno, 0, 1, name)) {
		pr_info("can not alloca chrdev retion\n");
		return -EFAULT;
	} else {
		pr_info("allocated %ld %ld\n", (long)MAJOR(devno), (long)MINOR(devno));
	}

	if ((classhello = class_create(THIS_MODULE, cname)) == NULL) {
		pr_info("can not create class %s\n", cname);
		unregister_chrdev_region(devno, 1);
		return -ENOMEM;
	}
	if ((devicehello = device_create(classhello, NULL, devno, NULL, name)) == NULL) {
		pr_info("can not create device %s\n", dname);
		class_destroy(classhello);
		unregister_chrdev_region(devno, 1);
	}

	cdev_init(&mbd.cdev, &fops);
	mbd.cdev.owner = THIS_MODULE;

	if (cdev_add(&mbd.cdev, devno, 1)) {
		pr_info("can not add cdev\n");
		device_destroy(classhello, devno);
		class_destroy(classhello);
		unregister_chrdev_region(devno, 1);
		return -EFAULT;
	}
	pr_info("before create attr\n");
	if (sysfs_create_file(&devicehello->kobj, &mattr.attr)) {
		pr_info("can not creat sysfs file\n");
	}
	return 0;
}

static void bend(void) {
	pr_info("end\n");
	device_destroy(classhello, devno);
	class_destroy(classhello);
	cdev_del(&mbd.cdev);
	unregister_chrdev_region(devno, 1);
}

MODULE_LICENSE("GPL");
module_init(bstart);
module_exit(bend);
