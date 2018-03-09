#include "wolfssl/wolfcrypt/rsa.h"
#include "wolfssl/wolfcrypt/asn_public.h"
#include "wolfssl/ssl.h"

#include "stdio.h"
#include "cynovo/util.h"
#include "cynovo/der_cert.h"

uint32_t generateCAKey()
{
	uint8_t  privkeydata[2048];
	uint32_t privkeydatalen = 2048;
	uint8_t  pubkeydata[2048];
	uint32_t pubkeydatalen = 2048;		
	uint32_t ret = 0;
	
	ret = genRsaKey(2048,privkeydata,&privkeydatalen,pubkeydata,&pubkeydatalen);
	printf("CA privkey\n");
	dump(privkeydata,privkeydatalen);
	printf("CA privkey\n");
	dump(pubkeydata,pubkeydatalen);
	
	if(ret==0){ 
		ret = writeFile("/mnt/sdcard/cert/ca_privkey.der",privkeydata,privkeydatalen);
		ret = writeFile("/mnt/sdcard/cert/ca_pubkey.der",pubkeydata,pubkeydatalen);
	}
	return ret;
}

uint32_t generateKLDKey()
{
	uint8_t  privkeydata[2048];
	uint32_t privkeydatalen = 2048;
	uint8_t  pubkeydata[2048];
	uint32_t pubkeydatalen = 2048;		
	uint32_t ret = 0;
	
	ret = genRsaKey(2048,privkeydata,&privkeydatalen,pubkeydata,&pubkeydatalen);
	printf("KLD privkey\n");
	dump(privkeydata,privkeydatalen);
	printf("KLD privkey\n");
	dump(pubkeydata,pubkeydatalen);
	
	if(ret==0){ 
		ret = writeFile("/mnt/sdcard/cert/kld_privkey.der",privkeydata,privkeydatalen);
		ret = writeFile("/mnt/sdcard/cert/kld_pubkey.der",pubkeydata,pubkeydatalen);
	}
	return ret;
}

uint32_t generateTermialKey()
{
	uint8_t  privkeydata[2048];
	uint32_t privkeydatalen = 2048;
	uint8_t  pubkeydata[2048];
	uint32_t pubkeydatalen = 2048;		
	uint32_t ret = 0;
	
	ret = genRsaKey(2048,privkeydata,&privkeydatalen,pubkeydata,&pubkeydatalen);
	printf("Termial privkey\n");
	dump(privkeydata,privkeydatalen);
	printf("Termial privkey\n");
	dump(pubkeydata,pubkeydatalen);	
	if(ret==0){ 
		ret = writeFile("/mnt/sdcard/cert/terminal_privkey.der",privkeydata,privkeydatalen);
		ret = writeFile("/mnt/sdcard/cert/terminal_pubkey.der",pubkeydata,pubkeydatalen);
	}
	return ret;
}


uint32_t generateSelfCert(const uint8_t *privkeypath,
	const uint8_t *certpath)
{
	uint8_t  privkey[2048];
	uint32_t privkeylen = 2048;
	uint8_t  certder[2048];
	uint32_t certderlen = 2048;		
	uint32_t ret = 0;	
	RsaKey rsakey;
	Cert cert;

	privkeylen = readFile(privkeypath,privkey);
	
	ret = readRsaPrivKeyFromBuffer(&rsakey,privkey,privkeylen);	
	
	certderlen = generateSelfSignedCert(&cert,&rsakey,certder,certderlen,30);
	printf("generateSelfSignedCert ret = %d \n",certderlen);
	dump(certder,certderlen);
	ret = writeFile(certpath,certder,certderlen);

	
	wc_FreeRsaKey(&rsakey);	
	return ret;
}

