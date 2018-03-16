#include "stdio.h"

#include <stdlib.h>
#include <string.h>

#include "wolfssl/wolfcrypt/rsa.h"
#include "wolfssl/wolfcrypt/asn_public.h"
#include "wolfssl/ssl.h"

#include "der_cert.h"
#include "der.h"

void revertBuf(uint8_t* in,const uint32_t inlen){
	uint32_t i;
	uint8_t temp[280] = {0};	
	
	for(i=0;i<inlen;i++)
		temp[i] = in[i];
	
	for(i=0;i<inlen;i++)
		in[i]=temp[inlen-i-1];	
}


void revertUint32(uint8_t* in,uint32_t inlen){
	uint32_t i,count =0;
	uint8_t temp[4] = {0};	
	count =inlen/4;
	
	for(i=0;i<count;i++){
		temp[0] = in[i*4];
		temp[1] = in[i*4+1];
		temp[2] = in[i*4+2];
		temp[3] = in[i*4+3];
		in[i*4+3] = temp[0];
		in[i*4+2] = temp[1];
		in[i*4+1] = temp[2];
		in[i*4+0] = temp[3];
	}
}
int writeFile(const char*filepath,uint8_t* buf,uint32_t buflen)
{
	FILE *pFile=fopen(filepath,"w"); 
	fwrite (buf,1,buflen,pFile);
	fflush(pFile); 
	fclose(pFile); 
	return 0;
}


int readFile(const char*filepath,uint8_t * pBuf)
{
	FILE *pFile=fopen(filepath,"r"); 
	uint32_t len = 0;
	
	fseek(pFile,0,SEEK_END); 
	len=ftell(pFile); 
	
	rewind(pFile); 
	fread(pBuf,sizeof(char),len,pFile); 
	
	pBuf[len]=0; 
	fclose(pFile); 
	return len;
}

void dumpX(uint8_t * data,uint32_t len,uint32_t oneline)
{
	uint32_t line = oneline;
	uint32_t i = 0;
	printf("++++ dump hex[%d] ++++\n",len);
	for(i=0;i<len;i++){
		printf("0x%02x,",data[i]);
		//if((i+1)%8 == 0)
			//printf(" ");

		if((i+1)%line == 0)
			printf("\n");		
	}
	if(len%line != 0)
		printf("\n");
}

void dumpPrintf(uint8_t * data,uint32_t len)
{
	uint32_t line = 16;
	uint32_t i = 0;
	printf("++++ dump hex[%d] ++++\n",len);
	for(i=0;i<len;i++){
		printf("0x%02x,",data[i]);
		//if((i+1)%8 == 0)
			//printf(" ");

		if((i+1)%line == 0)
			printf("\n");		
	}
	if(len%line != 0)
		printf("\n");
}


void dump(uint8_t * data,uint32_t len)
{
	uint32_t line = 15;
	uint32_t i = 0;
	printf("++++ dump hex[%d] ++++\n",len);
	for(i=0;i<len;i++){
		printf("0x%02x,",data[i]);
		//if((i+1)%8 == 0)
			//printf(" ");

		if((i+1)%line == 0)
			printf("\n");		
	}
	if(len%line != 0)
		printf("\n");
}

void dumpSubject(Sbuject_t *subject)
{
	printf("cn[%d]:%s\n",subject->cn_size,subject->cn);
	printf("province[%d]:%s\n",subject->province_size,subject->province);	
	printf("locality[%d]:%s\n",subject->locality_size,subject->locality);
	printf("origization[%d]:%s\n",subject->origization_size,subject->origization);		
	printf("origizationunitv[%d]:%s\n",subject->origizationunitv_size,subject->origizationunitv);	
	printf("commonname[%d]:%s\n",subject->commonname_size,subject->commonname);			
	printf("emailaddress[%d]:%s\n",subject->emailaddress_size,subject->emailaddress);
}

void dumpKivviCertReq(CertReqTBS_t *tbs)
{
	printf("+++++ dumpCertReq +++++\n");
	printf("[bodysize]%d\n",tbs->bodysize);	
	printf("[version]%02x\n",tbs->version);
	printf("[sigType]%04x\n",tbs->sigType);
	
	printf("[subject]:\n");
	dumpSubject(&(tbs->subject));
	
	printf("[pubkey][%d]:\n",tbs->publicekey_size);
	dump(tbs->publicekey,tbs->publicekey_size);

	printf("[signature][%d]:\n",tbs->signature_size);
	dump(tbs->signature,tbs->signature_size);	
}

void dumpKivviCert(CertTBS_t *tbs)
{
	printf("+++++ dumpCert +++++\n");
	printf("[bodysize]%d\n",tbs->bodysize);	
	printf("[version]%02x\n",tbs->version);
	printf("[sigType]%04x\n",tbs->sigType);

	printf("[serial]:\n");
	dump(tbs->serial,8);
	
	printf("[subject]:\n");
	dumpSubject(&(tbs->subject));

	printf("[utc]:\n");
	printf("before time:%s\n",tbs->beforetime);
	printf("after time:%s\n",tbs->aftertime);
	printf("[issuer]:\n");
	dumpSubject(&(tbs->issuer));	
	printf("[pubkey][%d]:\n",tbs->publicekey_size);
	dump(tbs->publicekey,tbs->publicekey_size);

	printf("[signature][%d]:\n",tbs->signature_size);
	dump(tbs->signature,tbs->signature_size);	
}

void dumpTlv(DerTLV_t* tlv,uint8_t *tag)
{
	printf("%s[%02x][%d]\n",tag,tlv->tag,tlv->tagDataLen);
	dump(tlv->tagData,tlv->tagDataLen);
}

