#include "stdio.h"

#include <stdlib.h>
#include <string.h>

#include "wolfssl/wolfcrypt/rsa.h"
#include "wolfssl/wolfcrypt/asn_public.h"
#include "wolfssl/ssl.h"

#include "der_cert.h"
#include "der.h"
#include "hlog.h"


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
			//hlog(H_DEBUG," ");

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
			//hlog(H_DEBUG," ");

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
	hlog(H_DEBUG,"++++ dump hex[%d] ++++\n",len);
	for(i=0;i<len;i++){
		hlog(H_DEBUG,"0x%02x,",data[i]);
		//if((i+1)%8 == 0)
			//hlog(H_DEBUG," ");

		if((i+1)%line == 0)
			hlog(H_DEBUG,"\n");		
	}
	if(len%line != 0)
		hlog(H_DEBUG,"\n");
}

void dumpSubject(Sbuject_t *subject)
{
	hlog(H_DEBUG,"cn[%d]:%s\n",subject->cn_size,subject->cn);
	hlog(H_DEBUG,"province[%d]:%s\n",subject->province_size,subject->province);	
	hlog(H_DEBUG,"locality[%d]:%s\n",subject->locality_size,subject->locality);
	hlog(H_DEBUG,"origization[%d]:%s\n",subject->origization_size,subject->origization);		
	hlog(H_DEBUG,"origizationunitv[%d]:%s\n",subject->origizationunitv_size,subject->origizationunitv);	
	hlog(H_DEBUG,"commonname[%d]:%s\n",subject->commonname_size,subject->commonname);			
	hlog(H_DEBUG,"emailaddress[%d]:%s\n",subject->emailaddress_size,subject->emailaddress);
}

void dumpKivviCertReq(CertReqTBS_t *tbs)
{
	hlog(H_DEBUG,"+++++ dumpCertReq +++++\n");
	hlog(H_DEBUG,"[bodysize]%d\n",tbs->bodysize);	
	hlog(H_DEBUG,"[version]%02x\n",tbs->version);
	hlog(H_DEBUG,"[sigType]%04x\n",tbs->sigType);
	
	hlog(H_DEBUG,"[subject]:\n");
	dumpSubject(&(tbs->subject));
	
	hlog(H_DEBUG,"[pubkey][%d]:\n",tbs->publicekey_size);
	dump(tbs->publicekey,tbs->publicekey_size);

	hlog(H_DEBUG,"[signature][%d]:\n",tbs->signature_size);
	dump(tbs->signature,tbs->signature_size);	
}

void dumpKivviCert(CertTBS_t *tbs)
{
	hlog(H_DEBUG,"+++++ dumpCert +++++\n");
	hlog(H_DEBUG,"[bodysize]%d\n",tbs->bodysize);	
	hlog(H_DEBUG,"[version]%02x\n",tbs->version);
	hlog(H_DEBUG,"[sigType]%04x\n",tbs->sigType);

	hlog(H_DEBUG,"[serial]:\n");
	dump(tbs->serial,8);
	
	hlog(H_DEBUG,"[subject]:\n");
	dumpSubject(&(tbs->subject));

	hlog(H_DEBUG,"[utc]:\n");
	hlog(H_DEBUG,"before time:%s\n",tbs->beforetime);
	hlog(H_DEBUG,"after time:%s\n",tbs->aftertime);
	hlog(H_DEBUG,"[issuer]:\n");
	dumpSubject(&(tbs->issuer));	
	hlog(H_DEBUG,"[pubkey][%d]:\n",tbs->publicekey_size);
	dump(tbs->publicekey,tbs->publicekey_size);

	hlog(H_DEBUG,"[signature][%d]:\n",tbs->signature_size);
	dump(tbs->signature,tbs->signature_size);	
}

void dumpTlv(DerTLV_t* tlv,uint8_t *tag)
{
	hlog(H_DEBUG,"%s[%02x][%d]\n",tag,tlv->tag,tlv->tagDataLen);
	dump(tlv->tagData,tlv->tagDataLen);
}

