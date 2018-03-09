#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <jni.h>
#include <android/log.h>


#include "kivvissl.h"
#include "hlog.h"

#ifdef __cplusplus
extern "C" {
#endif

int Java_com_cynovo_security_jni_WolfSsl_generateKey(JNIEnv* env, jobject obj,
	jbyteArray privkey)
{
	int ret = -1;
	
  	jbyte* privkey_data = env->GetByteArrayElements(privkey,0);
	if(privkey_data==NULL)
		goto out;
	
	ret = kivvissl_generateKey((uint8_t*)privkey_data);

	env->ReleaseByteArrayElements(privkey, privkey_data, 0);	
out:
	return ret;
	
}

int Java_com_cynovo_security_jni_WolfSsl_verifyCert(JNIEnv* env, jobject obj,
    jbyteArray cacert,int cacertlen,
    jbyteArray usercert,int usercertlen)
{
    int ret = -1;
	jbyte* cacert_data = NULL;
    jbyte* usercert_data = NULL;
	
  	cacert_data = env->GetByteArrayElements(cacert,0);
	if(cacert_data==NULL)
		goto out2;
    usercert_data = env->GetByteArrayElements(usercert,0);
	if(usercert_data==NULL)
		goto out1;

	ret = kivvissl_verifyCert((uint8_t*)cacert_data,cacertlen,
		(uint8_t*)usercert_data,usercertlen);

out1:
	env->ReleaseByteArrayElements(cacert, cacert_data, 0);	
out2:
    return ret;
}

int Java_com_cynovo_security_jni_WolfSsl_generateSelfCert(JNIEnv* env, jobject obj,
    jbyteArray privkey,int privkeydatalen,
    jbyteArray cert,int validays)
{
    int ret = -1;
	jbyte* cert_data = NULL;
    jbyte* privkey_data = NULL;
	
  	cert_data = env->GetByteArrayElements(cert,0);
	if(cert_data==NULL)
		goto out2;
    privkey_data = env->GetByteArrayElements(privkey,0);
	if(privkey_data==NULL)
		goto out1;

	ret = kivvissl_generateSelfCert((uint8_t*)privkey_data,privkeydatalen,(uint8_t*)cert_data,validays);
	
	env->ReleaseByteArrayElements(privkey, privkey_data, 0);	
out1:
	env->ReleaseByteArrayElements(cert, cert_data, 0);	
out2:
    return ret;
}

int Java_com_cynovo_security_jni_WolfSsl_signCrt(JNIEnv* env, jobject obj,
    jbyteArray privkey,int privkeydatalen,
    jbyteArray cert,int certdatalen,
    jbyteArray signedcert)
{
    int ret = -1;
	jbyte* cert_data = NULL;
    jbyte* privkey_data = NULL;
	jbyte* signedcert_data = NULL;

	cert_data = env->GetByteArrayElements(cert,0);
	if(cert_data==NULL)
		goto out3;
    privkey_data = env->GetByteArrayElements(privkey,0);
	if(privkey_data==NULL)
		goto out2;
    signedcert_data = env->GetByteArrayElements(signedcert,0);
	if(privkey_data==NULL)
		goto out1;	

	ret = kivvissl_signCrt((uint8_t*)privkey_data,privkeydatalen,
		(uint8_t*)cert_data,certdatalen,(uint8_t*)signedcert_data);


	env->ReleaseByteArrayElements(signedcert, signedcert_data, 0);	
out1:
	env->ReleaseByteArrayElements(privkey, privkey_data, 0);	
out2:	
	env->ReleaseByteArrayElements(cert, cert_data, 0);	
out3:	
	
    return ret;

}

int Java_com_cynovo_security_jni_WolfSsl_getPubKeyFromCert(JNIEnv* env, jobject obj,
    jbyteArray cert,int certdatalen,
    jbyteArray pubkey)
{
	int ret = -1;
	jbyte* cert_data = NULL;
	jbyte* pubkey_data = NULL;
	
	cert_data = env->GetByteArrayElements(cert,0);
	if(cert_data==NULL)
		goto out2;
	pubkey_data = env->GetByteArrayElements(pubkey,0);
	if(pubkey_data==NULL)
		goto out1;

	ret = kivvissl_getPubkeyFromCert((uint8_t *)cert_data,certdatalen,(uint8_t *)pubkey_data);

	env->ReleaseByteArrayElements(pubkey, pubkey_data, 0);	
out1:
	env->ReleaseByteArrayElements(cert, cert_data, 0);	
out2:
		return ret;

}
    

int Java_com_cynovo_security_jni_WolfSsl_pubEncrypt(
	JNIEnv* env, jobject obj,
    jbyteArray pubkey,int pubkeydatalen,
    jbyteArray plain,int plainlen,
    jbyteArray encrypt)
{
	int ret = -1;
	jbyte* pubkey_data = NULL;
	jbyte* plain_data = NULL;	
	jbyte* encrypt_data = NULL;

	pubkey_data = env->GetByteArrayElements(pubkey,0);
	if(pubkey_data==NULL)
		goto out3;
    plain_data = env->GetByteArrayElements(plain,0);
	if(plain_data==NULL)
		goto out2;
    encrypt_data = env->GetByteArrayElements(encrypt,0);
	if(encrypt_data==NULL)
		goto out1;	
	

	ret = kivvissl_pubEncrypt((uint8_t*)pubkey_data,pubkeydatalen,
			(uint8_t*)plain_data,plainlen,(uint8_t*)encrypt_data) ;
	
	env->ReleaseByteArrayElements(encrypt, encrypt_data, 0);	
out1:
	env->ReleaseByteArrayElements(plain, plain_data, 0);	
out2:	
	env->ReleaseByteArrayElements(pubkey, pubkey_data, 0);	
out3:	
	return ret;

}

int Java_com_cynovo_security_jni_WolfSsl_privDecrypt(
	JNIEnv* env, jobject obj,
    jbyteArray privkey,int privkeylen,
    jbyteArray encrypt,int encryptlen,
    jbyteArray plain)

{
	int ret = -1;
	jbyte* privkey_data = NULL;
	jbyte* encrypt_data = NULL;
	jbyte* plain_data = NULL;	

	privkey_data = env->GetByteArrayElements(privkey,0);
	if(privkey_data==NULL)
		goto out3;
	plain_data = env->GetByteArrayElements(plain,0);
	if(plain_data==NULL)
		goto out2;
	encrypt_data = env->GetByteArrayElements(encrypt,0);
	if(encrypt_data==NULL)
		goto out1;	
	

	ret = kivvissl_privDecrypt((uint8_t*)privkey_data,privkeylen,
			(uint8_t*)encrypt_data,encryptlen,(uint8_t*)plain_data) ;
	
	env->ReleaseByteArrayElements(encrypt, encrypt_data, 0);	
out1:
	env->ReleaseByteArrayElements(plain, plain_data, 0);	
out2:	
	env->ReleaseByteArrayElements(privkey, privkey_data, 0);	
out3:	
	return ret;
} 

int Java_com_cynovo_security_jni_WolfSsl_privSign(
	JNIEnv* env, jobject obj,
    jbyteArray privkey,int privkeylen,
    jbyteArray plain,int plainlen,
    jbyteArray signature)

{

	int ret = -1;
	jbyte* privkey_data = NULL;
	jbyte* plain_data = NULL;	
	jbyte* signature_data = NULL;

	privkey_data = env->GetByteArrayElements(privkey,0);
	if(privkey_data==NULL)
		goto out3;
	plain_data = env->GetByteArrayElements(plain,0);
	if(plain_data==NULL)
		goto out2;
	signature_data = env->GetByteArrayElements(signature,0);
	if(signature_data==NULL)
		goto out1;	
	

	ret = kivvissl_privSign((uint8_t*)privkey_data,privkeylen,
			(uint8_t*)plain_data,plainlen,(uint8_t*)signature_data) ;
	
	env->ReleaseByteArrayElements(signature, signature_data, 0);	
out1:
	env->ReleaseByteArrayElements(plain, plain_data, 0);	
out2:	
	env->ReleaseByteArrayElements(privkey, privkey_data, 0);	
out3:	
	return ret;
}



#ifdef __cplusplus
}
#endif