uint32_t signCrt(const uint8_t *cakeypath,const uint8_t *kldcrtpath,
		const uint8_t * signedcrtpath)
{
	uint8_t  privkey[2048];
	uint32_t privkeylen = 2048;
	uint8_t  certder[2048];
	uint32_t certderlen = 2048;	
	uint8_t  signedcertder[2048];
	uint32_t signedcertderlen = 2048;		
	uint32_t ret = 0;	
	RsaKey carsakey;
	CertTBS_t kldcert;

	privkeylen = readFile(cakeypath,privkey);
	certderlen = readFile(kldcrtpath,certder);
	
	ret = readRsaPrivKeyFromBuffer(&carsakey,privkey,privkeylen);	
	ret = Der_To_Cert(certder,certderlen,CTC_SHA256wRSA,&kldcert);

	memset(signedcertder,0,signedcertderlen);
	memcpy(signedcertder,kldcert.body,kldcert.bodysize);
	signedcertderlen = signCert(kldcert.bodysize,kldcert.sigType,
		signedcertder,signedcertderlen,&carsakey);
	dump(signedcertder,signedcertderlen);
	ret = writeFile(signedcrtpath,signedcertder,signedcertderlen);
	
	wc_FreeRsaKey(&carsakey);	

	return ret;	
	
}

uint32_t signCsr(const uint8_t *kldkeypath,const uint8_t *mintcsrpath,
		const uint8_t * mintcrtpath)
{
	uint8_t  privkey[2048];
	uint32_t privkeylen = 2048;
	uint8_t csrpem[2048] = {0};
	uint32_t csrpemlen = 2048;
	uint8_t csrder[2048] = {0};
	uint32_t csrderlen = 2048;	
	uint8_t  signedcertder[2048];
	uint32_t signedcertderlen = 2048;		
	uint32_t ret = 0;
	
	CertReqTBS_t mintcsr;
	RsaKey kldrsakey;

	privkeylen = readFile(kldkeypath,privkey);
	csrpemlen = readFile(mintcsrpath,csrpem);	
	csrderlen =  CertPemToDer(csrpem,csrpemlen,csrder,csrderlen,CERTREQ_TYPE);
	printf("mint_csr\n");
	dump(csrder,csrderlen);
	
	ret = readRsaPrivKeyFromBuffer(&kldrsakey,privkey,privkeylen);	
	ret = Der_To_Req(csrder,csrderlen,CTC_SHA256wRSA,&mintcsr);
	//dumpKivviCertReq(&mintcsr);
	
	signedcertderlen = genCert(&mintcsr,signedcertder,signedcertderlen,
		&kldrsakey,20);
	
	printf("crt\n");
	dump(signedcertder,signedcertderlen);
	
	ret = writeFile(mintcrtpath,signedcertder,signedcertderlen);
	
	wc_FreeRsaKey(&kldrsakey);	

	return ret;	

}

