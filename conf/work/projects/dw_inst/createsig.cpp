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

#define READSIZ 1*1024*1024

char *privkeypath = "remotepkgs/privkey.pem";
char *pubkeypath = "localpkgs/pubkey.pem";

static int get_sha256(char *fnamel, unsigned char* result);
static int ecdsa_sign(char *fname, unsigned char* result);

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

	int ret, fd;
	ECDSA_SIG *sig;
	EC_KEY *eckey;
	BIGNUM *privkey;
	EC_POINT *pubkey;
	eckey = EC_KEY_new_by_curve_name(NID_secp192k1);
	if (eckey == NULL) {
		printf("can not create curve\n");
		return -1;
	}
	if (!EC_KEY_generate_key(eckey)) {
		printf("cannot generate key from curve group\n");
		return -1;
	}

	pubkey = EC_KEY_get0_public_key(eckey);
	privkey = EC_KEY_get0_private_key(eckey);

	// convert priv key into char and store
	char *privkeystr = BN_bn2hex(privkey);
	if ((fd = open(privkeypath, O_WRONLY|O_CREAT|O_TRUNC)) == -1) {
		printf("can not open file for priv key store\n");
		return -1;
	}
	int count = strlen(privkeystr);
	if (write(fd, privkeystr, count) != count) {
		printf("error write to file of privkey\n");
		return -1;
	}
	free(privkeystr);
	close(fd);

	// read privkey and sign
	if ((fd = open(privkeypath, O_RDONLY)) == -1) {
		printf("can not open file for priv key read\n");
		return -1;
	}
	int count = lseek(fd, 0L, SEEK_END);
	char *privkeyread = (char*)malloc(count);
	if (privkeyread == NULL) {
		printf("can not alloc mem for priv key read\n");
		return -1;
	}
	if (read(fd, privkeyread, count) != count) {
		printf("can not read privkey from file\n");
		return -1;
	}
	BIGNUM *privbignum = NULL
	if (BN_hex2bn(&privbignum, privkeyread) == 0) {
		printf("create big num fail\n");
		return 0;
	}

	//save pub key
	char *pubkeystr = EC_POINT_point2hex(EC_KEY_get0_group(eckey), EC_KEY_get0_public_key(eckey), 2, NULL);
	if ((fd = open(pubkeypath, O_WRONLY|O_CREAT|O_TRUNC)) == -1) {
		printf("can not open file for pub key store\n");
		return -1;
	}
	int count = strlen(privkeystr);
	if (write(fd, pubkeystr, count) != count) {
		printf("error write to file of pubkey\n");
		return -1;
	}
	free(pubkeystr);
	close(fd);

	// read pubkey and verify ?? not finished
	if ((fd = open(privkeypath, O_RDONLY)) == -1) {
		printf("can not open file for priv key read\n");
		return -1;
	}
	int count = lseek(fd, 0L, SEEK_END);
	char *privkeyread = (char*)malloc(count);
	if (privkeyread == NULL) {
		printf("can not alloc mem for priv key read\n");
		return -1;
	}
	if (read(fd, privkeyread, count) != count) {
		printf("can not read privkey from file\n");
		return -1;
	}
	BIGNUM *privbignum = NULL
	if (BN_hex2bn(&privbignum, privkeyread) == 0) {
		printf("create big num fail\n");
		return 0;
	}

	//sign
	sig = ECDSA_do_sign(result, SHA256_DIGEST_LENGTH, eckey);
	if (sig == NULL) {
		printf("can not generate sig\n");
		return -1;
	}

	ret = ECDSA_do_verify(result, SHA256_DIGEST_LENGTH, sig, eckey);
	if (ret == -1) {
		printf("verify error\n");
	} else if (ret == 0) {
		printf("verify fail, wrong sig\n");
	} else {
		printf("verify success\n");
	}
	return 0;
	/*
	// write sig into file
	int fd = open(av[2], O_WRONLY|O_CREAT|O_TRUNC, 0644);
	if (fd == -1) {
		printf("can not open file %s\n", av[2]);
		return -1;
	}
	if (write(fd, result, SHA256_DIGEST_LENGTH) != SHA256_DIGEST_LENGTH) {
		printf("can not write into digest\n");
		return -1;
	}
	close(fd);
	*/
}
