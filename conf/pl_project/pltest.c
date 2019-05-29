#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include "pl2e.h"
#include "plinternal.h"

#define MAP_MASK (4096-1)
#define WORD 4
#define PLSTART 0x40000000
#define PLLEN 0xC000
#define WNGOFF 0x0
#define WNGLEN 0x2000
#define WNGUNIT 4
#define SESSIONOFF 0x2000
#define SESSIONLEN 0x2000
#define SESSIONUNIT 16
#define SESSIONSTOFF 0x4000
#define SESSIONSTLEN 0x2000
#define SESSIONSTUNIT 4  // read status is 1 byte, but read is 4byte, so read 4
#define PRIVKEYAUTHOFF 0x6000
#define PRIVKEYAUTHLEN 0x2000
#define PRIVKEYAUTHUNIT 16
#define SM2SIGNOFF 0x8000
#define SM2SIGNLEN 0x2000
#define SM2SIGNUNIT 100 // (1+8+16)*4
#define SM2ENKEYOFF 0xA000
#define SM2ENKEYLEN 0x2000
#define SM2ENKEYUNIT 100 // (1+8+16)*4

void printBuf(unsigned char* buf, int len) {
	for (int i = 0; i < len; i++) {
		printf("%02x ", (unsigned char)buf[i]);
		if (i % 16 == 15)
			putchar('\n');
	}
	putchar('\n');
}

int fillRandom(int fd) {
#ifdef __TEST
	MapInfo *info = (MapInfo*)fd;
	unsigned char testbuf[20];
	int rfd = open("/dev/urandom", O_RDONLY);
	if (rfd == -1) {
		Fatal(-1, "can not fill\n");
	}
	if (read(rfd, info->mapBase, info->mapLen) != info->mapLen) {
		Log("read random fail\n");
	}
	Log("fill completed\n");
	close(rfd);
#endif
	return 0;
}

int testRandom(int fd) {
	unsigned char randbuf[20] = {0};
	if (genRand(fd, sizeof(randbuf), randbuf) == -1) {
		Log("error genRand\n");
	} else {
		Log("get result:\n");
		printBuf(randbuf, sizeof(randbuf));
	}
	return 0;
}

int testSessionKey(int fd) {
	int loop = 1;
	unsigned char keybuf[17] = "1234567887654321";
	unsigned char recvbuf[17] = {0};
	srand(time(0));
	for (int i = 0; i < loop; i++) {
		Log("i is %d\n", i);
		int index = rand() % (SESSIONLEN / SESSIONUNIT);
		Log("rand index %d\n", index);
		memset(recvbuf, 0, sizeof(recvbuf));
		if (setSessionKey(fd,index,keybuf) == -1) {
			Log("error set sessionkey\n");
		} else {
			Log("have written:\n");
			printBuf(keybuf, sizeof(keybuf)-1);
		}
		if (getSessionKey(fd,index,recvbuf) == -1) {
			Log("error get sessionkey\n");
		} else {
			Log("get result:\n");
			printBuf(recvbuf, sizeof(recvbuf)-1);
		}
	}
	return 0;
}

int testSessionKeySt(int fd) {
	int loop = 1;
	unsigned char key = 1;
	unsigned char recvkey = 0xf; //default
	srand(time(0));
	for (int i = 0; i < loop; i++) {
		Log("i is %d\n", i);
		int index = rand() % (SESSIONLEN / SESSIONUNIT);
		key = rand() % 2;
		Log("rand index %d\n", index);
		recvkey = 0xf;
		if (setSessionKeySt(fd,index,key) == -1) {
			Log("error set sessionkey state\n");
		} else {
			Log("have written: %02x\n", (unsigned char)key);
		}
		if (getSessionKeySt(fd,index,&recvkey) == -1) {
			Log("error get sessionkey state\n");
		} else {
			Log("get result: %02x\n", recvkey);
		}
	}
	return 0;
}

int testPriAuthInfo(int fd) {
	int loop = 1;
	unsigned char key = 1;
	unsigned char recvkey = 0xf; //default
	srand(time(0));
	for (int i = 0; i < loop; i++) {
		Log("i is %d\n", i);
		int index = rand() % 512;
		int keyindex = rand() % 128;
		key = rand() % 2;
		Log("rand index %d, keyindex is %d, set key is %d\n", index, keyindex, key);
		recvkey = 0xf;
		if (setPriKeyAuthInfo(fd,index,keyindex,key) == -1) {
			Log("error set authinfo\n");
		} else {
			Log("have written: %02x\n", (unsigned char)key);
		}
		if (getPriKeyAuthInfo(fd,index,keyindex, &recvkey) == -1) {
			Log("error get authinfo\n");
		} else {
			Log("get result: %02x\n", recvkey);
		}
	}
	return 0;
}

