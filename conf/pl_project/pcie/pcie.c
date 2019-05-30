#include <linux/pci.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/version.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/ioport.h>
#include <linux/errno.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/major.h>
#include <linux/string.h>
#include <linux/fcntl.h>
#include <linux/ptrace.h>
#include <linux/serial.h>
#include <linux/time.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/device.h>
#include <linux/msi.h>

#include <linux/netdevice.h>
#include <linux/semaphore.h>
#include <linux/pcieport_if.h>
#include <linux/ide.h>
#include <linux/kernel.h>
#include <linux/mempool.h>
#include <linux/timer.h>
//#include <asm/dma-mapping.h>
#include "pcie.h"

#define DUMMY_DEVICE_DOMAIN_INFO ((struct device_domain_info *)(-1))

struct pcie_conf dev;
int barnum = 0;
struct class *pcie56_class;
struct sem_driver sem_array[6];
void hexdump(void* data,int len)
{
	int i;
	{
		for (i=0; i < len; i++)
		{
			if ((i % 16) == 0)
			{
				printk(KERN_EMERG"\n");
			}
			printk(KERN_EMERG"%02x ", ((unsigned char *)data)[i]);
		}
	}
}

int test_writeread() 
{
	printk(KERN_EMERG "test_writeread.\n");
	int i = 0, errorcalc = 0;
	long long tmp = 0;
	local_irq_disable();
	for(i = 0; i < 128; i++)
	{
		((long long *)dev.dmaWRBuffer)[i] = i + 128;
	}
	writel(0x100, dev.bar[0] + 0x60);
	writel(0, dev.bar[0] + 0x60);	
	writel(1, dev.bar[0] + 0x0);	
	writel(0, dev.bar[0] + 0x0);	
	writel(dev.dmaWRBufAddr, dev.bar[0] + 0x1c);
	writel(dev.dmaRDBufAddr, dev.bar[0] + 0x8);
	writel(0x10, dev.bar[0] + 0x20);
	writel(0x20, dev.bar[0] + 0x0c);			
	writel(128/64, dev.bar[0] + 0x24);
	writel(128/128, dev.bar[0] + 0x10);
	writel(0x10001, dev.bar[0] + 0x4);
	
	i = 0;
	while((tmp&0x1000100) != 0x1000100)
  	{
  		i++;
	 	if(i == 3000000)
	 	{
	 		break;
	 	}
		if(i%100 == 0)
		{
			printk(KERN_EMERG "write, i = %d, reg 0x4 = 0x%x.\n", i, tmp);
		}
		udelay(1);
	 	tmp = readl(dev.bar[0] + 0x4);
	 	smp_mb();
  	}
	if(i == 3000000)
	{
	 	return ERR_DMAWRITE_OVERTIME;
	}
	tmp = ((long long *)dev.dmaRDBuffer)[0];
	for(i = 1; i < 128/8; i++)
	{
		if(((long long *)dev.dmaRDBuffer)[i] != (tmp + 1))
		{
			errorcalc++;
			printk(KERN_EMERG "tmp = 0x%llx, testdata[%d] = 0x%llx.\n", tmp, i, ((long long *)dev.dmaRDBuffer)[i]);
		}
		tmp = ((long long *)dev.dmaRDBuffer)[i];
	}
	printk(KERN_EMERG "error calc = 0x%x.\n", errorcalc);
  	local_irq_enable();
	return 0;	
}

