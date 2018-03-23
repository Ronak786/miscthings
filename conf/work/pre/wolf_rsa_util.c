
#include "wolfssl/wolfcrypt/rsa.h"
#include "wolfssl/wolfcrypt/asn_public.h"
#include "wolfssl/ssl.h"
#include <wolfssl/internal.h>
#include <wolfssl/error-ssl.h>

#include <string.h>
#include "util.h"
#include "der_rsa.h"

uint32_t RSAKeySign(const uint8_t* plain, uint32_t plainlen,
			uint8_t* sign,uint32_t *signlen, RsaKey* key){
                            
	WC_RNG	rng;
	int ret;
	
    ret = wc_InitRng(&rng);
    if (ret != 0)
        return ret;	
	
	ret = wc_RsaSSL_Sign(plain, plainlen, sign, *signlen, key, &rng);
	
    if (ret > 0) { 
        *signlen = ret;
        ret = 0;
    }		
out:
	wc_FreeRng(&rng);
	return ret;
}

uint32_t RSAKeyVerify(const uint8_t* sign, uint32_t signlen,
				uint8_t* plain,uint32_t plainlen, RsaKey* key){  
    uint32_t ret = VerifyRsaSign(sign, signlen, plain, plainlen,key);		
	return ret;
}


uint32_t RSAPublicKeyEncrypt(const uint8_t* in, uint32_t inLen,
	uint8_t* out, uint32_t outLen,RsaKey* key)
{
	WC_RNG	rng;
	uint32_t ret;
	
    ret = wc_InitRng(&rng);
    if (ret != 0)
        return ret;	
	ret=wc_RsaPublicEncrypt(in,inLen,out,outLen,key,&rng);
out:
	wc_FreeRng(&rng);
	return ret;

}
uint32_t RSAPrivateKeyDecrypt(const uint8_t* in, uint32_t inLen,
	uint8_t* out, uint32_t outLen,RsaKey* key)
{
	return wc_RsaPrivateDecrypt(in,inLen,out,outLen,key);	
}

uint32_t RSAPrivateKeyEncrypt(const uint8_t* in, uint32_t inLen,
	uint8_t* out, uint32_t outLen,RsaKey* key)
{
	int len = outLen;
	int ret = RSAKeySign(in, inLen,out,&len,key);
	
	if(ret ==0)
		return len;

	return ret;
}

uint32_t RSAPublicKeyDecrypt(const uint8_t* in, uint8_t inlen,
				  uint8_t* out,uint8_t outlen,RsaKey* key)
{
    #ifdef WOLFSSL_SMALL_STACK
        byte* verifySig = NULL;
    #else
        byte verifySig[ENCRYPT_LEN];
    #endif
    byte* outdata = NULL;  /* inline result */
    int   ret;

    if (in == NULL || out == NULL || key == NULL) {
        return -1;
    }

    if (inlen > ENCRYPT_LEN) {
        return -2;
    }

    #ifdef WOLFSSL_SMALL_STACK
        verifySig = (byte*)XMALLOC(ENCRYPT_LEN, NULL,
                                   DYNAMIC_TYPE_SIGNATURE);
        if (verifySig == NULL)
            return MEMORY_ERROR;
    #endif

    XMEMCPY(verifySig, in, inlen);
    ret = wc_RsaSSL_VerifyInline(verifySig, inlen, &outdata, key);
	if(ret>0){
		memcpy(out,outdata,ret);
	}

    #ifdef WOLFSSL_SMALL_STACK
        XFREE(verifySig, NULL, DYNAMIC_TYPE_SIGNATURE);
    #endif

    return ret;
}


uint32_t RSAKeyToPrivDer(RsaKey* genKey,uint8_t * buf,uint32_t buflen)
{
	return wc_RsaKeyToDer(genKey, buf, buflen);
}
uint32_t RSAKeyToPubDer(RsaKey* genKey,uint8_t * buf,uint32_t buflen)
{
	return wc_RsaKeyToPublicDer(genKey, buf, buflen);
}

