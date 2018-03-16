#ifndef __DER_RSA_H__
#define __DER_RSA_H__

#include "util.h"

typedef struct 

{
	uint32_t keysize;
   uint8_t  version[8];
   uint32_t versionlen;
   uint8_t modulus[257];
   uint32_t moduluslen;
	uint8_t publicExponent[8];
   uint32_t publicExponentlen;
	uint8_t privateExponent[257];
   uint32_t privateExponentlen;
	uint8_t prime1[130];
	uint32_t prime1len;
	uint8_t prime2[130];
   uint32_t prime2len;
	uint8_t exponent1[130];
   uint32_t exponent1len;
	uint8_t exponent2[130];
   uint32_t exponent2len;
	uint8_t coefficient[130];
   uint32_t coefficientlen;	
}DerKey_t;



#endif