uint32_t checkCrtSign(const uint8_t *cacrtpath,const uint8_t *usercrtpath)
{
	uint8_t  cacertder[2048];
	uint32_t cacertderlen = 2048;	
	uint8_t  usercertder[2048];
	uint32_t usercertderlen = 2048;				
	uint32_t ret = 0;	
	CertTBS_t cacert;
	CertTBS_t usercert;
	RsaKey capubkey;

	cacertderlen = readFile(cacrtpath,cacertder);
	usercertderlen = readFile(usercrtpath,usercertder);
	
	
	printf("cacertderlen len = %d \n",cacertderlen);	
	printf("usercertderlen len = %d \n",usercertderlen);	

#if 0
	ret = kivvissl_verifyCert(cacertder,cacertderlen,usercertder,usercertderlen);
#else
	cacertderlen = readFile(cacrtpath,cacertder);
	usercertderlen = readFile(usercrtpath,usercertder);

	ret = Der_To_Cert(cacertder,cacertderlen,CTC_SHA256wRSA,&cacert);	
	ret = Der_To_Cert(usercertder,usercertderlen,CTC_SHA256wRSA,&usercert);
	dumpKivviCert(&cacert);
	dumpKivviCert(&usercert);
	
	ret = readRsaPubKeyFromBuffer(&capubkey,cacert.publicekey,cacert.publicekey_size);
	
	printf("checkCrtSign readRsaPubKeyFromBuffer = %d\n",ret);
	
	ret = verifyCertSign(&usercert,&capubkey);
	printf("checkCrtSign verifyCertSign = %d\n",ret);
		
	wc_FreeRsaKey(&capubkey);	
#endif	
	printf("checkCrtSign verifyCertSign = %d\n",ret);

		
	return ret; 
}
void RSAEncryptDecryptTest(uint8_t *privkeypath,uint8_t *pubkeypath)
{
	RsaKey privkey;
	RsaKey pubkey;
	uint32_t ret = 0;	

	uint8_t  temp[2048];
	uint32_t templen = 2048;	

	uint8_t* plain="cynovo_test";
	uint32_t plainlen = 11;
	uint8_t  encrptdata[512];
	uint32_t encrptdatalen = 512;	
	uint8_t  decryptdata[512];
	uint32_t decryptdatalen = 512;		
	
	templen = readFile(privkeypath,temp);
	ret = readRsaPrivKeyFromBuffer(&privkey,temp,templen);	
	printf("read priv key = %d \n",ret);	
	templen = 2048;
	templen = readFile(pubkeypath,temp);
	ret = readRsaPubKeyFromBuffer(&pubkey,temp,templen);	
	printf("read pub key = %d \n",ret);

#if 1
	printf("\n\n");
	printf("plain:%s \n",plain);
	encrptdatalen = RSAPublicKeyEncrypt(plain,plainlen,encrptdata,encrptdatalen,&pubkey);
	printf("RSAPublicKeyEncrypt encrptdata = %d \n",encrptdatalen);
	dumpX(encrptdata,encrptdatalen,16);
	decryptdatalen = RSAPrivateKeyDecrypt(encrptdata,encrptdatalen,decryptdata,decryptdatalen,&privkey);
	decryptdata[decryptdatalen]=0;
	printf("RSAPrivateKeyDecrypt decryptdatalen = %d(%s) \n",decryptdatalen,decryptdata);
#endif
	printf("\n\n");
	encrptdatalen = 512;
	decryptdatalen = 512;

#if 0
	printf("plain:%s \n",plain);
	dumpX(plain,plainlen,16);

	encrptdatalen = RSAPrivateKeyEncrypt(plain,plainlen,encrptdata,encrptdatalen,&privkey);
	printf("RSAPrivateKeyEncrypt encrptdata = %d \n",encrptdatalen);
	dumpX(encrptdata,encrptdatalen,16);
	decryptdatalen = RSAPublicKeyDecrypt(encrptdata,encrptdatalen,decryptdata,decryptdatalen,&pubkey);
	decryptdata[decryptdatalen]=0;
	printf("RSAPublicKeyDecrypt decryptdatalen = %d(%s) \n",decryptdatalen,decryptdata);
#endif
	printf("\n\n");
	encrptdatalen = 512;
	decryptdatalen = 512;

#if 0
	decryptdatalen = RSAPrivateKeyDecrypt(ubuntu_pubkey_encrypt,256,
		decryptdata,decryptdatalen,&privkey);
	printf("RSAPrivateKeyDecrypt ubuntu enctypt data ret = %d \n",decryptdatalen);

	decryptdatalen = RSAPublicKeyDecrypt(ubuntu_privkey_encrypt,256,
		decryptdata,decryptdatalen,&pubkey);
	printf("RSAPublicKeyDecrypt ubuntu enctypt data ret = %d \n",decryptdatalen);
#endif	
	wc_FreeRsaKey(&privkey);	
	wc_FreeRsaKey(&pubkey);	
}