//write
int Pcie56_DMA_H2L_Command(int dmano, long long hostAddr, int length) 
{
	//printk(KERN_EMERG "Pcie56_DMA_H2L.\n");

	int i = 0, tmp = 0x04;
	//int oldvalue = 0x00000000, newvalue = 0x00000000;
	 
	*(long long *)dev.descWRBuffer = dev.dmaWRBufAddrcommand;
	
	//printk(KERN_EMERG "length = 0x%x.\n", length);
	*(int *)(dev.descWRBuffer + 8) = length;
	*(long long *)(dev.descWRBuffer + 12) = 0x3;//dev.descWRAddr;
	*(int *)(dev.descWRBuffer + 20) = 0x12345678;
	*(int *)(dev.descWRBuffer + 24) = 0x90abcdef;
	*(int *)(dev.descWRBuffer + 28) = 0x1234cdef;

	//hexdump(dev.descWRBuffer, 32);
	writel((unsigned int)dev.descWRAddr, dev.bar[0] + 0x00);
	writel((unsigned int)(dev.descWRAddr>>32), dev.bar[0] + 0x04);
	writel(0x20, dev.bar[0] + 0x08);
	writel(0x14, dev.bar[0] + 0x0c);

  
	/*//bit11 become 0 form 1
	while(((newvalue&0x04) == 0x04)&&((oldvalue&0x00000800) == 0x00000800)&&((newvalue&0x00000800) == 0x00000000))
  	{
  		printk(KERN_EMERG "oldvalue = 0x%x, newvalue = 0x%x.\n", oldvalue, newvalue);
  		oldvalue = newvalue;
  		i++;
	 	if(i == 3000000)
	 	{
	 		break;
	 	}
		udelay(1);
	 	newvalue = readl(dev.bar[0] + 0x6c);
	 	smp_mb();
  	}*/
  	while((tmp&0x04) != 0x00)
  	{
  		i++;
	 	if(i == 3000000)
	 	{
	 		break;
	 	}
		//udelay(1);
	 	tmp = readl(dev.bar[0] + 0x0c);
	 	smp_mb();
  	}
	if(i == 3000000)
	{
		printk(KERN_EMERG "write overtime, tmp = 0x%x.\n", tmp);
	 	return ERR_DMAWRITE_OVERTIME;
	}
  	local_irq_enable();
	return 0;	
}

//read
int Pcie56_DMA_L2H_Command(int dmano, int hostAddr, int length) 
{
	int i = 0, tmp = 0x04;
	int oldvalue = 0x00000000, newvalue = 0x00000000;
	 
	*(long long *)dev.descRDBuffer = dev.dmaRDBufAddrcommand;
	//printk(KERN_EMERG "length = 0x%x.\n", length);
	*(int *)(dev.descRDBuffer + 8) = length;
	*(long long *)(dev.descRDBuffer + 12) = 0x3;//dev.descRDAddr;
	*(int *)(dev.descRDBuffer + 20) = 0x12345678;
	*(int *)(dev.descRDBuffer + 24) = 0x90abcdef;
	*(int *)(dev.descRDBuffer + 28) = 0x1234cdef;

	writel((unsigned int)dev.descRDAddr, dev.bar[0] + 0x10);
	writel((unsigned int)(dev.descRDAddr>>32), dev.bar[0] + 0x14);
	writel(0x20, dev.bar[0] + 0x18);
	writel(0x14, dev.bar[0] + 0x1c);

	/*while(((newvalue&0x04) == 0x04)&&((oldvalue&0x00000800) == 0x00000800)&&((newvalue&0x00000800) == 0x00000000))
  	{  
  		printk(KERN_EMERG "oldvalue = 0x%x, newvalue = 0x%x.\n", oldvalue, newvalue);
  		oldvalue = newvalue;
  		i++;
	 	if(i == 3000000)
	 	{
	 		break;
	 	}
		udelay(1);
	 	newvalue = readl(dev.bar[0] + 0x7c);
	 	smp_mb();
  	}*/
	while((tmp&0x04) != 0x00)
  	{
  		i++;
	 	if(i == 3000000)
	 	{
	 		break;
	 	}
		//udelay(1);
	 	tmp = readl(dev.bar[0] + 0x1c);
	 	smp_mb();
  	}
	if(i == 3000000)
	{
		printk(KERN_EMERG "read overtime, tmp = 0x%x.\n", tmp);
	 	return ERR_DMAREAD_OVERTIME;
	}
  	local_irq_enable();
	return 0;	
}

