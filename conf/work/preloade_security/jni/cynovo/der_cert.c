
#include "wolfssl/wolfcrypt/asn_public.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "der.h"
#include "der_cert.h"

static uint32_t TlvLengthSize(uint32_t length){
	if(length < 0x80)
		return 1;
	else if(length <= 0xff)
		return 2;
	else if(length <= 0xffff)
		return 3;
	else if(length <= 0xffffff)
		return 4;
	return 0;
}

static uint8_t SetTlvLength(uint8_t *pBuf, uint32_t length)
{
    uint8_t *p = pBuf;
	uint8_t lensize = 0;
	
    if(length < 0x80)
    {
        *p = length;
		lensize = 1;
    }
    else if(length <= 0xff)
    {
        p[0] = 0x81;
        p[1] = length;
		lensize = 2;
    }
    else if(length <= 0xffff)
    {
        p[0] = 0x82;
        p[1] = length>>8;
        p[2] = length;
		lensize = 3;		
    }
    else if(length <= 0xffffff)
    {
        p[0] = 0x83;
        p[1] = length >>16;
        p[2] = length >>8;
        p[3] = length;
		lensize = 4;		
    }
	return lensize;
} 

static int TlvToBuf(uint8_t *pBuf,DerTLV_t *tlv){
	int size = 0;
	pBuf[0] = tlv->tag;
	size = 1;
	size += SetTlvLength(pBuf+1,tlv->tagDataLen);
	memcpy(pBuf+size,tlv->tagData,tlv->tagDataLen);
	size += tlv->tagDataLen;
	return size;
}


uint32_t Analysis_UTC(uint8_t * beforetime,uint8_t *aftertime,uint8_t *derdata,uint32_t derdatalen)
{
	uint32_t ret = -1;	
	DerTLV_t tlvtemp;	

	ret = DER_ReadTLV(derdata, derdatalen,0,&tlvtemp);
	if(ret!=0 || tlvtemp.tagDataLen!=15){
		ret = -1;
		goto out;
	}
	beforetime[15]=0;	
	memcpy(beforetime,tlvtemp.tagData,tlvtemp.tagDataLen);
	
	ret = DER_ReadTLV(derdata, derdatalen,1,&tlvtemp);
	if(ret!=0 || tlvtemp.tagDataLen!=15){
		ret = -1;
		goto out;
	}
	aftertime[15]=0;	
	memcpy(aftertime,tlvtemp.tagData,tlvtemp.tagDataLen);

out:
	return ret;	
}

uint32_t Analysis_SubjectSection(uint8_t *derdata,uint32_t derdatalen,
		int id,uint8_t *section)
{
	uint32_t ret = -1;	
	DerTLV_t tlvtemp;
	uint32_t templen;
	uint8_t* tempdata;	

	templen = derdatalen;
	tempdata = derdata;
	ret = DER_ReadTLV(tempdata, templen,id,&tlvtemp);
	if(ret!=0)
		goto out;

	tempdata = tlvtemp.tagData;
	templen = tlvtemp.tagDataLen;
	ret = DER_ReadTLV(tempdata, templen,0,&tlvtemp);
	if(ret!=0)
		goto out;

	tempdata = tlvtemp.tagData;
	templen = tlvtemp.tagDataLen;
	ret = DER_ReadTLV(tempdata, templen,1,&tlvtemp);
	if(ret!=0)
		goto out;	

	ret = tlvtemp.tagDataLen;
	memcpy(section,tlvtemp.tagData,ret);
	section[ret]=0;
out:
	return ret;
}