int getPubkeyFromCert(uint8_t *certpath,uint8_t *pubkeypath)
{
	uint32_t ret = 0;	
	uint8_t  cert[2048];
	uint32_t certlen = 2048;	
	
	CertTBS_t cert_tbs;
	
	certlen = readFile(certpath,cert);
		
	ret = Der_To_Cert(cert,certlen,CTC_SHA256wRSA,&cert_tbs);
	if(ret==0)		
		ret = writeFile(pubkeypath,cert_tbs.publicekey,cert_tbs.publicekey_size);
out:
	return ret;
	
}

int CertEncrypt_PrivKeyDecrypt(uint8_t *certpath,uint8_t *privkeypath)
{
	const uint8_t* plain = "cynovo_test";
	const uint32_t plainlen = 11;

	uint32_t ret = 0;	
	uint8_t  cert[2048];
	uint32_t certlen = 2048;	
	uint8_t  privkeydata[2048];
	uint32_t privkeydatalen = 2048;
	uint8_t  encrptdata[256];
	uint32_t encrptdatalen = 256;	
	uint8_t  decrptdata[256];
	uint32_t decrptdatalen = 256;
	
	CertTBS_t cert_tbs;
	RsaKey pubkey;	
	RsaKey privkey;
	

	dumpPrintf(plain,plainlen);
	
	certlen = readFile(certpath,cert);
	privkeydatalen = readFile(privkeypath,privkeydata);
	
	ret = Der_To_Cert(cert,certlen,CTC_SHA256wRSA,&cert_tbs);
	ret =  readRsaPubKeyFromBuffer(&pubkey,cert_tbs.publicekey,cert_tbs.publicekey_size);
	printf("readRsaPubKeyFromBuffer ret = %d \n",ret);		
	if(ret!=0)
		goto out;

	ret = readRsaPrivKeyFromBuffer(&privkey,privkeydata,privkeydatalen);	
	printf("readRsaPrivKeyFromBuffer ret = %d \n",ret);
	if(ret!=0)
		goto out;	
	
	encrptdatalen = RSAPublicKeyEncrypt(plain,plainlen,encrptdata,encrptdatalen,&pubkey);
	printf("RSAPublicKeyEncrypt encrptdatalen = %d \n",encrptdatalen);		
	dumpPrintf(encrptdata,encrptdatalen);

	decrptdatalen = RSAPrivateKeyDecrypt(encrptdata,encrptdatalen,
		decrptdata,decrptdatalen,&privkey);
	printf("RSAPrivateKeyDecrypt decrptdatalen = %d (%s)\n",decrptdatalen,decrptdata);
	 
		
	wc_FreeRsaKey(&pubkey);	
	wc_FreeRsaKey(&privkey);	
out:
	return ret;
}

