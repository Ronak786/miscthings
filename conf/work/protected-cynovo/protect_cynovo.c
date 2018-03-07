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

#include "hlog.h"
#include "protect_cynovo.h"

#define cynovo_page "/protect_cynovo/cynovo_page_"

int handle;
int page_size = 0;

int open_disk()
{
	return 0;
}

int close_disk()
{
	return 0;
}


int read_pages(int page,unsigned char *buf)
{	
	FILE *fp;
	char file_name[64]={0};
	int ret = -1;
	
	memset(file_name,0,sizeof(file_name));
	sprintf(file_name, "%s%d",cynovo_page,page);

	fp = fopen(file_name,"r");
	if(fp==NULL)
		goto out;
	
	ret = fread(buf,1,4096,fp); 

	fclose(fp);
out:
	return ret;
}


int erase_page(int page)
{
	FILE *fp;
	char file_name[64]={0};
	int ret = -1;
	
	memset(file_name,0,sizeof(file_name));
	sprintf(file_name, "%s%d",cynovo_page,page);

	ret = remove(file_name);

	return 0;
}

int write_page(int page,unsigned char *buf,int length)
{
		FILE *fp;
		char file_name[64]={0};
		int ret = -1;
		
		memset(file_name,0,sizeof(file_name));
		sprintf(file_name, "%s%d",cynovo_page,page);
		
		fp = fopen(file_name,"w+");
		if(fp==NULL){
			hlog(H_INFO,"fopen error code = %d \n",errno);	
			goto out;
		}
		
		ret = fwrite(buf,1,length,fp); 
		
		fclose(fp);
	out:
		return ret;
}
