/*************************************************************************
	> File Name: createsig.c
	> Author: davee-x
	> Mail: davee.naughty@gmail.com 
	> Created Time: 2018年06月11日 星期一 21时29分38秒
 ************************************************************************/

// need to be careful of memory new and free when handle error

#include <stdio.h>
#include <openssl/sha.h>
#include <openssl/ecdsa.h>
#include <openssl/bn.h>
#include <openssl/evp.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define READSIZ 1*1024*1024
#define _DEBUG

#ifdef _DEBUG
#define pr_info(f_, ...) printf((f_), ##__VA_ARGS__)
#else
#define pr_info(f_, ...)
#endif

#define checkres((a),(b),(c)) do {\
	if ((a) != (b)) { \
		pr_info(c); \
	} while(0)

#define checkresret((a),(b),(c),(d)) do {\
	if ((a) != (b)) { \
		pr_info(c); \
		return d; \
	} while(0)

#define checknres((a),(b),(c)) do {\
	if ((a) != (b)) { \
		pr_info(c); \
	} while(0)

#define checknresret((a),(b),(c),(d)) do {\
	if ((a) != (b)) { \
		pr_info(c); \
		return d; \
	} while(0)

const char *privkeypath = "../remotepkgs/privkey.pem";
const char *pubkeypath = "../localpkgs/pubkey.pem";

static int get_sha256(char *fnamel, unsigned char* result);
static void dumpbuf(const unsigned char* buf, int size);
static int generate_new_keypairs();
int readpubeckey(EC_KEY **pubeckeyptr);
int readpriveckey(EC_KEY **priveckeyptr);
int ecdsa_sign(unsigned char *content, int contentlen, unsigned char*sig, unsigned int *siglenptr);
int ecdsa_verify(unsigned char *content, int contentlen, unsigned char*sig, unsigned int *siglenptr);

int main (int ac, char *av[]) {
	if (ac !=3) {
		pr_info("%s inputfile sigfilename\n", av[0]);
		return -1;
	}
	unsigned char content[SHA256_DIGEST_LENGTH];
	unsigned char *signature;
	int siglen;
	int fd;

	if (get_sha256(av[1], content) == -1) {
		pr_info("calculate sha256 fail\n");
		return -1;
	}

	// dump hash
//	dumpbuf(result, SHA256_DIGEST_LENGTH);

	if (ecdsa_sign(av[2], result) == -1) {
		pr_info("can not sign\n");
		return -1;
	}
	siglen = ecdsa_sign(NULL, 0, NULL, 0);
	if ((signature = (unsigned char*)OPENSSL_malloc(siglen)) == NULL) {
		printf("can not allocate signature buffer\n");
		return -1;
	}
	
	if (ecdsa_sign(content, SHA256_DIGEST_LENGTH, signature, &siglen) != 0) {
		printf("sig failed\n");
		return -1;
	}

	// write sig into file
	pr_info("begin write sig\n");
	fd = open(av[2], O_WRONLY|O_CREAT|O_TRUNC, 0644);
	if (fd == -1) {
		pr_info("can not open file %s\n", fname);
		return -1;
	}
	if (write(fd, signature, siglen) != siglen) {
		pr_info("can not write sig into file\n");
		return -1;
	}
	close(fd);

	if (ecdsa_verify(content, SHA256_DIGEST_LENGTH, signature, siglen) != 0) {
		printf("verify failed\n");
		return -1;
	}

	return 0;
}

/*
 * readin a file content, calculate sha256sum
 * fname: input file name
 * result: output sha256sum result
 * return value:
 *		0 success
 *		-1 fail
 */
int get_sha256(char *fname, unsigned char* result) {
	SHA256_CTX ctx;
	SHA256_Init(&ctx);

	int fd = open(fname, O_RDONLY);
	if (fd == -1) {
		pr_info("can not open file %s\n", fname);
		return -1;
	}

	char tmpbuf[READSIZ];
	int count = 0;
	while ((count = read(fd, tmpbuf, READSIZ)) > 0) {
		SHA256_Update(&ctx, tmpbuf, count);
	}
	SHA256_Final(result, &ctx);
	close(fd);
	return 0;
}

