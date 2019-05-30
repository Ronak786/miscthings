/*
 * can all be done via memcpy ??  4-byte align assign realy??
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/mman.h>
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


// return fd
int plInit() {
	MapInfo *info; 
	unsigned char* mapBase;
	int fd = 0;;
	if (!(info = malloc(sizeof(MapInfo)))) {
		return -1;
	}
	memset(info, 0, sizeof(MapInfo));

    
#ifdef __TEST
    if((fd = open("/dev/loop0", O_RDWR | O_SYNC)) == -1) {
		Fatal(-1, "can not communicate with loop\n");
	}
    Log("/dev/loop0 opened.\n"); 
    fflush(stdout);
    mapBase = mmap(0, PLLEN, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (mapBase == (void*)-1) {
		Fatal(-1, "test mmap return %s\n", strerror(errno));
	}
#else
    if((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1) {
		Fatal(-1, "can not communicate with mem\n");
	}
    Log("/dev/mem opened.\n"); 
    fflush(stdout);
    /* Map one page */
    mapBase = mmap(0, PLLEN, PROT_READ | PROT_WRITE, MAP_SHARED, fd, PLSTART & ~MAP_MASK);
	if (mapBase == (void*)-1) {
		Fatal(-1, "mmap return %s\n", strerror(errno));
	}
#endif

	info->mapBase = mapBase;
	info->mapLen = PLLEN;
	info->fd = fd;
	info->pid = getpid();
//	Log("init completed for process %d\n", (int)info->pid);
	return (int)info;
}

void plExit(int fd) {
	MapInfo *info = (MapInfo*)fd;
	if (info) {
		if(munmap(info->mapBase, info->mapLen) == -1) {
			Log("can not exit normally\n");
		}
		close(info->fd);
		free(info);
		info = NULL;
	}
}

int genRand(int fd, int len, unsigned char *pbRand) {
	unsigned int *uintptr = (unsigned int*)pbRand;
	unsigned char *charptr = NULL;
	int curReadLen = 0;
	MapInfo *info = (MapInfo*)fd;
	if (!info) {
		Log("invalid fd in %s\n", __FUNCTION__);
		return -1;
	}
	if (len > WNGLEN) {
		Fatal(-1, "request data size too large(> %d)\n", WNGLEN);
	}
	if (!pbRand) {
		Fatal(-1, "buffer passed in is NULL\n");
	}

	/*
	// should test this range of code !!
	// read every 4-byte per operation
	for (int i = 0; i < len / WORD; i++, uintptr++) {
		*uintptr = *(unsigned int*)(info->mapBase + WNGOFF + i * WORD);
	}

	// read remaining bytes
	charptr = (unsigned int*)uintptr;
	curReadLen = len / WORD * WORD;
	for (int i = 0; i < len % WORD; i++, charptr++) {
		*charptr = *(unsigned char*)(info->mapBase + WNGOFF + curReadLen + i); 
	}
	// --should test this range of code !!--
	*/
	memcpy(pbRand, info->mapBase + WNGOFF, len);
	return 0;
}

int getSessionKey(int fd,int index,unsigned char *key) {
	unsigned int *uintptr = (unsigned int*)key;
	MapInfo *info = (MapInfo*)fd;
	if (!info) {
		Log("invalid fd in %s\n", __FUNCTION__);
		return -1;
	}
	if (index >= SESSIONLEN / SESSIONUNIT) {
		Fatal(-1, "index out of range, max is %d\n", SESSIONLEN / SESSIONUNIT-1);
	}
	if (!key) {
		Fatal(-1, "buffer passed in is NULL\n");
	}

	int offset = index * SESSIONUNIT;
	Log("offset is %x %x\n", offset, SESSIONOFF + offset);
	for (int i = 0; i < SESSIONUNIT / WORD; i++, uintptr++) {
		*uintptr = *(unsigned int*)(info->mapBase + SESSIONOFF + offset + i * WORD);

	}
//	memcpy(key, info->mapBase + SESSIONOFF + offset, SESSIONUNIT);
	return 0;
}