int testSm2SignKey(int fd) {
	int loop = 1;
	unsigned char privkey[33] = 
		"1234567887654321"
		"1234567887654321";
	unsigned char pubkey[65] = 
		"1234567887654321"
		"1234567887654321"
		"1234567887654321"
		"1234567887654321";
	unsigned char recvpst, recvprivkey[33], recvpubkey[65];
	srand(time(0));
	for (int i = 0; i < loop; i++) {
		Log("i is %d\n", i);
		int index = rand() % 64;
		unsigned char pst = rand() % 2;
		Log("rand index %d, pst is %d\n", index, pst);
		recvpst = 0;
		memset(recvprivkey, 0, sizeof(recvprivkey));
		memset(recvpubkey, 0, sizeof(recvpubkey));
		if (setSm2SignKey(fd,index,pst,privkey, pubkey) == -1) {
			Log("error set sm2signkey\n");
		} else {
			Log("have written pst: %02x\n", (unsigned char)pst);
		}
		if (getSm2SignKey(fd,index,&recvpst, recvprivkey, recvpubkey) == -1) {
			Log("error get sm2signkey\n");
		} else {
			Log("get result: %02x\n", recvpst);
			printBuf(recvprivkey, sizeof(recvprivkey)-1);
			printBuf(recvpubkey, sizeof(recvpubkey)-1);
		}
	}
	return 0;
}

int testSm2EnKey(int fd) {
	int loop = 1;
	unsigned char privkey[33] = 
		"1234567887654321"
		"1234567887654321";
	unsigned char pubkey[65] = 
		"1234567887654321"
		"1234567887654321"
		"1234567887654321"
		"1234567887654321";
	unsigned char recvpst, recvprivkey[33], recvpubkey[65];
	srand(time(0));
	for (int i = 0; i < loop; i++) {
		Log("i is %d\n", i);
		int index = rand() % 64;
		unsigned char pst = rand() % 2;
		Log("rand index %d, pst is %d\n", index, pst);
		recvpst = 0;
		memset(recvprivkey, 0, sizeof(recvprivkey));
		memset(recvpubkey, 0, sizeof(recvpubkey));
		if (setSm2EnKey(fd,index,pst,privkey, pubkey) == -1) {
			Log("error set sm2enkey\n");
		} else {
			Log("have written: %02x\n", (unsigned char)pst);
		}
		if (getSm2EnKey(fd,index,&recvpst, recvprivkey, recvpubkey) == -1) {
			Log("error get sm2enkey\n");
		} else {
			Log("get result: %02x\n", recvpst);
			printBuf(recvprivkey, sizeof(recvprivkey)-1);
			printBuf(recvpubkey, sizeof(recvpubkey)-1);
		}
	}
	return 0;
}

int generalTest(int fd) {
	fillRandom(fd);

	Log("\ntest random\n");
	testRandom(fd);

	sleep(1);
	Log("\ntest sessionkey\n");
	testSessionKey(fd);

	sleep(1);
	Log("\ntest session key state\n");
	testSessionKeySt(fd);

	sleep(1);
	Log("\ntest auth info\n");
	testPriAuthInfo(fd);

	sleep(1);
	Log("\ntest sm2signkey\n");
	testSm2SignKey(fd);

	sleep(1);
	Log("\ntest sm2enkey\n");
	testSm2EnKey(fd);
}

void showHelp(void) {
	Log(""
   "-f fill random data\n"
   "-sr  session read\n"
   "-sw  sessoin write\n"
   "-str session state read\n"
   "-stw session state write\n"
   "-ar  auth read\n"
   "-aw  auth write\n"
   "-ssr  sm2 sign read\n"
   "-ssw  sm2 sign write\n"
   "-ser sm2 en read\n"
   "-sew sm2 en write\n"
   "-g   general random test(as a whole)\n"
   );
}

/*
 * args: 
 *	-f fill random data
 *  -sr  session read
 *  -sw  sessoin write
 *  -str session state read
 *  -stw session state write
 *  -ar  auth read
 *  -aw  auth write
 *  -ssr  sm2 sign read
 *  -ssw  sm2 sign write
 *  -ser sm2 en read
 *  -sew sm2 en write
 *  -g   general random test(as a whole)
 */
