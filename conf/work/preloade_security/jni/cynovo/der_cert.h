#ifndef __DER_CERT_H__
#define __DER_CERT_H__

#include "util.h"

typedef struct{
	uint8_t cn[64];
	uint32_t cn_size;
	uint8_t province[64];
	uint32_t province_size;	
	uint8_t locality[64];
	uint32_t locality_size;	
	uint8_t origization[64];
	uint32_t origization_size;	
	uint8_t origizationunitv[64];
	uint32_t origizationunitv_size;	
	uint8_t commonname[64];
	uint32_t commonname_size;	
	uint8_t emailaddress[64];	
	uint32_t emailaddress_size;
}Sbuject_t;


typedef struct 
{
	uint32_t sigType;
	uint32_t bodysize;
	
	uint8_t body[2048];
	uint8_t version;
	Sbuject_t subject;
	uint8_t publicekey[320];
	uint32_t publicekey_size;
	uint8_t signature[270];
	uint32_t signature_size;	
}CertReqTBS_t;

typedef struct 
{
	uint32_t sigType;
	uint32_t bodysize;
	
	uint8_t body[2048];
	uint8_t version;
	uint8_t serial[8];
	Sbuject_t subject;
	uint8_t beforetime[16];
	uint8_t aftertime[16];
	Sbuject_t issuer;
	uint8_t publicekey[320];
	uint32_t publicekey_size;
	uint8_t signature[270];
	uint32_t signature_size;	
}CertTBS_t;


#endif