uint32_t PrivKeyDerToPem(uint8_t * der,uint32_t derlen,uint8_t * buf,uint32_t buflen)
{
	return wc_DerToPem(der, derlen, buf, buflen,PRIVATEKEY_TYPE);
}


uint32_t readRsaPrivKeyFromBuffer(RsaKey* privKey,uint8_t *keybuf,uint32_t buflen)
{
	uint32_t idx = 0;
	return  wc_RsaPrivateKeyDecode(keybuf, &idx, privKey,buflen);
}
uint32_t readRsaPubKeyFromBuffer(RsaKey* pubKey,uint8_t *keybuf,uint32_t buflen){
	uint32_t idx = 0;
	return  wc_RsaPublicKeyDecode(keybuf, &idx, pubKey,buflen);
}

uint32_t AsymKeyToWolfKey(AsymKey_t* asymkey,RsaKey* rsakey)
{
	unsigned char  derdata[2048];
	const uint32_t rsabits = 2048;
	uint32_t ret = 0;
	DerKey_t derkey;
	mp_int d, p, q,dp,dq,u,tmp1,tmp2;
	
	mp_init_multi(&p, &q, &d, &dp, &dq, &u);
	mp_init_multi(&tmp1, &tmp2, NULL, NULL, NULL, NULL);

	Init_DerKey(&derkey);
	derkey.keysize = rsabits;
	
	memcpy(derkey.modulus,asymkey->Module,asymkey->ModuleLen);
	derkey.moduluslen = asymkey->ModuleLen;

	memcpy(derkey.privateExponent,asymkey->PrivateKey,asymkey->PrivateKeyLen);
	derkey.privateExponentlen= asymkey->PrivateKeyLen;

	memcpy(derkey.prime1,asymkey->exponent1,asymkey->exponent1Len);
	derkey.prime1len= asymkey->exponent1Len;

	memcpy(derkey.prime2,asymkey->exponent2,asymkey->exponent2Len);
	derkey.prime2len= asymkey->exponent2Len;
	
	revertUint32(derkey.modulus,derkey.moduluslen);
	revertUint32(derkey.privateExponent,derkey.privateExponentlen);
	revertUint32(derkey.prime1,derkey.prime1len);
	revertUint32(derkey.prime2,derkey.prime2len);
	
	revertBuf(derkey.modulus,derkey.moduluslen);
	revertBuf(derkey.privateExponent,derkey.privateExponentlen);
	revertBuf(derkey.prime1,derkey.prime1len);;
	revertBuf(derkey.prime2,derkey.prime2len);
	
	memcpy(derkey.publicExponent,"\x01\x00\x01",3);
	derkey.publicExponentlen= 3;

	ret = mp_read_unsigned_bin(&d, asymkey->PrivateKey,asymkey->PrivateKeyLen);
	if(ret!=0)
		goto out;		

	ret = mp_read_unsigned_bin(&p, asymkey->exponent1, asymkey->exponent1Len);	
	if(ret!=0)
		goto out;		

	ret = mp_read_unsigned_bin(&q,asymkey->exponent2, asymkey->exponent2Len);	
	if(ret!=0)
		goto out;	
	
	ret = mp_sub_d(&p, 1, &tmp1);
	if(ret !=0) 
		goto out;

	ret = mp_sub_d(&q, 1, &tmp2);
	if(ret !=0) 
		goto out;	

	ret = mp_mod(&d, &tmp1, &dp);
	if(ret !=0) 
		goto out;	

	ret = mp_mod(&d, &tmp2, &dq);
	if(ret !=0) 
		goto out;	

	ret = mp_invmod(&q, &p, &u);
	if(ret !=0) 
		goto out;	
	
	ret = mp_to_unsigned_bin(&dp,derkey.exponent1);
	if(ret !=0) 
		goto out;	
	derkey.exponent1len = (derkey.keysize)/16;
	
	ret = mp_to_unsigned_bin(&dq,derkey.exponent2);
	if(ret !=0) 
		goto out;	
	derkey.exponent2len = (derkey.keysize)/16;
	
	ret = mp_to_unsigned_bin(&u,derkey.coefficient);
	if(ret !=0) 
		goto out;	
	derkey.coefficientlen= (derkey.keysize)/16;
#if 0
	printf("dp\n\r");		
	dump(derkey.exponent1, derkey.exponent1len);
	printf("dq\n\r");			
	dump(derkey.exponent2, derkey.exponent2len);
	printf("u\n\r");				
	dump(derkey.coefficient, derkey.coefficientlen);
#endif	
	ret = DerKey_To_Der(derdata,&derkey);
	printf("DerKey_To_Der ret = %d\n",ret);
	if(ret<=0){
		ret = -1;
		goto out;
	}
	ret = readRsaPrivKeyFromBuffer(rsakey,derdata,ret); 
	if(ret !=0) 
		goto out;	
out:
	return ret;

}