//write_
int Pcie56_DMA_H2L(int dmano, long long hostAddr, int length) 
{
	//printk(KERN_EMERG "Pcie56_DMA_H2L.\n");

	int i = 0, tmp = 0x04;
	//int oldvalue = 0x00000000, newvalue = 0x00000000;
	 
	*(long long *)dev.descWRBuffer = dev.dmaWRBufAddr;
	
	//printk(KERN_EMERG "length = 0x%x.\n", length);
	*(int *)(dev.descWRBuffer + 8) = length;
	*(long long *)(dev.descWRBuffer + 12) = 0x3;//dev.descWRAddr;
	*(int *)(dev.descWRBuffer + 20) = 0x12345678;
	*(int *)(dev.descWRBuffer + 24) = 0x90abcdef;
	*(int *)(dev.descWRBuffer + 28) = 0x1234cdef;

	//hexdump(dev.descWRBuffer, 32);
	writel((unsigned int)dev.descWRAddr, dev.bar[0] + 0x60);
	writel((unsigned int)(dev.descWRAddr>>32), dev.bar[0] + 0x64);
	writel(0x20, dev.bar[0] + 0x68);
	writel(0x14, dev.bar[0] + 0x6c);


	/*//bit11 become 0 form 1
	while(((newvalue&0x04) == 0x04)&&((oldvalue&0x00000800) == 0x00000800)&&((newvalue&0x00000800) == 0x00000000))
  	{
  		printk(KERN_EMERG "oldvalue = 0x%x, newvalue = 0x%x.\n", oldvalue, newvalue);
  		oldvalue = newvalue;
  		i++;
	 	if(i == 3000000)
	 	{
	 		break;
	 	}
		udelay(1);
	 	newvalue = readl(dev.bar[0] + 0x6c);
	 	smp_mb();
  	}*/
  	while((tmp&0x04) != 0x00)
  	{
  		i++;
	 	if(i == 3000000)
	 	{
	 		break;
	 	}
		//udelay(1);
	 	tmp = readl(dev.bar[0] + 0x6c);
	 	smp_mb();
  	}
	if(i == 3000000)
	{
		printk(KERN_EMERG "write overtime, tmp = 0x%x.\n", tmp);
	 	return ERR_DMAWRITE_OVERTIME;
	}
  	local_irq_enable();
	return 0;	
}

//read
int Pcie56_DMA_L2H(int dmano, int hostAddr, int length) 
{
	int i = 0, tmp = 0x04;
	int oldvalue = 0x00000000, newvalue = 0x00000000;
	 
	*(long long *)dev.descRDBuffer = dev.dmaRDBufAddr;
	//printk(KERN_EMERG "length = 0x%x.\n", length);
	*(int *)(dev.descRDBuffer + 8) = length;
	*(long long *)(dev.descRDBuffer + 12) = 0x3;//dev.descRDAddr;
	*(int *)(dev.descRDBuffer + 20) = 0x12345678;
	*(int *)(dev.descRDBuffer + 24) = 0x90abcdef;
	*(int *)(dev.descRDBuffer + 28) = 0x1234cdef;

	writel((unsigned int)dev.descRDAddr, dev.bar[0] + 0x70);
	writel((unsigned int)(dev.descRDAddr>>32), dev.bar[0] + 0x74);
	writel(0x20, dev.bar[0] + 0x78);
	writel(0x14, dev.bar[0] + 0x7c);

	/*while(((newvalue&0x04) == 0x04)&&((oldvalue&0x00000800) == 0x00000800)&&((newvalue&0x00000800) == 0x00000000))
  	{  
  		printk(KERN_EMERG "oldvalue = 0x%x, newvalue = 0x%x.\n", oldvalue, newvalue);
  		oldvalue = newvalue;
  		i++;
	 	if(i == 3000000)
	 	{
	 		break;
	 	}
		udelay(1);
	 	newvalue = readl(dev.bar[0] + 0x7c);
	 	smp_mb();
  	}*/
	while((tmp&0x04) != 0x00)
  	{
  		i++;
	 	if(i == 3000000)
	 	{
	 		break;
	 	}
		//udelay(1);
	 	tmp = readl(dev.bar[0] + 0x7c);
	 	smp_mb();
  	}
	if(i == 3000000)
	{
		printk(KERN_EMERG "read overtime, tmp = 0x%x.\n", tmp);
	 	return ERR_DMAREAD_OVERTIME;
	}
  	local_irq_enable();
	return 0;	
}


void AlgDmaReset(int cardid, int algnum)
{	
	writel(0x100, dev.bar[0] + 0x60);
	writel(0x00, dev.bar[0] + 0x60);
	writel(0x01, dev.bar[0] + 0x00);
	writel(0x00, dev.bar[0] + 0x00);
}

