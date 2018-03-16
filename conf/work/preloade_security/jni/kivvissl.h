#ifndef _KIVVISSL_H
#define _KIVVISSL_H


#ifdef __cplusplus
extern "C" {
#endif

uint32_t kivvissl_verifyCert(uint8_t* cacert_data,uint32_t cacertlen,uint8_t* usercert_data,uint32_t usercertlen);
uint32_t kivvissl_generateKey(uint8_t* privkeydata);
uint32_t kivvissl_generateSelfCert(uint8_t *privkeydata,uint32_t privkeydatalen,uint8_t *certdata,uint32_t validays);
uint32_t kivvissl_signCrt(uint8_t *privkeydata,uint32_t privkeydatalen,uint8_t *certdata,uint32_t certdatalen,uint8_t *signed_certdata);
uint32_t kivvissl_getPubkeyFromCert(uint8_t *certdata,uint32_t certdatalen,uint8_t *pubkey);
uint32_t kivvissl_pubEncrypt(uint8_t *pubkeydata,uint32_t pubkeydatalen,uint8_t *plain,uint32_t plainlen,uint8_t *encrypt_data)	;
uint32_t kivvissl_privDecrypt(uint8_t *privatedata,uint32_t privatedatalen,uint8_t *encrypt_data,uint32_t encrypt_datalen,uint8_t *plain);

#ifdef __cplusplus
}
#endif
#endif
