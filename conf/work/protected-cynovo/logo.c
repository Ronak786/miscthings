#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/mount.h>
#include <sys/statfs.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/un.h> 

#include "protect_cynovo.h"

#define BLOCK_SZ 0x800000
#define PAGE_SZ 0x800
#define PAGE_NUM (BLOCK_SZ/PAGE_SZ)

static int handle;

int open_logo(char * disk)
{
	int ret = 0;
	
	handle = open("/dev/block/platform/mtk-msdc.0/11230000.msdc0/by-name/logo",O_RDWR,0777);
	if(handle<0)
		return -1;
	return 0;
}

int close_logo()
{
 	return close(handle);
}


int read_logo()
{	
		return 0;
}


int erase_logo()
{
	unsigned char *bufdata = malloc(sizeof(unsigned char)*PAGE_SZ);
	int ret  = 0;
	int i = 0;
	memset(bufdata,0,PAGE_SZ);

	for(i = 0;i< PAGE_NUM ;i++)
	{
		ret = lseek(handle,i*PAGE_SZ,SEEK_SET);
		ret = write(handle,bufdata,PAGE_SZ);
	}
	
	return 0;
}

int write_logo(char * logopath)
{
	FILE * logofile;
	int ret = 0;
	int rc = 0;
	unsigned int of = 0;
	unsigned char *bufdata = malloc(sizeof(unsigned char)*PAGE_SZ);
	
	logofile = fopen(logopath, "rb");
	if(logofile == NULL){
		ret = -1;
		goto out1;
	}

	while(1)
	{
		lseek(handle,of,SEEK_SET);
		memset(bufdata,0,PAGE_SZ);
		rc = fread(bufdata,sizeof(unsigned char),PAGE_SZ,logofile);
		if(rc <= 0)
			break;
		ret = write(handle,bufdata,rc);
		if(ret != rc){
			ret = -1;
			goto out2;
		}
		of = rc + of;
	}	
out2:
	ret = close(logofile);
out1:
	
	free(bufdata);
	return ret;
}