int AlgDmaWrite_unlock(int cardid, int algnum, int len, char *buf)
{
	int ret = 0;

	memcpy(dev.dmaWRBuffer, buf, len);
	ret = Pcie56_DMA_H2L(algnum, dev.dmaWRBufAddr, (len + 127)/128*128);
	if(ret != 0)
	{
		printk(KERN_EMERG "Pcie56_DMA_H2L error, ret = 0x%x.\n", ret);
		goto FUNCOVER;
	}
	
FUNCOVER:
	return ret;
}

//算法操作由算法模块给出长度，长度是128整数倍，数据不足整数倍则添0
int AlgDmaRead_unlock(int cardid, int algnum, int *len, char *buf)
{
	int ret = 0;
	
	ret = Pcie56_DMA_L2H(algnum, dev.dmaRDBufAddr, 128);
	if(ret != 0)
	{
		printk(KERN_EMERG "Pcie56_DMA_L2H head error, ret = 0x%x.\n", ret);
		goto FUNCOVER;
	}
	memcpy(buf, dev.dmaRDBuffer, 128);
	*len = 0;
	memcpy(len, buf + 6, 2);//returnlen
	//printk(KERN_EMERG "read head success, *len = 0x%x.\n", *len);
	if((*len < 0)||(*len > 16384))
	{
		printk(KERN_EMERG "*len error, *len = 0x%x.\n", *len);
		ret = ERR_DMAREADLENGTH;
		goto FUNCOVER;
	}
	if((*len - 128 + 127)/128*128 > 0)
	{
		ret = Pcie56_DMA_L2H(algnum, dev.dmaRDBufAddr, (*len - 128 + 127)/128*128);
		if(ret != 0)
		{
			printk(KERN_EMERG "Pcie56_DMA_L2H package error, ret = 0x%x.\n", ret);
			goto FUNCOVER;
		}
		memcpy(buf + 128, dev.dmaRDBuffer, *len - 128);
	}	
FUNCOVER:
	return ret;		
}

//inlen is needn't convert to fit 128 bytes
int PKIE_Command_unlock(int cardid, int inlen, char *in, int *outlen, char *out)
{
	int ret = 0;
	struct Communication *comm = (struct Communication *)in;
	
	comm->project = 0x0810;
	comm->sourceid = 0x01;
	comm->destinationid = 0x00;
	comm->packagelength = inlen + 32;	
	comm->returnlength = inlen + 32;
	comm->key0_en = 0x02;
	comm->key0_len = 0x06;	
	comm->key0_reserve = 0x04;
	comm->key0_num = 0x40;
	comm->key1_reserve = 0x04;
	comm->key1_len = 0x06;
	comm->key1_en = 0x02;
	comm->key1_num = 0x40;
	
	ret = AlgDmaWrite_unlock(cardid, 0, comm->packagelength, in);
	if(ret != 0)
	{
		printk(KERN_EMERG "AlgDmaWrite_unlock error, ret = 0x%x.\n", ret);
		goto FUNCOVER;
	}
	ret = AlgDmaRead_unlock(cardid, 0, outlen, out);
	if(ret != 0)
	{
		printk(KERN_EMERG "AlgDmaRead_unlock error, ret = 0x%x.\n", ret);
		goto FUNCOVER;
	}	
FUNCOVER:
	return ret;	
}

int AlgDmaWrite(int cardid, int algnum, int len, char *buf)
{
	int ret = 0;
	
	down(&sem_array[SEM_COMM]);
	sem_array[SEM_COMM].pid = 0;
	sem_array[SEM_COMM].used = 1;	
	ret = AlgDmaWrite_unlock(cardid, algnum, len, buf);
	sem_array[SEM_COMM].pid = 0;
	sem_array[SEM_COMM].used = 0;
	up(&sem_array[SEM_COMM]);
	return ret;
}

int AlgDmaRead(int cardid, int algnum, int *len, char *buf)
{
	int ret = 0;
	
	down(&sem_array[SEM_COMM]);
	sem_array[SEM_COMM].pid = 0;
	sem_array[SEM_COMM].used = 1;	
	ret = AlgDmaRead_unlock(cardid, algnum, len, buf);
	sem_array[SEM_COMM].pid = 0;
	sem_array[SEM_COMM].used = 0;
	up(&sem_array[SEM_COMM]);
	return ret;
}

