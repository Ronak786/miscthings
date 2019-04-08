/*************************************************************************
	> File Name: hello.c
	> Author: davee-x
	> Mail: davee.naughty@gmail.com 
	> Created Time: Mon 08 Apr 2019 01:52:10 PM CST
 ************************************************************************/

#include <linux/spinlock.h>
#include <asm/pgtable.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/gfp.h>
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
static char *buf;

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

#if 0
static int mmap (struct file *file, struct vm_area_struct *vma) {
	pr_info("first time mmap here\n");
	buf =  (char*)__get_free_pages(GFP_KERNEL, 2);  //allocate 4 pages
	if (!buf) {
		pr_info("can not allocate\n");
		return -ENOMEM;
	}
	/*
	if (remap_pfn_range(vma, vma->vm_start, (virt_to_phys(buf) >> PAGE_SHIFT) + vma->vm_pgoff,
				vma->vm_end - vma->vm_start, vma->vm_page_prot)) {
		pr_info("mmap failed\n");
		return -EFAULT;
	}
	*/
	if (remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff,
				vma->vm_end - vma->vm_start, vma->vm_page_prot)) {
		pr_info("mmap failed\n");
		return -EFAULT;
	}
	pr_info("success maped\n");
	return 0;
}
#else
void vmopen(struct vm_area_struct * area) {
	pr_info("start open in vmopen\n");
}

void vmclose(struct vm_area_struct * area) {
	pr_info("start close vmclose\n");
}

int vmfault(struct vm_area_struct *vma, struct vm_fault *vmf) {
	pr_info("we are in fault %ld\n", (unsigned long)vmf->virtual_address - vma->vm_start + (vma->vm_pgoff << PAGE_SHIFT));
	struct page * page = virt_to_page(__get_free_page(GFP_KERNEL));
	if (!page) {
		pr_info("can not get page\n");
		return VM_FAULT_ERROR;
	}
	vmf->page = page;
	get_page(page);
	return 0;
}

static struct vm_operations_struct myops = {
	.open = vmopen,
	.close = vmclose,
	.fault = vmfault,
};

static int mmap (struct file *file, struct vm_area_struct *vma) {
	pr_info("start mmap has ops\n");
	/*
	if (remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff,
				vma->vm_end - vma->vm_start, vma->vm_page_prot)) {
		pr_info("mmap failed\n");
		return -EFAULT;
	}
	*/
	vma->vm_ops = &myops;
	vmopen(vma);
	return 0;
}
#endif

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = open,
	.release = release,
	.read = read,
	.write = write,
	.mmap = mmap,
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
	if (buf) 
		free_pages((unsigned long)buf, 2);
}

MODULE_LICENSE("GPL");
module_init(bstart);
module_exit(bend);
