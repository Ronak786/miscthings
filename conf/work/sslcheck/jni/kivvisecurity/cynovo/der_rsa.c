#include <stdlib.h>

#include <stdint.h>
#include <string.h>

#include "der_rsa.h"
#include "der.h"



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



void  Init_DerKey(DerKey_t *rsakey){

	memset(rsakey->version,0,sizeof(rsakey->version));
	memset(rsakey->modulus,0,sizeof(rsakey->modulus));
	memset(rsakey->publicExponent,0,sizeof(rsakey->publicExponent));
	memset(rsakey->privateExponent,0,sizeof(rsakey->privateExponent));
	memset(rsakey->prime1,0,sizeof(rsakey->prime1));
	memset(rsakey->prime2,0,sizeof(rsakey->prime2));
	memset(rsakey->exponent1,0,sizeof(rsakey->exponent1));
	memset(rsakey->exponent2,0,sizeof(rsakey->exponent2));
	memset(rsakey->coefficient,0,sizeof(rsakey->coefficient));

	rsakey->keysize = 0;
	rsakey->versionlen = 1;
	rsakey->moduluslen = 0;
	rsakey->publicExponentlen = 0;
	rsakey->privateExponentlen = 0;
	rsakey->prime1len = 0;
	rsakey->prime2len = 0;
	rsakey->exponent1len = 0;
	rsakey->exponent2len = 0;
	rsakey->coefficientlen = 0;
}



uint32_t  DerKey_To_Der(uint8_t *derkeydata,DerKey_t *rsakey){

	uint32_t datalen = 0;
	uint32_t i=0;

	
	DER_SetTLV(derkeydata+datalen, INTERGER,rsakey->version,rsakey->versionlen);
	datalen = datalen + 1 + TlvLengthSize(rsakey->versionlen) + rsakey->versionlen;

	DER_SetTLV(derkeydata+datalen, INTERGER,rsakey->modulus,rsakey->moduluslen);
	datalen = datalen + 1 + TlvLengthSize(rsakey->moduluslen) + rsakey->moduluslen;

	DER_SetTLV(derkeydata+datalen, INTERGER,rsakey->publicExponent,rsakey->publicExponentlen);
	datalen = datalen + 1 + TlvLengthSize(rsakey->publicExponentlen) + rsakey->publicExponentlen;

	DER_SetTLV(derkeydata+datalen, INTERGER,rsakey->privateExponent,rsakey->privateExponentlen);
	datalen = datalen + 1 + TlvLengthSize(rsakey->privateExponentlen) + rsakey->privateExponentlen;

	DER_SetTLV(derkeydata+datalen, INTERGER,rsakey->prime1,rsakey->prime1len);
	datalen = datalen + 1 + TlvLengthSize(rsakey->prime1len) + rsakey->prime1len;

	DER_SetTLV(derkeydata+datalen, INTERGER,rsakey->prime2,rsakey->prime2len);
	datalen = datalen + 1 + TlvLengthSize(rsakey->prime2len) + rsakey->prime2len;

	DER_SetTLV(derkeydata+datalen, INTERGER,rsakey->exponent1,rsakey->exponent1len);
	datalen = datalen + 1 + TlvLengthSize(rsakey->exponent1len) + rsakey->exponent1len;

	DER_SetTLV(derkeydata+datalen, INTERGER,rsakey->exponent2,rsakey->exponent2len);
	datalen = datalen + 1 + TlvLengthSize(rsakey->exponent2len) + rsakey->exponent2len;

	DER_SetTLV(derkeydata+datalen, INTERGER,rsakey->coefficient,rsakey->coefficientlen);
	datalen = datalen + 1 + TlvLengthSize(rsakey->coefficientlen) + rsakey->coefficientlen;		
	
	for(i=datalen+4;i>=4;i--)
		derkeydata[i]=derkeydata[i-4];

	derkeydata[0] = SEQUENCE_APP;
	derkeydata[1] = 0x82;
	derkeydata[2] = datalen>>8;;
	derkeydata[3] = datalen;

	datalen = datalen +4;
	derkeydata[datalen] = 0;

	return datalen;	

}

