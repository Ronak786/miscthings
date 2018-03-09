#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <jni.h>
#include <android/log.h>

#include "wolfssl/wolfcrypt/rsa.h"
#include "wolfssl/wolfcrypt/asn_public.h"
#include "wolfssl/wolfcrypt/asn.h"
#include "wolfssl/ssl.h"


#include "cynovo/util.h"
#include "cynovo/der_cert.h"

#include "hlog.h"
#include "kivvissl.h"

uint32_t kivvissl_verifyCert(uint8_t* cacert_data,uint32_t cacertlen,
    uint8_t* usercert_data,uint32_t usercertlen)
{
    int ret = -1;
    CertTBS_t cacert_tbs;
    CertTBS_t usercert_tbs;
	RsaKey capubkey;
	

	ret = Der_To_Cert(cacert_data,cacertlen,CTC_SHA256wRSA,&cacert_tbs);
	if(ret!=0)
		goto out;
	
	ret = Der_To_Cert(usercert_data,usercertlen,CTC_SHA256wRSA,&usercert_tbs);
	if(ret!=0)
		goto out;

	ret = readRsaPubKeyFromBuffer(&capubkey,cacert_tbs.publicekey,cacert_tbs.publicekey_size);
	if(ret!=0)
		goto out;

	ret = verifyCertSign(&usercert_tbs,&capubkey);

	wc_FreeRsaKey(&capubkey);
out:
    return ret;
}


uint32_t kivvissl_generateKey(uint8_t* privkeydata)
{	
	uint8_t  pubkeydata[2048];
	uint32_t pubkeydatalen = 2048;	
	uint32_t privkeydatalen = 2048;	
	uint32_t ret = 0;
	
	ret = genRsaKey(2048,privkeydata,&privkeydatalen,pubkeydata,&pubkeydatalen);
	if(ret==0)
		return privkeydatalen;
	else
		return ret;
}

uint32_t kivvissl_generateSelfCert(
	uint8_t *privkeydata,uint32_t privkeydatalen,
	uint8_t *certdata,uint32_t validays)
{

	uint32_t ret = 0;
	uint32_t certderlen = 2048;
	RsaKey rsakey;
	Cert cert;

	
	ret = readRsaPrivKeyFromBuffer(&rsakey,privkeydata,privkeydatalen);	
	if(ret!=0)
		goto out;
	
	certderlen = generateSelfSignedCert(&cert,&rsakey,certdata,certderlen,validays);
	ret = certderlen;
	wc_FreeRsaKey(&rsakey);	
out:	
	return ret;
}

uint32_t kivvissl_signCrt(
		uint8_t *privkeydata,uint32_t privkeydatalen,
		uint8_t *certdata,uint32_t certdatalen,
		uint8_t *signed_certdata)
{

	uint32_t signedcertderlen = 2048;		
	uint32_t ret = 0;	
	RsaKey carsakey;
	CertTBS_t cert;

	ret = Der_To_Cert(certdata,certdatalen,CTC_SHA256wRSA,&cert);
	if(ret!=0)
		goto out;
	ret = readRsaPrivKeyFromBuffer(&carsakey,privkeydata,privkeydatalen);	
	if(ret!=0)
		goto out;
	

	memcpy(signed_certdata,cert.body,cert.bodysize);
	
	signedcertderlen = signCert(cert.bodysize,cert.sigType,
		signed_certdata,signedcertderlen,&carsakey);
	ret = signedcertderlen;
	
	wc_FreeRsaKey(&carsakey);	
out:
	return ret;	
	
}

uint32_t kivvissl_getPubkeyFromCert(
		uint8_t *certdata,uint32_t certdatalen,
		uint8_t *pubkey)
{
	uint32_t ret = 0;	

	CertTBS_t cert_tbs;
				
	ret = Der_To_Cert(certdata,certdatalen,CTC_SHA256wRSA,&cert_tbs);
	if(ret==0){		
		memcpy(pubkey,cert_tbs.publicekey,cert_tbs.publicekey_size);
		ret = cert_tbs.publicekey_size;
	}

	return ret;
}		

uint32_t kivvissl_pubEncrypt(
		uint8_t *pubkeydata,uint32_t pubkeydatalen,
		uint8_t *plain,uint32_t plainlen,
		uint8_t *encrypt_data)
{

	RsaKey pubkey;
	uint32_t ret = 0;
	uint32_t encrptdatalen = 256;
	
	ret = readRsaPubKeyFromBuffer(&pubkey,pubkeydata,pubkeydatalen);
		
	if(ret!=0)
		goto out;

	encrptdatalen = RSAPublicKeyEncrypt(plain,plainlen,encrypt_data,encrptdatalen,&pubkey);
	if(encrptdatalen>0)
		ret = encrptdatalen;

	wc_FreeRsaKey(&pubkey); 
out:
	return ret;
}


uint32_t kivvissl_privDecrypt(
		uint8_t *privatedata,uint32_t privatedatalen,
		uint8_t *encrypt_data,uint32_t encrypt_datalen,
		uint8_t *plain)
{
	RsaKey privkey;
	uint32_t ret = 0;
	uint32_t plainlen = 256;
	
	ret = readRsaPrivKeyFromBuffer(&privkey,privatedata,privatedatalen);
	if(ret!=0)
		goto out;
		
	plainlen = RSAPrivateKeyDecrypt(encrypt_data,encrypt_datalen,plain,plainlen,&privkey);
	if(plainlen>0)
		ret = plainlen;

	wc_FreeRsaKey(&privkey);	
out:
	return ret;

}



uint32_t kivvissl_privSign(uint8_t *privatedata,
		uint32_t privatedatalen,
		uint8_t *plain,int plainlen,
		uint8_t *signature)
{

	const int typeH    = SHA256h;
	uint8_t  digest[2048];
	uint8_t encSig[2048];
	uint32_t  digest_sz = 0;			
	uint32_t  encSig_Sz = 0;
	uint32_t ret = -1;	
	RsaKey privkey;
	WC_RNG rng;
	
	ret = wc_InitRng(&rng);
	if(ret!=0)
		return ret;
	

	ret = readRsaPrivKeyFromBuffer(&privkey,privatedata,privatedatalen);
	if(ret!=0)
		goto out;
	
	if ((ret = wc_Sha256Hash(plain, plainlen, digest)) == 0) {
		digest_sz = SHA256_DIGEST_SIZE;
	}

	encSig_Sz = wc_EncodeSignature(encSig, digest, digest_sz, typeH);
	ret = wc_RsaSSL_Sign(encSig, encSig_Sz, signature, 512, &privkey, &rng);

	wc_FreeRsaKey(&privkey);		
out:	
	wc_FreeRng(&rng);
	
	return ret;	

}