int generate_new_keypairs() {

	int ret, fd, count;
	EC_KEY *eckey;
	unsigned char *privkeystr = NULL , *pubkeystr = NULL;

	eckey = EC_KEY_new_by_curve_name(NID_secp256k1);
	if (eckey == NULL) {
		pr_info("can not create curve for eckey\n");
		return -1;
	}

	pr_info("begin generate key\n");
	if (!EC_KEY_generate_key(eckey)) {
		pr_info("cannot generate key from curve group\n");
		return -1;
	}

	pr_info("begin save privkey\n");
	// save priv key
	privkeystr = BN_bn2hex(EC_KEY_get0_private_key(eckey));
	if ((fd = open(privkeypath, O_WRONLY|O_CREAT|O_TRUNC, 0600)) == -1) {
		pr_info("can not open file for priv key store\n");
		return -1;
	}
	count = strlen((char*)privkeystr);
	if (write(fd, privkeystr, count) != count) {
		pr_info("error write to file of privkey\n");
		return -1;
	}
	free(privkeystr);
	close(fd);

	pr_info("begin save pubkey\n");
	// save pub key
	pubkeystr = EC_POINT_point2hex(EC_KEY_get0_group(eckey), EC_KEY_get0_public_key(eckey), POINT_CONVERSION_COMPRESSED, NULL);
	if ((fd = open(pubkeypath, O_WRONLY|O_CREAT|O_TRUNC, 0600)) == -1) {
		pr_info("can not open file for pub key store\n");
		return -1;
	}
	count = strlen((char*)pubkeystr);
	if (write(fd, pubkeystr, count) != count) {
		pr_info("error write to file of pubkey\n");
		return -1;
	}
	free(pubkeystr);
	close(fd);
	EC_KEY_free(eckey);
	return 0;
}

int readpriveckey(EC_KEY **priveckeyptr) {
	int count, fd;
	EC_KEY *priveckey = NULL;

	pr_info("begin read privkey\n");
	priveckey = EC_KEY_new_by_curve_name(NID_secp256k1);
	if (priveckey == NULL) {
		pr_info("can not create curve for priveckey\n");
		return -1;
	}
	// read privkey and sign
	if ((fd = open(privkeypath, O_RDONLY)) == -1) {
		pr_info("can not open file for priv key read\n");
		return -1;
	}
	count = lseek(fd, 0L, SEEK_END);
	lseek(fd, 0L, SEEK_SET);
	char *privkeyread = (char*)malloc(count);
	if (privkeyread == NULL) {
		pr_info("can not alloc mem for priv key read\n");
		return -1;
	}
	if (read(fd, privkeyread, count) != count) {
		pr_info("can not read privkey from file\n");
		return -1;
	}
	BIGNUM *privbignum = NULL;
	if (BN_hex2bn(&privbignum, privkeyread) == 0) {
		pr_info("create big num fail\n");
		return 0;
	}
	if (EC_KEY_set_private_key(priveckey, privbignum) != 1) {
		pr_info("can not set private key\n");
		return -1;
	}
	*priveckeyptr = priveckey;
	return 0;
}

int readpubeckey(EC_KEY **pubeckeyptr) {
	EC_POINT *pubkey = NULL;
	EC_KEY *pubeckey = NULL;
	int fd, count;

	// read pubkey and verify
	pr_info("begin read pubkey\n");
	pubeckey = EC_KEY_new_by_curve_name(NID_secp256k1);
	if (pubeckey == NULL) {
		pr_info("can not create curve for pubeckey\n");
		return -1;
	}
	if ((fd = open(pubkeypath, O_RDONLY)) == -1) {
		pr_info("can not open file for pub key read\n");
		return -1;
	}
	count = lseek(fd, 0L, SEEK_END);
	lseek(fd, 0L, SEEK_SET);
	char *pubkeyread = (char*)malloc(count);
	if (pubkeyread == NULL) {
		pr_info("can not alloc mem for pub key read\n");
		return -1;
	}
	if (read(fd, pubkeyread, count) != count) {
		pr_info("can not read pubkey from file\n");
		return -1;
	}
	if ((pubkey = EC_POINT_hex2point(EC_KEY_get0_group(pubeckey), pubkeyread, pubkey, NULL)) == NULL) {
		pr_info("can not convert from hex to ecpoint\n");
		return -1;
	}
	if (EC_KEY_set_public_key(pubeckey, pubkey) != 1) {
		pr_info("can not set public eckey\n");
		return -1;
	}
	*pubeckeyptr = pubeckey;
	return 0;
}

