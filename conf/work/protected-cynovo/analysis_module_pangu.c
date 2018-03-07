#include "stdio.h"
#include "protect_cynovo.h"
#include "hlog.h"
#include "analysis_module_pangu.h"

static cmd_analysis_t cmd_pangu[] =
{
    END() 
};  

static int handle_WritePage(analysis_module_t * module)// in: page(int),pagedata(char[]),datalength(in) ; out: ret(int)
{
	int ret = -1;
	unsigned char *data = malloc(sizeof(unsigned char)*MAXLENGTH);
	int datalen = 0;

	int page = 0;				
	unsigned char *pagedata = malloc(sizeof(unsigned char)*MAXLENGTH);
	int pagedatalen = 0;	
	char retdata[1] = {0};		

	datalen = MAXLENGTH;
	if(get(module,"page", data, &datalen))          
	{
		retdata[0] = -1;
		module->datalen = 0;			
		set(module, "ret", retdata, 1);
		ret = -1;
		goto EXIT;					
	}
	page = data[0]*256+ data[1];

	datalen = MAXLENGTH;		
	if(get(module,"pagedatalen", data, &datalen))          
	{
		retdata[0] = -1;
		module->datalen = 0;
    	set(module, "ret", retdata, 1);
		ret = -1;
		goto EXIT;			
    }
	pagedatalen = data[0]*256+ data[1];


	datalen = MAXLENGTH;		
	if(get(module,"pagedata", data, &datalen))          
    {
    	retdata[0] = -1;
		module->datalen = 0;
    	set(module, "ret", retdata, 1);
		ret = -1;
		goto EXIT;			
    }
	if(datalen < pagedatalen)
    {
    	retdata[0] = -1;
		module->datalen = 0;
    	set(module, "ret", retdata, 1);
		ret = -1;
		goto EXIT;			
    }	
	memcpy(pagedata,data,datalen);
	hlog(H_INFO,"pagedatalen = %d  \n",pagedatalen);	
	hlog(H_INFO,"page= %d  \n",page);	
	hlog_data(H_INFO, "pagedata", pagedata, pagedatalen);

	ret = open_disk();
	if(ret < 0){
    	retdata[0] = -1;
		module->datalen = 0;
    	set(module, "ret", retdata, 1);
		ret = -1;
		goto EXIT;				
	}
	ret= write_page(page,pagedata,pagedatalen);
	if(ret < 0){
		module->datalen = 0;					
		retdata[0] = -1;
		set(module, "ret", retdata, 1);
		ret = -1;
		goto EXIT;				
	}
	close_disk();	
	
	module->datalen = 0;		
	retdata[0] = 0;
	set(module, "ret", retdata, 1);	
EXIT:
	free(data);
	free(pagedata);	
	return ret;
}

static int handle_ReadPage(analysis_module_t * module)// in: page(int); out: ret(int) pagedatalength(int) pagedata(char[])
{
	int ret = -1;
	unsigned char *data = malloc(sizeof(unsigned char)*MAXLENGTH);
	int datalen = 0;

	int page = 0;				
	unsigned char *pagedata = malloc(sizeof(unsigned char)*MAXLENGTH);
	int pagedatalen = 0; 
	unsigned char upagedatalen[2] = {0};
	unsigned char retdata[1] = {0};		

	datalen = MAXLENGTH;
	if(get(module,"page", data, &datalen))			
	{
		retdata[0] = -1;
		module->datalen = 0;			
		set(module, "ret", retdata, 1);
		ret = -1;
		goto EXIT;					
	}
	page = data[0]*256+ data[1];
	hlog(H_INFO,"page= %d  \n",page);	

	ret = open_disk();
	if(ret < 0){
		module->datalen = 0;					
		retdata[0] = -1;
		set(module, "ret", retdata, 1);
		ret = -1;
		goto EXIT;				
	}
	
	pagedatalen = read_pages(page,pagedata);
	
	if(pagedatalen <0){
		module->datalen = 0;					
		retdata[0] = -1;
		set(module, "ret", retdata, 1);
		ret = -1;
		goto EXIT;			
	}
	
	close_disk();	
	
	upagedatalen[0] = pagedatalen %256;
	upagedatalen[1] = pagedatalen /256;
	retdata[0] = 0;

	module->datalen = 0;			
	set(module, "pagedatalength", upagedatalen, 2);
	set(module, "pagedata", pagedata, pagedatalen);
	set(module, "ret", retdata, 1);
	ret = 0;
	
EXIT:
	free(data);
	free(pagedata);
	
	return ret;

}