uint32_t Analysis_Subject(Sbuject_t* subject,uint8_t *derdata,uint32_t derdatalen)
{	
	subject->cn_size= 
		Analysis_SubjectSection(derdata,derdatalen,0,subject->cn);
	if(subject->cn_size<0)
		return -1;
	
	subject->province_size= 
		Analysis_SubjectSection(derdata,derdatalen,1,subject->province);
	if(subject->province_size<0)
		return -1;
	
	subject->locality_size= 
		Analysis_SubjectSection(derdata,derdatalen,2,subject->locality);
	if(subject->locality_size<0)
		return -1;
	
	subject->origization_size= 
		Analysis_SubjectSection(derdata,derdatalen,3,subject->origization);
	if(subject->origization_size<0)
		return -1;
	
	subject->origizationunitv_size= 
		Analysis_SubjectSection(derdata,derdatalen,4,subject->origizationunitv);
	if(subject->origizationunitv_size<0)
		return -1;
	
	subject->commonname_size= 
		Analysis_SubjectSection(derdata,derdatalen,5,subject->commonname);
	if(subject->commonname_size<0)
		return -1;
	
	subject->emailaddress_size= 
		Analysis_SubjectSection(derdata,derdatalen,5,subject->emailaddress);
	if(subject->emailaddress_size<0)
		return -1;
	return 0;
}

uint32_t Analysis_Tbs(CertTBS_t* tbs){
	uint32_t ret = -1,i;	
	
	uint32_t templen;
	uint8_t* tempdata;	
	DerTLV_t tlvtemp;
	DerTLV_t tlvtempa;

	templen = tbs->bodysize;
	tempdata = tbs->body;
	
	ret = DER_ReadTLV(tempdata, templen,0,&tlvtemp);
	if(ret!=0)
		goto out;
	else{
		tempdata = tlvtemp.tagData;
		templen = tlvtemp.tagDataLen;	
		
		//// version ////
		ret = DER_ReadTLV(tempdata, templen,0,&tlvtempa);
		if(ret!=0)
			goto out;
		tbs->version = tlvtempa.tagData[0];

		//// serial ////
		ret = DER_ReadTLV(tempdata, templen,1,&tlvtempa);
		if(ret!=0 || tlvtempa.tagDataLen!=8){
			ret = -1;
			goto out;
		}
		memcpy(tbs->serial,tlvtempa.tagData,tlvtempa.tagDataLen);
		
		//// subject info ////
		ret = DER_ReadTLV(tempdata, templen,3,&tlvtempa);
		if(ret!=0)
			goto out;		
		ret = Analysis_Subject(&(tbs->subject),tlvtempa.tagData,tlvtempa.tagDataLen);
		if(ret!=0)
			goto out;

		//// UTC info ////
		ret = DER_ReadTLV(tempdata, templen,4,&tlvtempa);
		if(ret!=0)
			goto out;		
		ret = Analysis_UTC(tbs->beforetime,tbs->aftertime,tlvtempa.tagData,tlvtempa.tagDataLen);
		if(ret!=0)
			goto out;		

		//// subject info ////
		ret = DER_ReadTLV(tempdata, templen,5,&tlvtempa);
		if(ret!=0)
			goto out;		
		ret = Analysis_Subject(&(tbs->issuer),tlvtempa.tagData,tlvtempa.tagDataLen);
		if(ret!=0)
			goto out;		
		
		//// pubkey ////
		ret = DER_ReadTLV(tempdata, templen,6,&tlvtempa);
		if(ret!=0)
			goto out;	
		tbs->publicekey_size = TlvToBuf(tbs->publicekey,&tlvtempa);

	}
	ret = 0;
out:
	return ret;	
}


