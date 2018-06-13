/*************************************************************************
	> File Name: createsig.h
	> Author: davee-x
	> Mail: davee.naughty@gmail.com 
	> Created Time: Wed 13 Jun 2018 09:06:55 AM CST
 ************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#include <openssl/ecdsa.h>
#include <openssl/sha.h>
#include <openssl/bn.h>
#include <openssl/evp.h>

/*
 * readin a file content, calculate sha256sum
 * fname: input file name
 * result: output sha256sum result
 * return value:
 *		0 success
 *		-1 fail
 */
int get_sha256(const char *fnamel, unsigned char* result);

/*
 * buf dump into hex
 * input buf and size of buf
 */
void dumpbuf(const unsigned char* buf, int size);

/*
 * success: return 0
 * error: -1
 */
int generate_new_keypairs();

/*
 * success: 0
 * error: -1
 */
int readpubeckey(EC_KEY **pubeckeyptr);

/*
 * success: 0
 * error: -1
 */
int readpriveckey(EC_KEY **priveckeyptr);

/*
 * success: num of bytes in buf
 * error: -1
 */
int readkeyfromfile(const char* fname, char**bufptr);

/*
 * success: 0
 * error: -1
 */
int ecdsa_sign(unsigned char *content, int contentlen, unsigned char*sig, unsigned int *siglenptr);

/*
 * success: >0
 * fail: 0
 * error: -1
 */
int ecdsa_verify(unsigned char *content, int contentlen, unsigned char*sig, unsigned int siglen);

#ifdef __cplusplus
}
#endif