int PKIE_Command(int cardid, int inlen, char *in, int *outlen, char *out)
{
	int ret = 0;
	//printk(KERN_EMERG "inlen = 0x%x.\n", inlen);
	down(&sem_array[SEM_COMM]);
	sem_array[SEM_COMM].pid = 0;
	sem_array[SEM_COMM].used = 1;	
	ret = PKIE_Command_unlock(cardid, inlen, in, outlen, out);
	sem_array[SEM_COMM].pid = 0;
	sem_array[SEM_COMM].used = 0;
	up(&sem_array[SEM_COMM]);
	return ret;
}

static int pcie56_open(struct inode *inode, struct file *filp) 
{ 
	//printk(KERN_EMERG "SECTOR_SIZE = 0x%x.\n", SECTOR_SIZE);
	//printk(KERN_EMERG "inode = 0x%x, filp = 0x%x.\n", inode, filp);
	//printk(KERN_EMERG "SECTOR_SHIFT = 0x%x.\n", SECTOR_SHIFT);
	return 0; 
} 

static int pcie56_release(struct inode *inode, struct file *filp) 
{
	int i = 0;
	for(i = 0; i < sizeof(sem_array)/sizeof(*sem_array); i++)
	{
		if((sem_array[i].used == 1)&&(sem_array[i].pid == current->pid))
		{
			printk(KERN_EMERG "pcie56_release, sem_array[%d], pid = %d\n", i, sem_array[i].pid);
			sem_array[i].used = 0;
			sem_array[i].pid = 0;
			up(&sem_array[i].sem);
		}
	}
	return 0;
} 
 
static ssize_t pcie56_write(struct file *file, const char *buf, size_t count, loff_t *ppos) 
{
	int ret = 0, i = 0;
	long long regdata = 0;
	struct pciedata datawrite;
  
	copy_from_user(&datawrite, buf, count);
	if((datawrite.length < 0) ||(datawrite.length > 65536))
	{
		printk(KERN_EMERG "datawrite.length = 0x%x.\n", datawrite.length);
		return ERR_PARAMETER;
	}
	if(datawrite.type == 0)
	{
		if(datawrite.length % 128 != 0)
		{
			printk(KERN_EMERG "datawrite.length = 0x%x.\n", datawrite.length);
			return ERR_PARAMETER;
		}
		copy_from_user(dev.dmaWRBuffer, datawrite.data, datawrite.length);
		//printk(KERN_EMERG "type = 0x%x.\n", datawrite.type);		
		ret = Pcie56_DMA_H2L(datawrite.type, dev.dmaWRBufAddr, datawrite.length);
		if(ret != 0)
		{
			goto FUNCOVER;
		}		
	}
	else if(datawrite.type == 2)
	{
		if(datawrite.length % 128 != 0)
		{
			printk(KERN_EMERG "datawrite.length = 0x%x.\n", datawrite.length);
			return ERR_PARAMETER;
		}
		//copy_from_user(dev.dmaWRBuffer, datawrite.data, datawrite.length);
		copy_from_user(dev.dmaWRBuffercommand, datawrite.data, datawrite.length);
		//hexdump(dev.dmaWRBuffer,datawrite.length);
		//printk(KERN_EMERG "type = 0x%x.\n", datawrite.type);
		
		ret = Pcie56_DMA_H2L_Command(datawrite.type, dev.dmaWRBufAddrcommand, datawrite.length);
		if(ret != 0)
		{
			goto FUNCOVER;
		}		
	}
	else if(datawrite.type == 1)
	{
		for(i=0; i < datawrite.length/8; i++)
		{
			copy_from_user(&regdata, datawrite.data + 8*i, 8);
			writeq(regdata, dev.bar[barnum] + datawrite.regaddr + 8*i);
		}
	}
	else
	{
		ret = ERR_PARAMETER;		
		goto FUNCOVER;
	}
FUNCOVER:
	//printk(KERN_EMERG "ret = 0x%x.\n", ret);
	return ret;
} 

static int pcie56_ioctl(struct file *filp,unsigned int cmd, unsigned long argAddr) 
{	
	int ret = 0;
	switch(cmd)
	{				
		case SET_BAR:
			copy_from_user(&barnum, (char *)argAddr, sizeof(barnum));
			printk(KERN_EMERG "barnum = 0x%x.\n", barnum);
			break;
		
		default:
			printk(KERN_EMERG "cmd = 0x%x.\n", cmd);
			ret = ERR_PARAMETER;
			break;
	}
	return ret;
} 