uint32_t Analysis_ReqTbs(CertReqTBS_t* tbs){
	uint32_t ret = -1,i;	
	uint32_t templen;
	uint8_t* tempdata;	
	DerTLV_t tlvtemp;
	DerTLV_t tlvtempa;

	templen = tbs->bodysize;
	tempdata = tbs->body;
	
	ret = DER_ReadTLV(tempdata, templen,0,&tlvtemp);
	if(ret!=0)
		goto out;
	else{
		tempdata = tlvtemp.tagData;
		templen = tlvtemp.tagDataLen;	
		
		//// version ////
		ret = DER_ReadTLV(tempdata, templen,0,&tlvtempa);
		if(ret!=0)
			goto out;
		tbs->version = tlvtempa.tagData[0];
		
		//// subject info ////
		ret = DER_ReadTLV(tempdata, templen,1,&tlvtempa);
		if(ret!=0)
			goto out;		
		ret = Analysis_Subject(&(tbs->subject),tlvtempa.tagData,tlvtempa.tagDataLen);
		if(ret!=0)
			goto out;

		//// pubkey ////
		ret = DER_ReadTLV(tempdata, templen,2,&tlvtempa);
		if(ret!=0)
			goto out;	
		tbs->publicekey_size = TlvToBuf(tbs->publicekey,&tlvtempa);
	}
out:
	return ret;	
}
uint32_t Der_To_Cert(uint8_t *derdata,uint32_t derdatalen,int sigType,CertTBS_t *tbs){
	uint32_t ret = -1,i;
	uint32_t templen;
	uint8_t* tempdata;

	DerTLV_t tlvtemp;
	DerTLV_t CertTBS;

	ret = DER_ReadTLV(derdata, derdatalen,0,&tlvtemp);
	if(ret!=0)
		goto out;
	else{
		tempdata = tlvtemp.tagData;
		templen = tlvtemp.tagDataLen;
		
		ret = DER_ReadTLV(tempdata,templen,0,&CertTBS);
		if(ret!=0)
			goto out;	
		
		tbs->bodysize = TlvToBuf(tbs->body,&CertTBS);
		
		ret = Analysis_Tbs(tbs);
		if(ret!=0)
			goto out;			
		
		ret = DER_ReadTLV(tempdata, templen,2,&tlvtemp);	
		if(ret!=0)
			goto out;
		tbs->signature_size = tlvtemp.tagDataLen;		
		memcpy(tbs->signature,tlvtemp.tagData,tbs->signature_size);		
		tbs->sigType = sigType;
	}
out:
	return ret;	
}
uint32_t  Der_To_Req(uint8_t *derdata,uint32_t derdatalen,int sigType,CertReqTBS_t *tbs){
	uint32_t ret = -1,i;
	uint32_t templen;
	uint8_t* tempdata;

	DerTLV_t tlvtemp;
	DerTLV_t ReqTBS;
		
	ret = DER_ReadTLV(derdata, derdatalen,0,&tlvtemp);
	if(ret!=0)
		goto out;
	else{
		tempdata = tlvtemp.tagData;
		templen = tlvtemp.tagDataLen;
		
		ret = DER_ReadTLV(tempdata,templen,0,&ReqTBS);
		if(ret!=0)
			goto out;	
		
		tbs->bodysize = TlvToBuf(tbs->body,&ReqTBS);
		
		ret = Analysis_ReqTbs(tbs);
		if(ret!=0)
			goto out;			
		
		ret = DER_ReadTLV(tempdata, templen,2,&tlvtemp);	
		if(ret!=0)
			goto out;
		tbs->signature_size = tlvtemp.tagDataLen;		
		memcpy(tbs->signature,tlvtemp.tagData,tbs->signature_size);		
		tbs->sigType = sigType;
	}
	
out:
	return ret;
}

void testReadCert(CertTBS_t *tbs,const *filepath)
{
	uint32_t ret = 0;
	uint8_t derkeydata[2048];
	uint32_t derkeydatalen=sizeof(derkeydata);	
	
	uint8_t base[2048] = {0};
	uint32_t baselen = sizeof(base);

	baselen = readFile(filepath,base);
	derkeydatalen =  CertPemToDer(base,baselen,derkeydata,derkeydatalen,CERT_TYPE);
	printf("der len [%d]\n",derkeydatalen);
	
	ret = Der_To_Cert(derkeydata,derkeydatalen,CTC_SHA256wRSA,tbs);
	printf("Convert Der_To_Cert ret = %d \n",ret);	
}


void testReadReq(CertReqTBS_t *tbs,const *filepath)
{
	uint32_t ret = 0;
	uint8_t derkeydata[2048];
	uint32_t derkeydatalen=sizeof(derkeydata);	
	
	uint8_t base[2048] = {0};
	uint32_t baselen = sizeof(base);

	baselen = readFile(filepath,base);
	derkeydatalen =  CertPemToDer(base,baselen,derkeydata,derkeydatalen,CERTREQ_TYPE);
	printf("der len [%d]\n",derkeydatalen);
	
	ret = Der_To_Req(derkeydata,derkeydatalen,CTC_SHA256wRSA,tbs);
	printf("Convert Der_To_CertReq ret = %d \n",ret);
	
}