int setSessionKey(int fd,int index,unsigned char *key){
	MapInfo *info = (MapInfo*)fd;
	unsigned int *uintptr = (unsigned int*)key;
	if (!info) {
		Log("invalid fd in %s\n", __FUNCTION__);
		return -1;
	}
	if (index >= SESSIONLEN / SESSIONUNIT) {
		Fatal(-1, "index out of range, max is %d\n", SESSIONLEN / SESSIONUNIT-1);
	}
	if (!key) {
		Fatal(-1, "buffer passed in is NULL\n");
	}
	int offset = index * SESSIONUNIT;
	Log("offset is %x %x\n", offset, SESSIONOFF + offset);
	/*
	for (int i = 0; i < SESSIONUNIT / WORD; i++, uintptr++) {
	*(unsigned int*)(info->mapBase + SESSIONOFF + offset + i * WORD) = *uintptr;

	}
	*/
	memcpy(info->mapBase + SESSIONOFF + offset, key, SESSIONUNIT);
	return 0;
}

int getSessionKeySt(int fd,int index,unsigned char *pst){
	unsigned int *uintptr = (unsigned int*)pst;
	MapInfo *info = (MapInfo*)fd;
	if (!info) {
		Log("invalid fd in %s\n", __FUNCTION__);
		return -1;
	}
	if (index >= SESSIONSTLEN / SESSIONSTUNIT) {
		Fatal(-1, "index out of range, max is %d\n", SESSIONSTLEN / SESSIONSTUNIT-1);
	}
	if (!pst) {
		Fatal(-1, "buffer passed in is NULL\n");
	}
	int offset = index * SESSIONSTUNIT;
	Log("offset is %x %x\n", offset, SESSIONSTOFF + offset);
	/*
	for (int i = 0; i < SESSIONSTUNIT / WORD; i++, uintptr++) {
		*uintptr = *(unsigned int*)(info->mapBase + SESSIONSTOFF + offset + i * WORD);

	}
	*/
	// care of edian problem
	*pst = *(unsigned int*)(info->mapBase + SESSIONSTOFF + offset);
	return 0;
}

int setSessionKeySt(int fd,int index,unsigned char st) {
	MapInfo *info = (MapInfo*)fd;
	unsigned int uintst = st;
	unsigned int *uintptr = &uintst;
	if (!info) {
		Log("invalid fd in %s\n", __FUNCTION__);
		return -1;
	}
	if (index >= SESSIONSTLEN / SESSIONSTUNIT) {
		Fatal(-1, "index out of range, max is %d\n", SESSIONSTLEN / SESSIONSTUNIT-1);
	}
	
	int offset = index * SESSIONSTUNIT;
	Log("offset is %x %x\n", offset, SESSIONSTOFF + offset);
	/*
	for (int i = 0; i < SESSIONSTUNIT / WORD; i++, uintptr++) {
		*(unsigned int*)(info->mapBase + SESSIONSTOFF + offset + i * WORD) = *uintptr;

	}
	*/
	// copy 4byte at least
	memcpy(info->mapBase + SESSIONSTOFF + offset, uintptr, SESSIONSTUNIT);
	return 0;
}

/*
 *功  能:获取某个会话某个私钥的授权信息
 *参  数:fd  ---[in] 文件描述符
		 index --- [in] 要获取私钥授权信息的会话密钥索引，范围是0--511
		 keyindex---[in] 私钥索引，范围0-127
 *       pinfo --- [out] 存放获取到的信息，0：表示没有授权，1：表示已授权
 *返回值:成功返回0,失败返回-1
*/
int getPriKeyAuthInfo(int fd,int index,int keyindex,unsigned char  *pinfo) {
	MapInfo *info = (MapInfo*)fd;
	if (!info) {
		Log("invalid fd in %s\n", __FUNCTION__);
		return -1;
	}
	if (index >= PRIVKEYAUTHLEN / PRIVKEYAUTHUNIT) {
		Fatal(-1, "index out of range, max is %d\n", PRIVKEYAUTHLEN / PRIVKEYAUTHUNIT-1);
	}
	if (keyindex >= 128) {
		Fatal(-1, "keyindex out of range [0-127]\n");
	}
	if (!pinfo) {
		Fatal(-1, "buffer passed in is NULL\n");
	}
	// read should align 32bit, 128 / 32 = which
	int offset = index * PRIVKEYAUTHUNIT;
	unsigned int whichint = keyindex / 32;
	unsigned int whichbitinint = keyindex % 32;
	unsigned char readbuf[PRIVKEYAUTHUNIT];
	Log("offset is %x %x %x\n", offset, PRIVKEYAUTHOFF + offset + whichint * 4, whichbitinint);
	/*
	// only read the least char of least int  of total 4-int
	memcpy(readbuf, info->mapBase + PRIVKEYAUTHOFF + offset, PRIVKEYAUTHUNIT);
	*pinfo = !!(readbuf[keyindex/8] & (1 << keyindex % 8))
	*/
	*pinfo = !!(*(unsigned int*)(info->mapBase + PRIVKEYAUTHOFF + offset + whichint * 4) 
				& (1 << whichbitinint));
	return 0;
}