static int pcie56_read(struct file *file, char *buf,size_t count,loff_t *ppos) 
{
	int ret = 0, i = 0;
	long long regdata = 0;
	struct pciedata dataread;

	copy_from_user(&dataread, buf, count);
	if((dataread.type < 0) ||(dataread.type > 16)
	 ||(dataread.length < 0)||(dataread.length > 65536))
	{
		printk(KERN_EMERG "dataread.length = 0x%x.\n", dataread.length);
		return ERR_PARAMETER;
	}	
	else if(dataread.type == 0)
	{
		if(dataread.length % 128 != 0)
		{
			printk(KERN_EMERG "dataread.length = 0x%x.\n", dataread.length);
		}
		ret = Pcie56_DMA_L2H(dataread.type, dev.dmaRDBufAddr, dataread.length);
		if(ret != 0)
		{
			goto FUNCOVER;
		}
		copy_to_user(dataread.data, dev.dmaRDBuffer, dataread.length);
	}
	else if(dataread.type == 2)
	{
		if(dataread.length % 128 != 0)
		{
			printk(KERN_EMERG "dataread.length = 0x%x.\n", dataread.length);
		}
		ret = Pcie56_DMA_L2H_Command(dataread.type, dev.dmaRDBufAddrcommand, dataread.length);
		if(ret != 0)
		{
			goto FUNCOVER;
		}
		copy_to_user(dataread.data, dev.dmaRDBuffercommand, dataread.length);
	}
	else
	{
		for(i=0; i < dataread.length/8; i++)
		{
			//copy_to_user(dataread.data + 4*i, dev.bar[0] + dataread.regaddr + 4*i, 4);
			regdata = readq(dev.bar[barnum] + dataread.regaddr + 8*i);
			copy_to_user(dataread.data + 8*i, &regdata, 8);
		}
	}	
FUNCOVER:	
	//printk(KERN_EMERG "ret = 0x%x\n", ret);
	return ret;
} 

static struct file_operations pcie56_fops =
{
	owner:		THIS_MODULE,
	read:		pcie56_read,
	write:		pcie56_write,
	unlocked_ioctl:		pcie56_ioctl,
	mmap:		NULL,
	open:		pcie56_open,
	release:	pcie56_release,
};

struct pci_device_id pcie56_id[] = {
	{VENDOR_ID_V1, DEVICE_ID_V1, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 56},
	{VENDOR_ID_V2, DEVICE_ID_V2, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 56}
};

char *virtdma, *virtnormol;
int phydma, phynormal;

