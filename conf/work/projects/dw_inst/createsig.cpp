/*************************************************************************
	> File Name: createsig.c
	> Author: davee-x
	> Mail: davee.naughty@gmail.com 
	> Created Time: 2018年06月11日 星期一 21时29分38秒
 ************************************************************************/

#include <stdio.h>
#include <openssl/sha.h>
#include <openssl/ecdsa.h>
#include <openssl/bn.h>
#include <openssl/evp.h>
#include <fcntl.h>
#include <unistd.h>

#include <string.h>

#define READSIZ 1*1024*1024

const char *privkeypath = "../remotepkgs/privkey.pem";
const char *pubkeypath = "../localpkgs/pubkey.pem";

static int get_sha256(char *fnamel, unsigned char* result);
static int ecdsa_sign(char *fname, unsigned char* result);
static void dumpbuf(const unsigned char* buf, int size);

int main (int ac, char *av[]) {
	if (ac !=3) {
		printf("%s inputfile sigfilename\n", av[0]);
		return -1;
	}
	unsigned char result[SHA256_DIGEST_LENGTH];

	if (get_sha256(av[1], result) == -1) {
		printf("calculate sha256 fail\n");
		return -1;
	}

	// dump hash
	for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
		printf("%02x", (int)result[i]);
		if (i % 16 == 15) putchar('\n');
	}

	if (ecdsa_sign(av[2], result) == -1) {
		printf("can not sign\n");
		return -1;
	}

	return 0;
}

