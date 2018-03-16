#include "wolfssl/wolfcrypt/rsa.h"
#include "wolfssl/wolfcrypt/asn_public.h"
#include "wolfssl/wolfcrypt/asn.h"
#include "wolfssl/ssl.h"


#include "stdio.h"
#include "util.h"
#include "der_cert.h"

uint32_t genCertReq(Cert *req,RsaKey* key,
		uint8_t *reqdata,int reqdatalen)
{
	RNG    rng;
	uint32_t    ret;
	
	ret = wc_InitRng(&rng);
	if(ret!=0)
		return ret;
	
	wc_InitCert(req);
	
	strncpy(req->subject.country, "CN", CTC_NAME_SIZE);
	strncpy(req->subject.state, "jiangsu", CTC_NAME_SIZE);
	strncpy(req->subject.locality, "wuxi", CTC_NAME_SIZE);
	strncpy(req->subject.org, "cynovo", CTC_NAME_SIZE);
	strncpy(req->subject.unit, "cynovo", CTC_NAME_SIZE);
	strncpy(req->subject.commonName, "cynovogroup", CTC_NAME_SIZE);
	strncpy(req->subject.email, "cynovogroup@cynovo.com.cn", CTC_NAME_SIZE);
	req->isCA    = 1;
	req->sigType = CTC_SHA256wRSA;

	/* add Policies */
	strncpy(req->certPolicies[0], "2.16.840.1.101.3.4.1.42",
		CTC_MAX_CERTPOL_SZ);
	strncpy(req->certPolicies[1], "1.2.840.113549.1.9.16.6.5",
		CTC_MAX_CERTPOL_SZ);
	req->certPoliciesNb = 2;

	/* add SKID from the Public Key */
	if (wc_SetSubjectKeyIdFromPublicKey(req, key, NULL) != 0){
		ret = -398;
		goto out;
	}

	/* add Key Usage */
	if (wc_SetKeyUsage(req,"digitalSignature,nonRepudiation,"
                                "keyEncipherment,keyAgreement") != 0){
		ret = -400;
		goto out;
	}

	ret = wc_MakeCertReq(req, reqdata, reqdatalen, key, NULL);
	if(ret<0)
		goto out;
		
	ret = wc_SignCert(req->bodySz, req->sigType, reqdata, reqdatalen,
		key, NULL, &rng);
	if(ret<0)
		return ret;
out:
	wc_FreeRng(&rng);	
	return ret;
}

uint32_t generateSelfSignedCert(Cert* cert,RsaKey* key,
	uint8_t* buffer, uint32_t buffSz,uint32_t daysValid)
{
	uint32_t    ret;
	RNG    rng;
	
	ret = wc_InitRng(&rng);
	if(ret!=0)
		return ret;

	wc_InitCert(cert);
	strncpy(cert->subject.country, "CN", CTC_NAME_SIZE);
	strncpy(cert->subject.state, "jiangsu", CTC_NAME_SIZE);
	strncpy(cert->subject.locality, "wuxi", CTC_NAME_SIZE);
	strncpy(cert->subject.org, "cynovo", CTC_NAME_SIZE);
	strncpy(cert->subject.unit, "cynovo", CTC_NAME_SIZE);
	strncpy(cert->subject.commonName, "cynovogroup", CTC_NAME_SIZE);
	strncpy(cert->subject.email, "cynovogroup@cynovo.com.cn", CTC_NAME_SIZE);
	
	cert->isCA    = 1;
	cert->sigType = CTC_SHA256wRSA;
	cert->daysValid = daysValid;

	strncpy(cert->certPolicies[0], "2.16.840.1.101.3.4.1.42",
			CTC_MAX_CERTPOL_SZ);
	strncpy(cert->certPolicies[1], "1.2.840.113549.1.9.16.6.5",
			CTC_MAX_CERTPOL_SZ);
	cert->certPoliciesNb = 2;
	
	/* add SKID from the Public Key */
	if (wc_SetSubjectKeyIdFromPublicKey(cert, key, NULL) != 0) {
		ret= -398;
		goto out;
	}
	
	 /* add AKID from the Public Key */
	 if (wc_SetAuthKeyIdFromPublicKey(cert, key, NULL) != 0) {
		 ret= -399;
		 goto out;		 
	}
	
	/* add Key Usage */
	if (wc_SetKeyUsage(cert,"cRLSign,keyCertSign") != 0) {
		ret= -400;
		goto out;		
	}

	ret = wc_MakeSelfCert(cert,buffer,buffSz,key,&rng);
out:
	wc_FreeRng(&rng);

	return ret;	
}