static int pcie56_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
	printk(KERN_EMERG "pcie56_probe\n");
	int ret = 0, data = 0, i = 0;
	struct msi_desc *entry;
	ret = pci_enable_device(pdev);
	if (ret != 0) 
	{
		return ret; 
	}
	
	pci_set_master(pdev);  	
 	
	dev.descWRBuffer = (char *)pci_alloc_consistent(pdev, 1024, &dev.descWRAddr);
	dev.descRDBuffer = (char *)pci_alloc_consistent(pdev, 1024, &dev.descRDAddr);	
	dev.dmaWRBuffer = (char *)pci_alloc_consistent(pdev, JMK_BUFFER_SIZE, &dev.dmaWRBufAddr);
	dev.dmaRDBuffer = (char *)pci_alloc_consistent(pdev, JMK_BUFFER_SIZE, &dev.dmaRDBufAddr);	
	dev.dmaWRBuffercommand = (char *)pci_alloc_consistent(pdev, JMK_BUFFER_SIZE, &dev.dmaWRBufAddrcommand);
	dev.dmaRDBuffercommand = (char *)pci_alloc_consistent(pdev, JMK_BUFFER_SIZE, &dev.dmaRDBufAddrcommand);

	//virtdma = (char *)kmalloc(8192, GFP_DMA);
	virtnormol = (char *)kmalloc(8192, GFP_KERNEL);
	printk(KERN_EMERG "dma_ops = 0x%x\n", dma_ops);
	printk(KERN_EMERG "get_dma_ops(&pdev->dev) = 0x%x\n", get_dma_ops(&pdev->dev));
	
	printk(KERN_EMERG "virtnormol = 0x%x\n", virtnormol);
	printk(KERN_EMERG "phydma = 0x%x\n", phydma);
	printk(KERN_EMERG "phynormal = 0x%x\n", phynormal);
	//dev.dmaWRBufAddr = virt_to_phys(dev.dmaWRBuffer);
	printk(KERN_EMERG "dev.descWRBuffer = 0x%llx\n", dev.descWRBuffer);
	printk(KERN_EMERG "dev.descWRAddr = 0x%llx\n", dev.descWRAddr);
	printk(KERN_EMERG "dev.descRDBuffer = 0x%llx\n", dev.descRDBuffer);
	printk(KERN_EMERG "dev.descRDAddr = 0x%llx\n", dev.descRDAddr);
	
	printk(KERN_EMERG "dev.dmaWRBuffer = 0x%llx\n", dev.dmaWRBuffer);
	printk(KERN_EMERG "dev.dmaWRBufAddr = 0x%llx\n", dev.dmaWRBufAddr);
	printk(KERN_EMERG "dev.dmaRDBuffer = 0x%llx\n", dev.dmaRDBuffer);
	printk(KERN_EMERG "dev.dmaRDBufAddr = 0x%llx\n", dev.dmaRDBufAddr);
	printk(KERN_EMERG "dev.dmaWRBuffercommand = 0x%llx\n", dev.dmaWRBuffercommand);
	printk(KERN_EMERG "dev.dmaWRBufAddrcommand = 0x%llx\n", dev.dmaWRBufAddrcommand);
	printk(KERN_EMERG "dev.dmaRDBuffercommand = 0x%llx\n", dev.dmaRDBuffercommand);
	printk(KERN_EMERG "dev.dmaRDBufAddrcommand = 0x%llx\n", dev.dmaRDBufAddrcommand);
	printk(KERN_EMERG "virt_to_phys(dev.dmaRDBuffer) = 0x%llx.\n", virt_to_phys(dev.dmaRDBuffer));
	printk(KERN_EMERG "virt_to_phys(dev.dmaWRBuffer) = 0x%llx.\n", virt_to_phys(dev.dmaWRBuffer));
	printk(KERN_EMERG "phys_to_virt(dev.dmaRDBufAddr) = 0x%llx.\n", phys_to_virt(dev.dmaRDBufAddr));
	printk(KERN_EMERG "phys_to_virt(dev.dmaWRBufAddr) = 0x%llx.\n", phys_to_virt(dev.dmaWRBufAddr));
	printk(KERN_EMERG "virt_to_bus(dev.dmaRDBuffer) = 0x%llx.\n", virt_to_bus(dev.dmaRDBuffer));
	printk(KERN_EMERG "virt_to_bus(dev.dmaWRBuffer) = 0x%llx.\n", virt_to_bus(dev.dmaWRBuffer));
	printk(KERN_EMERG "bus_to_virt(dev.dmaRDBufAddr) = 0x%llx.\n", bus_to_virt(dev.dmaRDBufAddr));
	printk(KERN_EMERG "bus_to_virt(dev.dmaWRBufAddr) = 0x%llx.\n", bus_to_virt(dev.dmaWRBufAddr));
	printk(KERN_EMERG "PAGE_SIZE = 0x%llx\n", PAGE_SIZE);
	printk(KERN_EMERG "PAGE_OFFSET = 0x%llx\n", PAGE_OFFSET);
	printk(KERN_EMERG "high_memory = 0x%llx\n", high_memory);
	printk(KERN_EMERG "VMALLOC_START = 0x%llx\n", VMALLOC_START);
	printk(KERN_EMERG "VMALLOC_END = 0x%llx\n", VMALLOC_END);
	printk(KERN_EMERG "num_physpages = 0x%llx\n", get_num_physpages);	
	
	

	
	ret = pci_enable_msi(pdev);	
	printk(KERN_EMERG "ret = 0x%x, pdev.irq = 0x%x\n", ret, pdev->irq);	
	
    	dev.bar[0] = (char *)ioremap(pci_resource_start(pdev,0), pci_resource_len(pdev,0));
	printk(KERN_EMERG "len = 0x%llx, virtual address of bar0 is 0x%llx, physical address of bar0 is 0x%x\n", pci_resource_len(pdev,0), dev.bar[0], pci_resource_start(pdev,0));
	dev.bar[1] = (char *)ioremap(pci_resource_start(pdev,1), pci_resource_len(pdev,1));
	printk(KERN_EMERG "len = 0x%llx, virtual address of bar1 is 0x%llx, physical address of bar1 is 0x%x\n", pci_resource_len(pdev,1), dev.bar[1], pci_resource_start(pdev,1));
	dev.bar[2] = (char *)ioremap(pci_resource_start(pdev,2), pci_resource_len(pdev,2));
	printk(KERN_EMERG "len = 0x%llx, virtual address of bar2 is 0x%llx, physical address of bar2 is 0x%x\n", pci_resource_len(pdev,2), dev.bar[2], pci_resource_start(pdev,2));
	dev.bar[3] = (char *)ioremap(pci_resource_start(pdev,3), pci_resource_len(pdev,3));
	printk(KERN_EMERG "len = 0x%llx, virtual address of bar3 is 0x%llx, physical address of bar3 is 0x%x\n", pci_resource_len(pdev,3), dev.bar[3], pci_resource_start(pdev,3));
	dev.bar[4] = (char *)ioremap(pci_resource_start(pdev,4), pci_resource_len(pdev,4));
	printk(KERN_EMERG "len = 0x%llx, virtual address of bar4 is 0x%llx, physical address of bar4 is 0x%x\n", pci_resource_len(pdev,4), dev.bar[4], pci_resource_start(pdev,4));
	dev.bar[5] = (char *)ioremap(pci_resource_start(pdev,5), pci_resource_len(pdev,5));
	printk(KERN_EMERG "len = 0x%llx, virtual address of bar5 is 0x%llx, physical address of bar5 is 0x%x\n", pci_resource_len(pdev,5), dev.bar[5], pci_resource_start(pdev,5));
	
	ret = register_chrdev(PCIE_MAJOR, PCIE_NAME, &pcie56_fops);
  	if (ret < 0)
  	{
	    printk(KERN_EMERG "register_chrdev error.\n");
	    return -1;
  	}
  	else
  	{
	    printk(KERN_EMERG "register_chrdev success.\n");
  	}
  	
	/*for(i=0; i < 512/8; i++)
	{
		printk("%x:%x ", 8*i, readq(dev.bar[0] + 8*i));	
		if(i%32 == 0)
		{
			printk("\n");
		}
	}*/

  	
	return ret;
}