int main(int ac, char *av[]) {
	if (ac < 2) {
		showHelp();
		Fatal(-1, "at least two args\n");
	}
	int fd = plInit();
	Log("init successed %x\n", fd);
	if (fd == -1){
		Fatal(-1, "can not init\n");
	}

	// an array of function with av as arg
	// correspond flags, then in func deal 
	// special args
	if (!strcmp("-f", av[1])) {
		fillRandom(fd);
	} else if (!strcmp("-sw", av[1])) {
		if (ac != 3) {
			Log("need index\n");
		} else {
			unsigned char keybuf[17] = "1234567887654321";
			int index = atoi(av[2]);
			if (setSessionKey(fd, index, keybuf) == -1) {
				Log("set session key error\n");
			}
		}
	} else if (!strcmp("-sr", av[1])) {
		if (ac != 3) {
			Log("need index\n");
		} else {
			unsigned char keybuf[17] = {0};
			int index = atoi(av[2]);
			if (getSessionKey(fd, index, keybuf) == -1) {
				Log("get session key error\n");
			} else {
				Log("get keybuf result:\n");
				printBuf(keybuf, sizeof(keybuf)-1);
			}
		}
	} else if (!strcmp("-stw", av[1])) {
		if (ac != 4) {
			Log("need index state\n");
		} else {
			int index = atoi(av[2]);
			unsigned char key = atoi(av[3]);
			if (setSessionKeySt(fd, index, key) == -1) {
				Log("set session key error\n");
			}
		}
	} else if (!strcmp("-str", av[1])) {
		if (ac != 3) {
			Log("need index\n");
		} else {
			int index = atoi(av[2]);
			unsigned char key = 0xf;
			if (getSessionKeySt(fd, index, &key) == -1) {
				Log("get session key error\n");
			} else {
				Log("get keybuf result: %02x\n", key);
			}
		}
	} else if (!strcmp("-aw", av[1])) {
		if (ac != 5) {
			Log("need index keyindex state\n");
		} else {
			int index = atoi(av[2]);
			int keyindex = atoi(av[3]);
			unsigned char key = atoi(av[4]);
			if (setPriKeyAuthInfo(fd, index,keyindex, key) == -1) {
				Log("set session key error\n");
			}
		}
	} else if (!strcmp("-ar", av[1])) {
		if (ac != 4) {
			Log("need index keyindex\n");
		} else {
			int index = atoi(av[2]);
			int keyindex = atoi(av[3]);
			unsigned char key = 0xf;
			if (getPriKeyAuthInfo(fd, index, keyindex, &key) == -1) {
				Log("get session key error\n");
			} else {
				Log("get keybuf result: %02x\n", key);
			}
		}
	} else if (!strcmp("-ssw", av[1])) {
		if (ac != 4) {
			Log("need index pst\n");
		} else {
			unsigned char privkey[33] = 
				"1234567887654321"
				"1234567887654321";
			unsigned char pubkey[65] = 
				"1234567887654321"
				"1234567887654321"
				"1234567887654321"
				"1234567887654321";
			int index = atoi(av[2]);
			unsigned char pst = atoi(av[3]);
			if (setSm2SignKey(fd, index,pst, privkey, pubkey) == -1) {
				Log("set session key error\n");
			}
		}
	} else if (!strcmp("-ssr", av[1])) {
		if (ac != 3) {
			Log("need index\n");
		} else {
			int index = atoi(av[2]);
			unsigned char pst = 0xf;
			unsigned char recvpst, recvprivkey[33], recvpubkey[65];
			if (getSm2SignKey(fd, index, &pst, recvprivkey, recvpubkey) == -1) {
				Log("get session key error\n");
			} else {
				Log("get keybuf result: %02x\n", pst);
				printBuf(recvprivkey, sizeof(recvprivkey)-1);
				printBuf(recvpubkey, sizeof(recvpubkey)-1);
			}
		}
	} else if (!strcmp("-sew", av[1])) {
		if (ac != 4) {
			Log("need index pst\n");
		} else {
			unsigned char privkey[33] = 
				"1234567887654321"
				"1234567887654321";
			unsigned char pubkey[65] = 
				"1234567887654321"
				"1234567887654321"
				"1234567887654321"
				"1234567887654321";
			int index = atoi(av[2]);
			unsigned char pst = atoi(av[3]);
			if (setSm2EnKey(fd, index,pst, privkey, pubkey) == -1) {
				Log("set session key error\n");
			}
		}
	} else if (!strcmp("-ser", av[1])) {
		if (ac != 3) {
			Log("need index\n");
		} else {
			int index = atoi(av[2]);
			unsigned char pst = 0xf;
			unsigned char recvpst, recvprivkey[33], recvpubkey[65];
			if (getSm2EnKey(fd, index, &pst, recvprivkey, recvpubkey) == -1) {
				Log("get session key error\n");
			} else {
				Log("get keybuf result: %02x\n", pst);
				printBuf(recvprivkey, sizeof(recvprivkey)-1);
				printBuf(recvpubkey, sizeof(recvpubkey)-1);
			}
		}
	} else if (!strcmp("-g", av[1])) {
		generalTest(fd);
	} else {
		Log("unknown argument %s\n", av[1]);
		showHelp();
	}

	plExit(fd);
	return 0;
}