uint32_t genCert(CertReqTBS_t *tbs,uint8_t* buffer,uint32_t buffSz,
	RsaKey* key,uint32_t daysValid)
{
	uint32_t    ret;
	RNG    rng;
	Cert cert;
	RsaKey rsaPubKey;
	
	ret = wc_InitRng(&rng);
	if(ret!=0)
		return ret;

	wc_InitCert(&cert);
	cert.sigType = tbs->sigType;
	cert.bodySz = tbs->bodysize;
	cert.isCA    = 0;
	cert.daysValid = daysValid;
	strncpy(cert.subject.country, tbs->subject.cn, tbs->subject.cn_size);
	strncpy(cert.subject.state, tbs->subject.province, tbs->subject.province_size);
	strncpy(cert.subject.locality, tbs->subject.locality, tbs->subject.locality_size);
	strncpy(cert.subject.org, tbs->subject.origization, tbs->subject.origization_size);
	strncpy(cert.subject.unit, tbs->subject.origizationunitv, tbs->subject.origizationunitv_size);
	strncpy(cert.subject.commonName, tbs->subject.commonname, tbs->subject.commonname_size);
	strncpy(cert.subject.email, tbs->subject.emailaddress, tbs->subject.emailaddress_size);

	ret = readRsaPubKeyFromBuffer(&rsaPubKey,tbs->publicekey,tbs->publicekey_size);	
	if(ret!=0)
		goto out;

	strncpy(cert.certPolicies[0], "2.16.840.1.101.3.4.1.42",CTC_MAX_CERTPOL_SZ);
	strncpy(cert.certPolicies[1], "1.2.840.113549.1.9.16.6.5",CTC_MAX_CERTPOL_SZ);
	cert.certPoliciesNb = 2;
	
	if (wc_SetSubjectKeyIdFromPublicKey(&cert, &rsaPubKey, NULL) != 0) {
		ret= -398;
		goto out;
	}
	
	if (wc_SetAuthKeyIdFromPublicKey(&cert, &rsaPubKey, NULL) != 0) {
		 ret= -399;
		 goto out;		 
	}
	
	ret = wc_MakeSelfCert(&cert,buffer,buffSz,key,&rng);

out:
	wc_FreeRng(&rng);

	return ret;		
}

uint32_t signCert(uint32_t bodySz,uint32_t sigType,uint8_t* buffer, uint32_t buffSz,RsaKey* key)
{
	uint32_t    ret;
	RNG	  rng;
					   
	ret = wc_InitRng(&rng);
	if(ret!=0)
		return ret;

	ret =  wc_SignCert(bodySz, sigType,buffer, buffSz, key, NULL, &rng);

out:
	wc_FreeRng(&rng);

	return ret;
}

//CERT_TYPE  CERTREQ_TYPE
uint32_t CertDerToPem(uint8_t * der,uint32_t derlen,uint8_t * buf,uint32_t buflen,int type)
{
	return wc_DerToPem(der, derlen, buf, buflen,type);
}

uint32_t CertPemToDer(uint8_t * pem,uint32_t pemlen,uint8_t * buf,uint32_t buflen,int type)
{
	return wolfSSL_CertPemToDer(pem, pemlen,buf, buflen,type);
}


uint32_t hash256(uint8_t* in,uint32_t len,uint8_t*out)
{
	uint32_t ret;
    Sha256 sha;

	ret = wc_InitSha256(&sha);
    if (ret != 0)
        goto out;

    ret = wc_Sha256Update(&sha, in,len);
    if (ret != 0)
        goto out;
    ret = wc_Sha256Final(&sha, out);
    if (ret != 0)
        goto out;
out:
	ret;
}


uint32_t verifyCertSign(CertTBS_t *tbs,RsaKey *rsakey)
{
	uint32_t ret=0;
	uint8_t sha256[SHA256_DIGEST_SIZE]={0};
	uint8_t encsha256[SHA256_DIGEST_SIZE*2]={0};
	uint32_t encsha256Sz=0;
	uint32_t typeH	 = SHA256h;

	uint8_t* sign;	
	uint32_t signlen = 0;
	
	ret = hash256(tbs->body,tbs->bodysize,sha256);
	if(ret!=0)
		goto out;
	
	encsha256Sz = wc_EncodeSignature(encsha256, sha256, SHA256_DIGEST_SIZE, typeH);
	
	if(tbs->signature[0]==0x0){
		sign = tbs->signature +1;
		signlen = tbs->signature_size -1;
	}
	else{
		sign = tbs->signature;
		signlen = tbs->signature_size ;		
	}
	
	ret = RSAKeyVerify(sign,signlen,encsha256,encsha256Sz,rsakey);
out:
	return ret;	
}
