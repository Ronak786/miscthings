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

#ifdef __cplusplus
}
#endif

#include <string>

class SigUtil {

private:
    /*
     * readin a file content, calculate sha256sum
     * fname: input file name
     * result: output sha256sum result
     * return value:
     *		0 success
     *		-1 fail
     */
    static int get_sha256(const char *fnamel, unsigned char* result);

    /*
     * success: 0
     * error: -1
     */
    static int readpubeckey(EC_KEY **pubeckeyptr, const char* pub);

    /*
     * success: 0
     * error: -1
     */
    static int readpriveckey(EC_KEY **priveckeyptr, const char *priv, const char* pub);

    /*
     * success: num of bytes in buf
     * error: -1
     */
    static int readkeyfromfile(const char* fname, char**bufptr);

    /*
     * read priv and pub key from file and sign
     * success: 0
     * error: -1
     */
    static int ecdsa_sign(unsigned char *content, int contentlen, unsigned char* sig, unsigned int *siglenptr,
            const char *privkeyfname, const char *pubkeyfname);

    /*
     * internal function called by checksig ??
     * read pub key from file and verify
     * success: >0
     * fail: 0
     * error: -1
     */
    static int ecdsa_verify(unsigned char *content, int contentlen, unsigned char* sig, unsigned int siglen,
            const char* pubkeyfname);

public:
    /*
     * success: return 0
     * error: -1
     */
    static int generate_new_keypairs(const char* privpath, const char *pubpath);

    /*
     * check signature
     * input: pkgfilename, pkgfilesigname, public key file name
     * output: signature true or false
     */
    static bool verify(const char* fname, unsigned char* sigbuf, unsigned int siglen, const char* pubkeypath);
    static int sign(const char* fname, unsigned char* sig, unsigned int *siglenptr,
              const char*privkeyfname, const char *pubkeyfname);
};