int get_sha256(char *fname, unsigned char* result) {
	SHA256_CTX ctx;
	SHA256_Init(&ctx);

	int fd = open(fname, O_RDONLY);
	if (fd == -1) {
		printf("can not open file %s\n", fname);
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


int ecdsa_sign(char *fname, unsigned char *result) {

	int ret, fd, count;
	ECDSA_SIG *sig;
	EC_KEY *eckey, *priveckey, *pubeckey;
	EC_POINT *pubkey = NULL;


	if (access(privkeypath, F_OK) != 0 || access(pubkeypath, F_OK) != 0) {

		eckey = EC_KEY_new_by_curve_name(NID_secp256k1);
		if (eckey == NULL) {
			printf("can not create curve for eckey\n");
			return -1;
		}

		// if key not saved yet , create them
		printf("begin generate key\n");
		if (!EC_KEY_generate_key(eckey)) {
			printf("cannot generate key from curve group\n");
			return -1;
		}

		printf("begin save privkey\n");
		// save priv key
		char *privkeystr = BN_bn2hex(EC_KEY_get0_private_key(eckey));
		if ((fd = open(privkeypath, O_WRONLY|O_CREAT|O_TRUNC, 0600)) == -1) {
			printf("can not open file for priv key store\n");
			return -1;
		}
		count = strlen(privkeystr);
		if (write(fd, privkeystr, count) != count) {
			printf("error write to file of privkey\n");
			return -1;
		}
		free(privkeystr);
		close(fd);

		printf("begin save pubkey\n");
		// save pub key
		char *pubkeystr = EC_POINT_point2hex(EC_KEY_get0_group(eckey), EC_KEY_get0_public_key(eckey), POINT_CONVERSION_COMPRESSED, NULL);
		if ((fd = open(pubkeypath, O_WRONLY|O_CREAT|O_TRUNC, 0600)) == -1) {
			printf("can not open file for pub key store\n");
			return -1;
		}
		count = strlen(pubkeystr);
		if (write(fd, pubkeystr, count) != count) {
			printf("error write to file of pubkey\n");
			return -1;
		}
		free(pubkeystr);
		close(fd);
		EC_KEY_free(eckey);
	}


	printf("begin read privkey\n");
	priveckey = EC_KEY_new_by_curve_name(NID_secp256k1);
	if (priveckey == NULL) {
		printf("can not create curve for priveckey\n");
		return -1;
	}
	// read privkey and sign
	if ((fd = open(privkeypath, O_RDONLY)) == -1) {
		printf("can not open file for priv key read\n");
		return -1;
	}
	count = lseek(fd, 0L, SEEK_END);
	lseek(fd, 0L, SEEK_SET);
	char *privkeyread = (char*)malloc(count);
	if (privkeyread == NULL) {
		printf("can not alloc mem for priv key read\n");
		return -1;
	}
	if (read(fd, privkeyread, count) != count) {
		printf("can not read privkey from file\n");
		return -1;
	}
	BIGNUM *privbignum = NULL;
	if (BN_hex2bn(&privbignum, privkeyread) == 0) {
		printf("create big num fail\n");
		return 0;
	}
	if (EC_KEY_set_private_key(priveckey, privbignum) != 1) {
		printf("can not set private key\n");
		return -1;
	}


	// read pubkey and verify
	printf("begin read pubkey\n");
	pubeckey = EC_KEY_new_by_curve_name(NID_secp256k1);
	if (pubeckey == NULL) {
		printf("can not create curve for pubeckey\n");
		return -1;
	}
	if ((fd = open(pubkeypath, O_RDONLY)) == -1) {
		printf("can not open file for pub key read\n");
		return -1;
	}
	count = lseek(fd, 0L, SEEK_END);
	lseek(fd, 0L, SEEK_SET);
	char *pubkeyread = (char*)malloc(count);
	if (pubkeyread == NULL) {
		printf("can not alloc mem for pub key read\n");
		return -1;
	}
	if (read(fd, pubkeyread, count) != count) {
		printf("can not read pubkey from file\n");
		return -1;
	}
	if ((pubkey = EC_POINT_hex2point(EC_KEY_get0_group(pubeckey), pubkeyread, pubkey, NULL)) == NULL) {
		printf("can not convert from hex to ecpoint\n");
		return -1;
	}
	if (EC_KEY_set_public_key(pubeckey, pubkey) != 1) {
		printf("can not set public eckey\n");
		return -1;
	}
	if (EC_KEY_set_public_key(priveckey, pubkey) != 1) {
		printf("can not set public eckey into privkey\n");
		return -1;
	}

	//sign
	sig = ECDSA_do_sign(result, SHA256_DIGEST_LENGTH, priveckey);
	if (sig == NULL) {
		printf("can not generate sig\n");
		return -1;
	}
	printf("success signed\n");


	printf("begin write sig\n");
	// write sig into file
	int siglen = 1024;
	unsigned char *sigbuf = (unsigned char*)malloc(siglen);
	if (sigbuf == NULL) {
		printf(" cannot allocate sig buf\n");
		return -1;
	}
	memset(sigbuf, 0, siglen);

	if ((count = i2d_ECDSA_SIG(sig, &sigbuf)) == 0) {
		printf("cannot convert to sig der struct\n");
		return -1;
	}
	printf("convert count is %d\n", count);

	// try revert from i2d
	ECDSA_SIG *sigreadtmp = NULL;
	d2i_ECDSA_SIG(&sigreadtmp, (const unsigned char**)&sigbuf, count);
	if (sigreadtmp == NULL) {
		printf("try cannot convert der struct to sig\n");
		return -1;
	} else {
		printf("try success convert try\n");
		return -1;
	}
	// try revert from i2d


	fd = open(fname, O_WRONLY|O_CREAT|O_TRUNC, 0644);
	if (fd == -1) {
		printf("can not open file %s\n", fname);
		return -1;
	}
	if (write(fd, sigbuf, count) != count) {
		printf("can not write sig into file\n");
		return -1;
	}
	close(fd);


	printf("begin read sig\n");
	// read sig from file
	unsigned char *sigreadbuf = (unsigned char*)malloc(ECDSA_size(pubeckey));
	if (sigreadbuf == NULL) {
		printf(" cannot allocate sigread buf\n");
		return -1;
	}
	ECDSA_SIG *sigread = NULL;

	fd = open(fname, O_RDONLY);
	if (fd == -1) {
		printf("can not open file %s for read\n", fname);
		return -1;
	}
	count = lseek(fd, 0L, SEEK_END);
	lseek(fd, 0L, SEEK_SET);
	if (read(fd, sigreadbuf, count) != count) {
		printf("read sig from file failed\n");
		return -1;
	}
	if ((sigread = d2i_ECDSA_SIG(NULL, (const unsigned char**)&sigreadbuf, count)) == NULL) {
		printf("cannot convert der struct to sig\n");
		return -1;
	}

	// verify
	ret = ECDSA_do_verify(result, SHA256_DIGEST_LENGTH, sigread, pubeckey);
	if (ret == -1) {
		printf("verify error\n");
	} else if (ret == 0) {
		printf("verify fail, wrong sig\n");
	} else {
		printf("verify success\n");
	}
	EC_KEY_free(priveckey);
	EC_KEY_free(pubeckey);
	return 0;
}

void dumpbuf(const unsigned char* buf, int size) {
	for (int i = 0; i < size; ++i) {
		printf("%02x", buf[i]);
		if (i % 16 == 15) {
			putchar('\n');
		}
	}
}
