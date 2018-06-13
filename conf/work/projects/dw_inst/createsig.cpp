/*************************************************************************
	> File Name: createsig.cpp
	> Author: davee-x
	> Mail: davee.naughty@gmail.com 
	> Created Time: Wed 13 Jun 2018 10:17:43 AM CST
 ************************************************************************/

/* currently should be use in dir in remotepkgs/ dir !!,otherwise key store not correct */
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include "sigutil.h"

int main(int ac, char *av[]) {
	if (ac !=3) {
		printf("help: %s inputfile sigfilename\n", av[0]);
		return -1;
	}
	unsigned char content[SHA256_DIGEST_LENGTH];
	unsigned char *signature;
	unsigned int siglen;
	const char *privkeypath = "../remotepkgs/privkey.pem";
	const char *pubkeypath = "../localpkgs/pubkey.pem";
	int fd;

	if (get_sha256(av[1], content) == -1) {
		printf("calculate sha256 fail\n");
		return -1;
	}

	// get sigbug length
	siglen = ecdsa_sign(NULL, 0, NULL, NULL, privkeypath, pubkeypath);
	if ((signature = (unsigned char*)OPENSSL_malloc(siglen)) == NULL) {
		printf("can not allocate signature buffer\n");
		return -1;
	}
	
	if (ecdsa_sign(content, SHA256_DIGEST_LENGTH, signature, &siglen, 
				privkeypath, pubkeypath) != 0) {
		printf("sig failed\n");
		return -1;
	}

	/************** write sig into file************/
	printf("begin write sig\n");
	fd = open(av[2], O_WRONLY|O_CREAT|O_TRUNC, 0644);
	if (fd == -1) {
		printf("can not open file %s\n", av[2]);
		return -1;
	}
	if (write(fd, signature, siglen) != siglen) {
		printf("can not write sig into file\n");
		return -1;
	}
	close(fd);
	/**********************************************/

	return 0;
}