static int handle_ErasePage(analysis_module_t * module)// in: page(int); out: ret(int)
{
	int ret = -1;
	unsigned char *data = malloc(sizeof(unsigned char)*MAXLENGTH);
	
	int datalen = 0;

	int page = 0;				
	char retdata[1] = {0};		

	datalen = MAXLENGTH;
	if(get(module,"page", data, &datalen))			
	{
		retdata[0] = -1;
		module->datalen = 0;			
		set(module, "ret", retdata, 1);
		ret = -1;
		goto EXIT;					
	}
	page = data[0]*256+ data[1];

	hlog(H_INFO,"page= %d  \n",page);	

	ret = open_disk();
	if(ret < 0){
		retdata[0] = -1;
		module->datalen = 0;
		set(module, "ret", retdata, 1);
		ret = -1;
		goto EXIT;				
	}
	retdata[0] = erase_page(page);
	close_disk();	
	
	module->datalen = 0;		
	set(module, "ret", retdata, 1); 
EXIT:
	free(data);
	return ret;

}

static int handle_Updatelogo(analysis_module_t * module)// in: disk(char *); logofile(char*)  out: ret(int)
{
	int ret = -1;
	unsigned char *data = malloc(sizeof(unsigned char)*MAXLENGTH);
	int datalen = 0;		
	char retdata[1] = {0};		
	char disk[128]= {0};
	char logofile[128]= {0};


	datalen = MAXLENGTH;
	if(get(module,"disk", data, &datalen))			
	{
		retdata[0] = -1;
		module->datalen = 0;			
		set(module, "ret", retdata, 1);
		ret = -1;
		goto EXIT;					
	}
	memset(disk,0,128);
	memcpy(disk,data,datalen);
	hlog(H_INFO,"disk= %s  \n",disk);	


	datalen = MAXLENGTH;
	if(get(module,"logofile", data, &datalen))			
	{
		retdata[0] = -1;
		module->datalen = 0;			
		set(module, "ret", retdata, 1);
		ret = -1;
		goto EXIT;					
	}
	memset(logofile,0,128);
	memcpy(logofile,data,datalen);
	hlog(H_INFO,"logofile= %s  \n",logofile);


	ret = open_logo(disk);	
	if(ret < 0){
		retdata[0] = -1;
		module->datalen = 0;			
		set(module, "ret", retdata, 1);
		ret = -1;
		goto EXIT;
	}
	ret = erase_logo();
	if(ret < 0){
		retdata[0] = -1;
		module->datalen = 0;			
		set(module, "ret", retdata, 1);
		ret = -1;
		goto EXIT;
	}
	ret = write_logo(logofile);
	if(ret < 0){
		retdata[0] = -1;
		module->datalen = 0;			
		set(module, "ret", retdata, 1);
		ret = -1;
		goto EXIT;
	}
	ret = close_logo();
	if(ret < 0){
		retdata[0] = -1;
		module->datalen = 0;			
		set(module, "ret", retdata, 1);
		ret = -1;
		goto EXIT;
	}
	
	module->datalen = 0;		
	set(module, "ret", retdata, 1); 
EXIT:
	free(data);
	return ret;

}

protected_cynovo_cmd_t protected_cynovo_cmd_list[] = {
	{
		.cmd = CMD_WRITEPAGE,
		.handlecmd = handle_WritePage,
	},
	{
		.cmd = CMD_READPAGE,
		.handlecmd = handle_ReadPage,
	},
	{
		.cmd = CMD_ERASEPAGE,
		.handlecmd = handle_ErasePage,
	},
	{
		.cmd = CMD_UPDATELOGO,
		.handlecmd = handle_Updatelogo,
	},	
};

int get_protected_cynovo_cmd_length()
{
    return ARRAY_SIZE(protected_cynovo_cmd_list);
}

protected_cynovo_cmd_t * get_cynovo_device(int id)
{
	protected_cynovo_cmd_t * pdev = NULL;
    pdev = &protected_cynovo_cmd_list[id];
    return pdev;
}

analysis_module_t analysis_module_pangu =
{
    .cmd = cmd_pangu,
	.socket_type = SOCKET_TYPE_LOCAL,
	.socket_addr = {0},
};