/*
 *功  能:设置某个会话某个私钥的授权信息
 *参  数:fd  ---[in] 文件描述符
		 index --- [in] 要获取私钥授权信息的会话密钥索引，范围是0--511
		 keyindex---[in] 私钥索引，范围0-127
 *       info --- [in] 要设置的授权信息，0：表示没有授权，1：表示已授权
 *返回值:成功返回0,失败返回-1
*/
int setPriKeyAuthInfo(int fd,int index,int keyindex,unsigned char  cinfo) {
	MapInfo *info = (MapInfo*)fd;
	if (!info) {
		Log("invalid fd in %s\n", __FUNCTION__);
		return -1;
	}
	if (index >= PRIVKEYAUTHLEN / PRIVKEYAUTHUNIT) {
		Fatal(-1, "index out of range, max is %d\n", PRIVKEYAUTHLEN / PRIVKEYAUTHUNIT-1);
	}
	if (keyindex >= 128) {
		Fatal(-1, "keyindex out of range [0-127]\n");
	}
	unsigned int offset = index * PRIVKEYAUTHUNIT;
	unsigned int whichint = keyindex / 32;
	unsigned int whichbitinint = keyindex % 32;
	Log("offset is %x %x %x\n", offset, PRIVKEYAUTHOFF + offset + whichint * 4, whichbitinint);

	// set must align 4-byte boundary
	*(unsigned int*)(info->mapBase + PRIVKEYAUTHOFF + offset + whichint * 4) &= 
			~(1 << whichbitinint);  // clear 0
	*(unsigned int*)(info->mapBase + PRIVKEYAUTHOFF + offset + whichint * 4) |=
			cinfo << whichbitinint;  // set 0 or 1
	return 0;
}



/*
 *功  能:获取SM2签名密钥
 *参  数:fd  ---[in] 文件描述符
		 index ----[in]密钥索引号
		 pst --- [out] 指向一个字节的缓冲区，存放要获取的密钥的状态，0：不存在，1：存在
		 prikey---[out] 指向32字节的缓冲区,存放要获取的私钥
 *       pubkey --- [out] 指向64字节的缓冲区，存放要获取的公钥
 *返回值:成功返回0,失败返回-1
*/
int getSm2SignKey(int fd,int index,unsigned char *pst,unsigned char *prikey,unsigned char  *pubkey) {
	MapInfo *info = (MapInfo*)fd;
	if (!info) {
		Log("invalid fd in %s\n", __FUNCTION__);
		return -1;
	}
	// this not use up mem
	if (index >= SM2SIGNLEN / SM2SIGNUNIT) {
		Fatal(-1, "index out of range, max is %d\n", SM2SIGNLEN / SM2SIGNUNIT-1);
	}
	if (!pst || !prikey || !pubkey) {
		Fatal(-1, "buffer passed in NULL\n");
	}
	int offset = index * SM2SIGNUNIT;
	Log("offset is %x %x \n", offset, SM2SIGNOFF + offset);
	*pst = *(unsigned int*)(info->mapBase + SM2SIGNOFF + offset); // get state
	memcpy(prikey, info->mapBase + SM2SIGNOFF + offset + 4, 32); // get prikey
	memcpy(pubkey, info->mapBase + SM2SIGNOFF + offset + 36, 64); // get pubkey
	return 0;
}