uint32_t WolfKeyToAsymKey(RsaKey* rsakey,AsymKey_t* asymkey)
{
	uint32_t ret = 0;

	asymkey->ModuleLen = 256;
	ret = mp_to_unsigned_bin(&(rsakey->n),asymkey->Module);
	if(ret!=0)
		goto out;

	asymkey->PrivateKeyLen = 256;	
	ret = mp_to_unsigned_bin(&(rsakey->d),asymkey->PrivateKey);
	if(ret!=0)
		goto out;

	asymkey->exponent1Len = 128;	
	ret = mp_to_unsigned_bin(&(rsakey->p),asymkey->exponent1);
	if(ret!=0)
		goto out;

	asymkey->exponent2Len = 128;	
	ret = mp_to_unsigned_bin(&(rsakey->q),asymkey->exponent2);
	if(ret!=0)
		goto out;

	revertBuf(asymkey->Module,asymkey->ModuleLen);
	revertBuf(asymkey->PrivateKey,asymkey->PrivateKeyLen);
	revertBuf(asymkey->exponent1,asymkey->exponent1Len);
	revertBuf(asymkey->exponent2,asymkey->exponent2Len);
	revertUint32(asymkey->Module,asymkey->ModuleLen);
	revertUint32(asymkey->PrivateKey,asymkey->PrivateKeyLen);
	revertUint32(asymkey->exponent1,asymkey->exponent1Len);
	revertUint32(asymkey->exponent2,asymkey->exponent2Len);	

	asymkey->PublicKeyLen = 4;	
	memcpy(asymkey->PublicKey,"\x00\x01\x00\x01",4);
	if(ret!=0)
		goto out;	
out:
	return ret;
}

uint32_t genRsaKey(int rsabits,
	uint8_t* privkeybuf,uint32_t *privkeybuflen,
	uint8_t* pubkeybuf,uint32_t *pubkeybuflen){
	
	uint8_t der[2048];
	uint8_t pem[2048];
	
	RsaKey privKey;
	RNG    rng;
	uint32_t    ret;
	
	ret = wc_InitRng(&rng);
	if(ret!=0)
		return ret;
	
	ret = wc_InitRsaKey(&privKey, 0);
	if(ret!=0)
		goto out;

	ret = wc_MakeRsaKey(&privKey, rsabits, 65537, &rng);
	if (ret !=0)
		goto out;

	*privkeybuflen = RSAKeyToPrivDer(&privKey,privkeybuf,*privkeybuflen);	
	if(*privkeybuflen<0){
		ret = -2;
		goto outfree;
	}
	*pubkeybuflen = RSAKeyToPubDer(&privKey,pubkeybuf,*pubkeybuflen);	
	if(*pubkeybuflen<0){
		ret = -2;
		goto outfree;
	}	
	
outfree:
	wc_FreeRsaKey(&privKey);	
out:
	wc_FreeRng(&rng);
	
	return ret;
}