unsigned char pub_encry_data_32550[256] = {
	0x79,0xfc,0x2b,0x70,0x3e,0x27,0x6a,0xc3,0xb1,0x2f,0xa1,0x80,0xc4,0xc4,0xb8,0x7c,
	0x82,0x05,0x37,0x03,0x23,0x1f,0x24,0x8d,0xc1,0xc9,0x8e,0x68,0x96,0x16,0xcb,0xde,
	0x0a,0xcf,0x02,0x7d,0x75,0x34,0xdc,0x1a,0xfe,0x7e,0x79,0xca,0x98,0x37,0xf4,0x8b,
	0xe8,0xe3,0xe0,0x8d,0xe3,0x63,0x5f,0x4a,0x97,0x0f,0xc9,0xd4,0xaf,0x2d,0x82,0x0c,
	0x74,0x39,0xad,0xed,0xc1,0xf1,0xc2,0x0f,0x18,0xff,0x77,0xe9,0x8c,0x82,0xa4,0x9b,
	0x2b,0x53,0xfd,0x29,0xf7,0xeb,0x43,0x7f,0x13,0xf9,0x0d,0x99,0xe4,0x13,0x01,0x75,
	0x22,0x71,0xd0,0xc8,0x75,0x5b,0x33,0xd2,0xf2,0x78,0x15,0x01,0x9b,0x0f,0xe6,0x74,
	0x79,0xd6,0x6f,0x05,0x11,0xb3,0xd8,0xf2,0xa5,0xaf,0xc2,0x97,0x79,0x2f,0x8b,0xf9,
	0xd1,0xfd,0x88,0x2a,0x41,0xaa,0xc4,0x62,0x1a,0xd6,0x64,0x15,0x55,0x84,0x56,0x4d,
	0x21,0xe7,0xe3,0x36,0x77,0x33,0x18,0x81,0xd5,0x3b,0xc2,0x99,0xb9,0x46,0xc1,0xad,
	0x5e,0x52,0xb6,0xaf,0x2a,0x78,0xb7,0x0b,0x04,0xb3,0x33,0x3f,0xb9,0x49,0x94,0x2f,
	0x63,0x3a,0x03,0x84,0x57,0x98,0x5a,0xc7,0x3b,0xaf,0x7e,0xb5,0x86,0x75,0x12,0xa7,
	0xad,0xee,0x32,0x6f,0x99,0x30,0x4a,0x39,0x8b,0x94,0xee,0xbc,0x0d,0x3b,0x65,0x34,
	0x41,0x21,0x39,0x29,0xa1,0x6b,0x11,0x6d,0x62,0xdb,0x40,0xa5,0xd4,0x7e,0x10,0x8e,
	0x11,0x47,0x56,0xa1,0x31,0x98,0x20,0xdd,0xb8,0xcb,0x53,0x4c,0xba,0xbe,0x06,0x8a,
	0xc1,0x57,0x85,0xfb,0xa2,0x29,0xf1,0xee,0xde,0xc6,0x25,0x1e,0x9b,0x1b,0x16,0x16,

};

void EncryptTest(uint8_t *pubkeypath)
{
	RsaKey pubkey;
	uint32_t ret = 0;	

	uint8_t  temp[2048];
	uint32_t templen = 2048;	
	uint8_t  encrptdata[2048];
	uint32_t encrptdatalen = 2048;
	
	templen = readFile(pubkeypath,temp);
	ret = readRsaPubKeyFromBuffer(&pubkey,temp,templen);	
	printf("read pub key = %d \n",ret);	
	
	encrptdatalen = RSAPublicKeyEncrypt("cynovo_test",11,encrptdata,encrptdatalen,&pubkey);
	printf("RSAPublicKeyEncrypt encrptdatalen = %d \n",encrptdatalen);		
	dumpPrintf(encrptdata,encrptdatalen);

	wc_FreeRsaKey(&pubkey);	
}



void RSADecryptTest(uint8_t *privkeypath)
{
	RsaKey privkey;
	uint32_t ret = 0;	

	uint8_t  temp[2048];
	uint32_t templen = 2048;	
	uint8_t  decryptdata[2048];
	uint32_t decryptdatalen = 2048;
	
	templen = readFile(privkeypath,temp);
	ret = readRsaPrivKeyFromBuffer(&privkey,temp,templen);	
	printf("read priv key = %d \n",ret);	
	decryptdatalen = RSAPrivateKeyDecrypt(pub_encry_data_32550,256,
		decryptdata,decryptdatalen,&privkey);
	printf("RSAPrivateKeyDecrypt RSAPrivateKeyDecryptt data ret = %d \n",decryptdatalen);


	wc_FreeRsaKey(&privkey);	
}