/*
 *功  能:设置SM2签名密钥
 *参  数:fd  ---[in] 文件描述符
		 index ----[in]密钥索引号	
		 st --- [in] 密钥的状态，0：不存在，1：存在
		 prikey---[in] 指向32字节的缓冲区,存放私钥
 *       pubkey --- [in] 指向64字节的缓冲区，存放公钥
 *返回值:成功返回0,失败返回-1
*/
int setSm2SignKey(int fd,int index,unsigned char st,unsigned char *prikey,unsigned char  *pubkey) {
	MapInfo *info = (MapInfo*)fd;
	if (!info) {
		Log("invalid fd in %s\n", __FUNCTION__);
		return -1;
	}
	if (index >= SM2SIGNLEN / SM2SIGNUNIT) {
		Fatal(-1, "index out of range, max is %d\n", SM2SIGNLEN / SM2SIGNUNIT-1);
	}
	if (!prikey || !pubkey) {
		Fatal(-1, "buffer passed in NULL\n");
	}
	int offset = index * SM2SIGNUNIT;
	Log("offset is %x %x \n", offset, SM2SIGNOFF + offset);
	*(unsigned int*)(info->mapBase + SM2SIGNOFF + offset) = (unsigned int)st; // set state
	memcpy(info->mapBase + SM2SIGNOFF + offset + 4, prikey, 32); // set prikey
	memcpy(info->mapBase + SM2SIGNOFF + offset + 36, pubkey, 64); // set pubkey
	return 0;
}

/*
 *功  能:获取SM2加密密钥
 *参  数:fd  ---[in] 文件描述符
		 index ----[in]密钥索引号
		 pst --- [out] 指向一个字节的缓冲区，存放要获取的密钥的状态，0：不存在，1：存在
		 prikey---[out] 指向32字节的缓冲区,存放要获取的私钥
 *       pubkey --- [out] 指向64字节的缓冲区，存放要获取的公钥
 *返回值:成功返回0,失败返回-1
*/
int getSm2EnKey(int fd,int index,unsigned char *pst,unsigned char *prikey,unsigned char  *pubkey) {
	MapInfo *info = (MapInfo*)fd;
	if (!info) {
		Log("invalid fd in %s\n", __FUNCTION__);
		return -1;
	}
	// this not use up mem
	if (index >= SM2ENKEYLEN / SM2ENKEYUNIT) {
		Fatal(-1, "index out of range, max is %d\n", SM2ENKEYLEN / SM2ENKEYUNIT);
	}
	if (!pst || !prikey || !pubkey) {
		Fatal(-1, "buffer passed in NULL\n");
	}
	int offset = index * SM2ENKEYUNIT;
	Log("offset is %x %x \n", offset, SM2ENKEYOFF + offset);
	*pst = *(unsigned int*)(info->mapBase + SM2ENKEYOFF + offset); // get state
	memcpy(prikey, info->mapBase + SM2ENKEYOFF + offset + 4, 32); // get prikey
	memcpy(pubkey, info->mapBase + SM2ENKEYOFF + offset + 36, 64); // get pubkey
	return 0;
}



/*
 *功  能:设置SM2加密密钥
 *参  数:fd  ---[in] 文件描述符
		 index ----[in]密钥索引号	
		 st --- [in] 密钥的状态，0：不存在，1：存在
		 prikey---[in] 指向32字节的缓冲区,存放私钥
 *       pubkey --- [in] 指向64字节的缓冲区，存放公钥
 *返回值:成功返回0,失败返回-1
*/
int setSm2EnKey(int fd,int index,unsigned char st,unsigned char *prikey,unsigned char  *pubkey) {
	MapInfo *info = (MapInfo*)fd;
	if (!info) {
		Log("invalid fd in %s\n", __FUNCTION__);
		return -1;
	}
	if (index >= SM2ENKEYLEN / SM2ENKEYUNIT) {
		Fatal(-1, "index out of range, max is %d\n", SM2ENKEYLEN / SM2ENKEYUNIT);
	}
	if (!prikey || !pubkey) {
		Fatal(-1, "buffer passed in NULL\n");
	}
	int offset = index * SM2ENKEYUNIT;
	Log("offset is %x %x \n", offset, SM2ENKEYOFF + offset);
	*(unsigned int*)(info->mapBase + SM2ENKEYOFF + offset) = (unsigned int)st; // set state
	memcpy(info->mapBase + SM2ENKEYOFF + offset + 4, prikey, 32); // set prikey
	memcpy(info->mapBase + SM2ENKEYOFF + offset + 36, pubkey, 64); // set pubkey
	return 0;
}

