/*************************************************************************
	> File Name: createsig.h
	> Author: davee-x
	> Mail: davee.naughty@gmail.com 
	> Created Time: Wed 13 Jun 2018 09:06:55 AM CST
 ************************************************************************/

/*
 * this library just a helper lib of functions in openssl
 * , use c-type is better, should not use qt types in private functions I think,
 * just use them in public funcitons
 */
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

#include <QString>
#include <pkghandle.h>

class SigUtil {

friend class PkgHandle;
private:
    /*
     * readin a file content, calculate sha256sum
     * fname: input file name, start offset in file , end offset in file(exclusively, 0L means read until end)
     * result: output sha256sum result
     * return value:
     *		0 success
     *		-1 fail
     */
    static int get_sha256_from_file(const char *fnamel, unsigned char* result, unsigned long start, unsigned long end);

    /*
     * file structure  data + signature + siglen(1 byte)
     * this function check this structure, return in parameter data start offset and datalen, and copy
     *      signature into sig ptr and set siglength into siglen
     */
    static int split_and_getsig(const char *fname, unsigned long *datastart, unsigned long *datalen,
                         unsigned char* sig, unsigned int *siglen);
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
    static int readkeyfromfile(const char* fname, char** bufptr);

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
     * input: pubkey filepath and privatekey filepath
     * success: return 0
     * error: -1
     */
    static int generate_new_keypairs(QString privpath, QString pubpath);

    /*
     * check signature
     * input: pkgfilename, pkgfilesigname, public key file name
     * output: signature true or false
     */
    static bool verify(QString fname, QString pubkeypath);


    /*
     * sign one package
     * input: package full path, a qbytearray buffer for sig store,
     *          privatekey and publickey file path
     * output: 0 for success, 1 for fail
     *
     */
    static int sign(QString fname, QByteArray& sig,
              QString privkeyfname, QString pubkeyfname);

    // resize file to wipe sig part for later extract
    static int resize(QString fname);


};

