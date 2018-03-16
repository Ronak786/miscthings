#ifndef __UTIL_H__
#define __UTIL_H__

#define uint32_t unsigned int
#define uint8_t unsigned char


typedef struct 
{
    uint8_t PublicKey[256];
    uint32_t PublicKeyLen;
    uint8_t PrivateKey[256];
    uint32_t PrivateKeyLen;
    uint8_t Module[256];
    uint32_t ModuleLen;
    
    uint8_t exponent1[128];  ///
    uint32_t exponent1Len;
    uint8_t exponent2[128];  ///
    uint32_t exponent2Len;
} AsymKey_t;


#endif