/*
 * passin NULL siglen poiner and return size of signature needed
 *
 * if success, return 0
 *	  error, return -1
 */
int ecdsa_sign(unsigned char *content, int contentlen, unsigned char*sig, unsigned int *siglenptr) {

	EC_KEY *priveckey;

	// return size needed
	if (siglenptr == NULL || sig == NULL) {
		EC_KEY *tmpkey = EC_KEY_new();
		int tmpsize = ECDSA_size(tmpkey);
		EC_KEY_free(tmpkey);
		return tmpsize;
	}

	// if private or public key file not exist, regenerate keys and store into file
	if (access(privkeypath, F_OK) != 0 || access(pubkeypath, F_OK) != 0) {
		if (generate_new_keypairs() != 0) {
			printf("can not generate key pair\n");
			return -1;
		}
	}

	if (readpriveckey(&priveckey) != 0) {
		printf("can not read priveckey\n");
		return -1;
	}

	/*
	// also set pubkey into privatekey
	if (EC_KEY_set_public_key(priveckey, pubkey) != 1) {
		pr_info("can not set public eckey into privkey\n");
		return -1;
	}
	*/

	//sign
	if (!ECDSA_sign(0, content, contentlen, sig, siglenptr, priveckey)) {
		pr_info("can not generate sig\n");
		return -1;
	}

	EC_KEY_free(priveckey);
	return 0;
}


/*
 * verify
 * return:
 *		-1 error, 0 verify fail, 1 success
 */
int ecdsa_verify(unsigned char *content, int contentlen, unsigned char*sig, unsigned int *siglenptr) {
	EC_KEY *pubeckey;
	int fd, count, ret;

	// do a checkarg here??
	if (sig == NULL || siglenptr == NULL) {
		printf("can not get sig\n");
		return -1;
	}

	if (readpubeckey(&pubeckey) != 0) {
		printf("can not read pubeckey\n");
		return -1;
	}

	/*
	pr_info("begin write sig\n");
	// write sig into file
	fd = open(fname, O_WRONLY|O_CREAT|O_TRUNC, 0644);
	if (fd == -1) {
		pr_info("can not open file %s\n", fname);
		return -1;
	}
	if (write(fd, signature, siglen) != siglen) {
		pr_info("can not write sig into file\n");
		return -1;
	}
	close(fd);


	pr_info("begin read sig\n");
	// read sig from file
	unsigned char *sigreadbuf = NULL;
	if ((sigreadbuf = (unsigned char *)OPENSSL_malloc(ECDSA_size(pubeckey))) == NULL) {
		pr_info("can not allocate buffer for signature read\n");
		return -1;
	}

	fd = open(fname, O_RDONLY);
	if (fd == -1) {
		pr_info("can not open file %s for read\n", fname);
		return -1;
	}
	count = lseek(fd, 0L, SEEK_END);
	lseek(fd, 0L, SEEK_SET);
	if (read(fd, sigreadbuf, count) != count) {
		pr_info("read sig from file failed\n");
		return -1;
	}
	*/

	// verify
	ret = ECDSA_verify(0, content, contentlen, sig, *siglenptr, pubeckey);
	if (ret == -1) {
		pr_info("verify error\n");
	} else if (ret == 0) {
		pr_info("verify fail, wrong sig\n");
	} else {
		pr_info("verify success\n");
	}
	EC_KEY_free(pubeckey);
	return ret;
}

void dumpbuf(const unsigned char* buf, int size) {
	for (int i = 0; i < size; ++i) {
		pr_info("%02x", buf[i]);
		if (i % 16 == 15) {
			putchar('\n');
		}
	}
}