int main(){
	uint32_t ret = 0;


	uint8_t  privkeydata[2048];
	uint32_t privkeydatalen = 2048;
	uint8_t  pubkeydata[2048];
	uint32_t pubkeydatalen = 2048;		
	
#if 0		
	ret = genRsaKey(2048,privkeydata,&privkeydatalen,pubkeydata,&pubkeydatalen);
	printf("CA privkey  ret = %d \n",ret);
	dump(privkeydata,privkeydatalen);	
	if(ret==0){ 
		ret = writeFile("/mnt/sdcard/posssl/ca_privkey.der",privkeydata,privkeydatalen);
	}
	else{
		printf("generate ca key fail\n");
		goto out;
	}

	ret =  generateSelfCert("/mnt/sdcard/posssl/ca_privkey.der",
		"/mnt/sdcard/posssl/ca_cert.der");
	if(ret!=0){
		printf("generate ca cert fail\n");
		goto out;		
	}


	ret = genRsaKey(2048,privkeydata,&privkeydatalen,pubkeydata,&pubkeydatalen);
	printf("pad privkey ret = %d \n",ret);
	dump(privkeydata,privkeydatalen);	
	if(ret==0){ 
		ret = writeFile("/mnt/sdcard/posssl/pad_privkey.der",privkeydata,privkeydatalen);
	}
	else{
		printf("generate pad key fail\n");
		goto out;
	}
	ret =  generateSelfCert("/mnt/sdcard/posssl/pad_privkey.der",
		"/mnt/sdcard/posssl/pad_cert.der");
	if(ret!=0){
		printf("generate pad cert fail\n");
		goto out;		
	}	

		ret = signCrt("/mnt/sdcard/posssl/ca_privkey.der","/mnt/sdcard/posssl/pad_cert.der",
		"/mnt/sdcard/posssl/pad_cert_signed.der");
	if(ret!=0){
		printf("sign pad cert fail\n");
		goto out;		
	}
	
#endif 


//	ret = checkCrtSign("/mnt/sdcard/posssl/ca_cert.der",
//		"/mnt/sdcard/posssl/pad_cert_signed.der");
//	printf("check pad_cert_signed.der sign by ca ret = %d\n",ret);	

//	ret = signCrt("/mnt/sdcard/posssl/ca_privkey.der",
//		"/mnt/sdcard/posssl/kivvi_security_subdevice_cert.der",
//		"/mnt/sdcard/posssl/kivvi_security_subdevice_cert_signed.der");

//	printf("sign pos cert ret = %d\n",ret);
//	if(ret!=0){
//		printf("sign pad cert fail\n");
//		goto out;	
//	}
//	ret = checkCrtSign("/mnt/sdcard/posssl/ca_cert.der",
//		"/mnt/sdcard/posssl/kivvi_security_subdevice_cert_signed.der");
//	printf("check kivvi_security_subdevice_cert_signed.der sign by ca ret = %d\n",ret);	


	//getPubkeyFromCert(
	//	"/mnt/sdcard/posssl/kivvi_security_subdevice_cert.der",
	//	"/mnt/sdcard/posssl/kivvi_security_subdevice_cert_pubkey.der");


	//getPubkeyFromCert(
	//	"/mnt/sdcard/posssl/pad_cert.der",
	//	"/mnt/sdcard/posssl/pad_cert_pubkey.der");

	
	//RSAEncryptDecryptTest(
	//	"/mnt/sdcard/posssl/pad_privkey.der",
	//	"/mnt/sdcard/posssl/kivvi_security_subdevice_cert_pubkey.der");


	//RSADecryptTest("/mnt/sdcard/posssl/pad_privkey.der");
	EncryptTest("/mnt/sdcard/posssl/kivvi_security_subdevice_cert_pubkey.der");



out:
	return ret;
}


