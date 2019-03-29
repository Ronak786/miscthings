/*************************************************************************
	> File Name: /home/sora/gitbase/miscthings/about-ldd/source/scull/timerproc.c
	> Author: davee-x
	> Mail: davee.naughty@gmail.com 
	> Created Time: Fri 29 Mar 2019 03:11:46 PM CST
 ************************************************************************/

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>

#define PDEBUG(fmt, args...) printk(KERN_DEBUG "scull: " fmt, ## args)

static int tshow(struct seq_file *m, void *v) {
	unsigned long jtime = jiffies;
	struct timespec spec = current_kernel_time();
	
	PDEBUG("jiffies %lu\n", jtime);
	PDEBUG("timespec %lu %lu timespec_to_jiffies %lu\n", spec.tv_sec, spec.tv_nsec, timespec_to_jiffies(&spec));
	seq_printf(m, "jiffies %lu\n", jtime);
	seq_printf(m, "timespec %lu %lu timespec_to_jiffies %lu\n", spec.tv_sec, spec.tv_nsec, timespec_to_jiffies(&spec));
	return 0;
}

static int topen(struct inode *inode ,struct file *file) {
	return single_open(file, tshow, NULL);
}

static struct file_operations ops = {
	.open = topen,
	.llseek = seq_lseek,
	.read = seq_read,
	.release = single_release,
};

static int __init tstart(void) {
	PDEBUG("in tstart\n");
	proc_create("timerproc", 0, NULL, &ops);
	return 0;
}

static void __exit tend(void) {
	remove_proc_entry("timerproc", NULL);
	PDEBUG("in tend\n");
}

MODULE_AUTHOR("muryliang");
MODULE_LICENSE("GPL");
module_init(tstart);
module_exit(tend);
