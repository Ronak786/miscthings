#include "stdio.h"
#include "analysis_package.h"

static int wait_ack(unsigned char serialno,int delay){
	int ret = 0;
	unsigned char ack[3];
	
	ret = read_uart(ack,3,delay);
	//dump(ack,ret);
	if(ret!=3)
		return -1;
	if(ack[0] == 0xaa && ack[1] == 0xbb && ack[2] == serialno )
		return 0;

	return -1;
}

int cmd_shankhand(unsigned serialno)
{
	int ret =0,i;
	unsigned char buf[ANALYSIS_MSG_SIZE];
	unsigned char shankhand[18] = 
		{0xaa,0xbb,0x00,0x0d,0x00,
		 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		 0xcc,0xdd,0xe3};

	flush_uart();

	ret = write_uart(shankhand,18);
	ret = wait_ack(serialno,200*1000);	
	if(ret!=0)
		return -2;
	
	ret = read_uart(buf,128,300*1000);	
	if(ret>=17){
		for(i = 0;i<ret-5;i++)
		{
			if(buf[i]==0xaa && buf[i+1] == 0xbb)
			{				
				if(buf[i+2] == serialno && buf[i+5] == 0x0)
				{
					return 0;
				}
			}			
			break;
		}
	}
	
	return -2;
}

int cmd_version(int serialno)
{
	int ret =0;
	unsigned char cmd[ANALYSIS_MSG_SIZE];
	unsigned char buf[ANALYSIS_MSG_SIZE];
	int buflen = 0;
	int cmdlen = 0;
	unsigned char* cmddata = cmd + 7;
	
	cmdlen = pack_data(cmddata,cmdlen,"cmd",3,"basic.cmd.version",17,Value_Type_STR);
	cmdlen = pack_data_headtail(serialno,cmd,cmdlen);

	ret = write_uart(cmd,cmdlen);
	ret = wait_ack(serialno,200*1000);	
	if(ret!=0)
		return -2;

	buflen = read_uart(buf,256,200*1000);	
	if(buflen<=0)
		return -3;

	int startpos = check_data(buf,buflen,serialno);	
	if(startpos>=0)
	{
	
		printf("+++ getdata +++ \n");
		unsigned char result[48];
		ret = getdata(buf,buflen,startpos,"ret",3,result);
		printf("ret ret = %d \n",ret);
		if(ret >0){
			dump(result,ret);		
			printf("ret = %d \n",(int)result[0]);
		}

		ret = getdata(buf,buflen,startpos,"result",6,result);
		printf("result ret = %d \n",ret);		
		if(ret >0){
			dump(result,ret);	
			printf("result = %s \n",result);
		}

		ret = getdata(buf,buflen,startpos,
			"serialNo",8,result);		
		if(ret >0){
			dump(result,ret);	
			printf("serialNo = %s \n",result);
		}	

		ret = getdata(buf,buflen,startpos,
			"chipVer",7,result);		
		if(ret >0){
			dump(result,ret);	
			printf("chipVer = %s \n",result);
		}	

		ret = getdata(buf,buflen,startpos,
			"fwVer",5,result);		
		if(ret >0){
			dump(result,ret);	
			printf("fwVer = %s \n",result);
		}	

		ret = getdata(buf,buflen,startpos,
			"security",8,result);		
		if(ret >0){
			dump(result,ret);	
			printf("security = %s \n",result);
		}			
		printf("--- getdata ---\n");
		return 0;
	}

	return -4;
}

int cmd_qbcode(unsigned char serialno)
{
	int ret =0;
	unsigned char cmd[ANALYSIS_MSG_SIZE];
	unsigned char buf[ANALYSIS_MSG_SIZE];
	unsigned char qrcode[16] = {0x31,0x32,0x33,0x34,0x31,0x32,0x33,0x34,0x31,0x32,0x33,0x34,0x31,0x32,0x33,0x34};
	
	int buflen = 0;
	int cmdlen = 0;
	unsigned char* cmddata = cmd + 7;
	
	cmdlen = pack_data(cmddata,cmdlen,"cmd",3,"exscreen.cmd.qrcode",19,Value_Type_STR);
	cmdlen = pack_data(cmddata,cmdlen,"payType",7,"ALIPAY",6,Value_Type_STR);
	cmdlen = pack_data(cmddata,cmdlen,"data",4,qrcode,sizeof(qrcode),Value_Type_BIN);
	cmdlen = pack_data_headtail(serialno,cmd,cmdlen);


	ret = write_uart(cmd,cmdlen);
	ret = wait_ack(serialno,200*1000);	
	if(ret!=0)
		return -2;

	buflen = read_uart(buf,256,200*1000);	
	if(buflen<=0)
		return -3;

	int startpos = check_data(buf,buflen,serialno); 
	if(startpos>=0)
	{
	
		printf("+++ getdata +++ \n");
		unsigned char result[48];
		ret = getdata(buf,buflen,startpos,"ret",3,result);
		printf("ret ret = %d \n",ret);
		if(ret >0){
			dump(result,ret);		
			printf("ret = %d \n",(int)result[0]);
		}

		ret = getdata(buf,buflen,startpos,"result",6,result);
		printf("result ret = %d \n",ret);		
		if(ret >0){
			dump(result,ret);	
			printf("result = %s \n",result);
		}	
		printf("--- getdata ---\n");
		return 0;
	}

	return -4;
}		