#if 0
int main(){
	uint32_t ret = 0;

	
#if 0
	//RSADecryptTest("/mnt/sdcard/kivvissl/kivvi_security_ca_key.der");
	

	/*
	RSAEncryptDecryptTest("/mnt/sdcard/terminal_privkey.der",
	"/mnt/sdcard/terminal_pubkey.der");
	*/

	//ret = CertEncrypt_PrivKeyDecrypt("/mnt/sdcard/terminal_signedby_kld.der",
	//"/mnt/sdcard/terminal_privkey.der");	

	//ret = CertEncrypt_PrivKeyDecrypt(
	//"/mnt/sdcard/cert/ca.der",
	//"/mnt/sdcard/cert/ca_privkey.der");	


	RSAEncryptDecryptTest(
		"/mnt/sdcard/terminal_privkey.der",
		"/mnt/sdcard/terminal_pubkey.der");
	RSAEncryptDecryptTest(
		"/mnt/sdcard/kivvissl/kivvi_security_ca_key.der",
		"/mnt/sdcard/kivvissl/kivvi_security_ca_pubkey.der");		


	//ret = CertEncrypt_PrivKeyDecrypt(
	//	"/mnt/sdcard/kivvissl/kivvi_security_ca_cert.der",
	//	//"/mnt/sdcard/kivvissl/kivvi_security_subdevice_cert_signed_by_ca.der",
	//	"/mnt/sdcard/kivvissl/kivvi_security_ca_key.der");	
	

	printf("CertEncrypt ret = %d\n",ret);
	
#endif

#if 0
	ret = generateCAKey();
	printf("generateCA 2048 Key ret = %d\n",ret);
	printf("store in path /mnt/sdcard/cert/, ca_privkey.der ca_pubkey.der \n");

	ret = generateSelfCert("/mnt/sdcard/cert/ca_privkey.der",
			"/mnt/sdcard/cert/ca.der");
	printf("generate CA selfsigned cert ret = %d\n",ret);
			
#endif

#if 0
	ret = generateKLDKey();
	printf("generateKLDKey 2048 Key ret = %d\n",ret);
	printf("store in path /mnt/sdcard/cert/, kld_privkey.der kld_pubkey.der \n");

	ret = generateSelfCert("/mnt/sdcard/cert/kld_privkey.der",
			"/mnt/sdcard/cert/kld.der");
	printf("generate kld selfsigned cert ret = %d\n",ret);	
#endif
#if 0
	ret = generateTermialKey();
	printf("generateTermialKey 2048 Key ret = %d\n",ret);
	printf("store in path /mnt/sdcard/cert/, terminal_privkey.der terminal_pubkey.der \n");

	ret = generateSelfCert("/mnt/sdcard/cert/terminal_privkey.der",
			"/mnt/sdcard/cert/terminal.der");
	printf("generate terminal selfsigned cert ret = %d\n",ret);	
#endif

#if 0
	ret = signCrt("/mnt/sdcard/cert/ca_privkey.der","/mnt/sdcard/cert/kld.der",
		"/mnt/sdcard/cert/kld_signedby_ca.der");
	printf("sign kld.der by ca key,generate kld_signedby_ca.der  ret = %d\n",ret);	

#endif

#if 0
	ret = signCsr("/mnt/sdcard/cert/kld_privkey.der","/mnt/sdcard/cert/mint.csr",
	"/mnt/sdcard/cert/mint.der");
	printf("sign mint.crs by kld key,generate mint.der  ret = %d\n",ret);	
#endif

#if 0
	ret = signCrt("/mnt/sdcard/cert/kld_privkey.der","/mnt/sdcard/cert/terminal.der",
		"/mnt/sdcard/cert/terminal_signedby_kld.der");
	printf("sign terminal.der by kld key,generate terminal_signedby_kld.der  ret = %d\n",ret);	

#endif

#if 0
	ret = checkCrtSign("/mnt/sdcard/cert/kld.der","/mnt/sdcard/cert/mint.der");
	printf("check mint.der sign by kld ret = %d\n",ret);	
#endif
	
#if 0
	printf("checkCrtSign P1\n");	
	ret = checkCrtSign("/mnt/sdcard/kivvissl/kivvi_security_ca_cert.der",
		"/mnt/sdcard/kivvissl/kivvi_security_subdevice_cert_signed_by_ca.der");
	printf("check kld.der sign by ca ret = %d\n",ret);	
#endif
#if 0
	printf("checkCrtSign P2\n");	
	RSAEncryptDecryptTest("/mnt/sdcard/cert/terminal_privkey.der",
		"/mnt/sdcard/cert/terminal_pubkey.der");
#endif
	//testdata();
	//testAsymKey();
	return 0;
}
#endif

