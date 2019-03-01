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
//#include "camdev.h"

#include <linux/fs.h> 
#include <linux/file.h> 
#include <linux/mm.h> 
#include <linux/cdev.h>
#include <asm/uaccess.h>

#define CAMNAME "linecam"

typedef struct linecamdev
{
	struct cdev	*cdevptr;
	struct spi_device	*spidev;

	struct spi_message	spi_msg1;
	struct spi_transfer	spi_xfer1;
	u8 *sendbuf;

}linecamdev_t;

struct linecamdev *camdev;
static int read_lines = 1;
static int read_width = 1;
static int frame_head = 6;

static int line_cam_read(struct linecamdev *camdev, u8 *spi_data_value, u16 spi_data_len)
{
//	struct spi_transfer *xfer = &camdev->spi_xfer1;
//	struct spi_message *msg = &camdev->spi_msg1;
	int ret;
        struct spi_message      msg;
        struct spi_transfer     xfer;

	/* initialise pre-made spi transfer messages */
	spi_message_init(&msg);
	spi_message_add_tail(&xfer, &msg);
	xfer.bits_per_word = 8;
	xfer.len = spi_data_len;
	xfer.tx_buf = camdev->sendbuf;
	xfer.rx_buf = spi_data_value;
	ret = spi_sync(camdev->spidev, &msg);
//	pr_alert("aaaa %02x %02x %02x %02x\n", spi_data_value[9], spi_data_value[1], spi_data_value[2],spi_data_value[3]);

	return ret;
}

static int chr_release (struct inode * ind, struct file * fs) {
    pr_alert("start release linecamera\n");
    fs->private_data = NULL;
    return 0;
}

static int chr_open (struct inode *ind, struct file *fs) {
    pr_alert("start open linecamera\n");
    fs->private_data = camdev;
    return 0;
}

static ssize_t chr_read (struct file *fptr, char __user * userbuf, size_t siz, loff_t *nouse) {

    int ret; 
    u8* tmpbuf = NULL;
    struct linecamdev *dev = (struct linecamdev*)fptr->private_data;

//    pr_alert("start read linecamera\n");
	if (siz != read_width * read_lines + frame_head) {
		pr_alert("read size not match from kernel and userspace\n");
		return -2;
	}
    tmpbuf = (u8*)kzalloc(siz, GFP_KERNEL);
    if (!tmpbuf) {
        pr_alert("can not allocate kernel buffer for linecamerea read\n");
        return -1;
    }
    
    ret = line_cam_read(dev, tmpbuf, siz);
    if (ret == 0) { // spi_sync success
        ret = copy_to_user((char __user*)userbuf, tmpbuf, siz);
        ret = siz;  
    }
    kfree(tmpbuf);
//    pr_alert("return size is %d\n", ret);
    return ret;
}

static ssize_t chr_write (struct file *fptr, const char __user *userbuf, size_t siz, loff_t *nouse) {
    pr_alert("no support write linecamera\n");
    return -1;
}

static struct file_operations camops = {
    .open = chr_open,
    .release = chr_release,
    .read = chr_read,
    .write = chr_write,
};

/* driver bus management functions */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,8,0)
static int camdev_probe(struct spi_device *spi)
#else
static int __devinit camdev_probe(struct spi_device *spi)
#endif
{
	struct cdev * chrdev;
//	struct device_node *np = spi->dev.of_node;
    dev_t devnum;
    int ret = 0;

	pr_alert("linecamera: %s,line=%d, busnumber %d\n", __func__,__LINE__, spi->master->bus_num);
	//pr_alert("camdev max_speed_hz=%d,node_name=%s\n",spi->max_speed_hz,spi->dev.of_node->name);

    // alloc private structure
    camdev = (struct linecamdev*)kmalloc(sizeof(struct linecamdev), GFP_KERNEL);
    if (!camdev) {
        pr_alert("can not alloc memory for camdev structure\n");
        ret = -1;
        goto ERROR3;
    }

    // register a chardeivce
    if (alloc_chrdev_region(&devnum, 0, 1, CAMNAME) != 0) {
        pr_alert("can not alloc char dev number for linecamera\n");
        ret = -1;
        goto ERROR2;
    }

    chrdev = cdev_alloc();
	if (!chrdev)
	{
		pr_alert("failed to alloc linecamera character device\n");
		ret = -ENOMEM;
        goto ERROR1;
	}
    cdev_init(chrdev, &camops);
    if (cdev_add(chrdev, devnum, 1) != 0) {
        pr_alert("can not add\n");
        ret = -ENOMEM;
        goto ERROR1;
    }

	spi->bits_per_word = 8;

	camdev->cdevptr = chrdev;
	camdev->spidev = spi;

        if (read_lines == 1 || read_width == 1) {
                pr_info("forget to set read_width or read_lines in param!!\n");
                ret = -EFAULT;
                goto ERROR1;
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

	return 0;
ERROR0:
    kfree(camdev->sendbuf);
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

    unregister_chrdev_region(priv->cdevptr->dev, 1);
    cdev_del(priv->cdevptr);
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

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,8,0)
	.remove = camdev_remove,
#else
	.remove = __devexit_p(camdev_remove),
#endif

};

static int __init camdev_init(void)
{
	return spi_register_driver(&camdev_driver);
}

static void __exit camdev_exit(void)
{
	spi_unregister_driver(&camdev_driver);
}

//module_init(camdev_init);
late_initcall(camdev_init);
module_exit(camdev_exit);

module_param(read_lines, int, 0644);
module_param(read_width, int, 0644);
module_param(frame_head, int, 0644);
MODULE_DESCRIPTION("LineCamera driver");
MODULE_AUTHOR("cynovo");
MODULE_LICENSE("GPL");

//MODULE_PARM_DESC(message, "");
//MODULE_ALIAS("spi:camdev");