static void pcie56_remove(struct pci_dev *pdev)
{
	printk(KERN_EMERG "pcie56_remove\n");		
	iounmap((void*)dev.bar[0]);
	iounmap((void*)dev.bar[2]); 
	pci_free_consistent(pdev, JMK_BUFFER_SIZE, dev.dmaRDBuffer, dev.dmaRDBufAddr);
	pci_free_consistent(pdev, JMK_BUFFER_SIZE, dev.dmaWRBuffer, dev.dmaWRBufAddr);
	pci_release_regions(pdev);
	pci_disable_device(pdev);			
}

static struct pci_driver pcie56_driver = {
	.name		= PCIE_NAME,
	.probe		= pcie56_probe,
	.remove		= pcie56_remove,
	.id_table	= pcie56_id,
};

int pcie_init(void)
{
	int ret = 0, i = 0;	
	ret = pci_register_driver(&pcie56_driver);
	if(ret != 0)
	{
		printk(KERN_EMERG "pci_register_driver error.\n");
	}	
	pcie56_class = class_create(THIS_MODULE, PCIE_NAME);
	device_create(pcie56_class, NULL, MKDEV(PCIE_MAJOR, 0), 0, PCIE_NAME); 
	memset(&sem_array, 0x00, sizeof(sem_array));
	
	return 0;
}

void pcie_exit(void)
{		
	device_destroy(pcie56_class, MKDEV(PCIE_MAJOR, 0));  
    	class_destroy(pcie56_class); 	
	pci_unregister_driver(&pcie56_driver); 
	unregister_chrdev(PCIE_MAJOR,PCIE_NAME);
	printk(KERN_EMERG "pcie exit.\n");
	return 0;
}

module_init(pcie_init);
module_exit(pcie_exit);
MODULE_LICENSE("GPL");

