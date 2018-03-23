
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

uint32_t checkCrtSignMem(const uint8_t *cacrtpath, uint32_t ca_sz, const uint8_t *usercrtpath, uint32_t user_sz)
{
    const uint8_t  *cacertder = cacrtpath;
    uint32_t cacertderlen;
    const uint8_t  *usercertder = usercrtpath;
    uint32_t usercertderlen;
    uint32_t ret = 0;   
    CertTBS_t cacert;
    CertTBS_t usercert;
    RsaKey capubkey;

    cacertderlen = ca_sz;
    usercertderlen = user_sz;
    
    ret = Der_To_Cert(cacertder,cacertderlen,CTC_SHA256wRSA,&cacert);   
    ret = Der_To_Cert(usercertder,usercertderlen,CTC_SHA256wRSA,&usercert);
    
    ret = readRsaPubKeyFromBuffer(&capubkey,cacert.publicekey,cacert.publicekey_size);
    
    printf("checkCrtSign readRsaPubKeyFromBuffer = %d\n",ret);
    
    ret = verifyCertSign(&usercert,&capubkey);
    printf("checkCrtSign verifyCertSign = %d\n",ret);
    
    wc_FreeRsaKey(&capubkey);   

    return ret;
}

void dumpX(uint8_t * data,uint32_t len,uint32_t oneline)
{
    uint32_t line = oneline;
    uint32_t i = 0;
    printf("++++ dump hex[%d] ++++\n",len);
    for(i=0;i<len;i++){
        printf("0x%x,",data[i]);

        if((i+1)%line == 0)
            printf("\n");    
    }   
    if(len%line != 0)
        printf("\n");
}

