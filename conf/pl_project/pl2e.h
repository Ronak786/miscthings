//-------------PS<->PL通信接口-----------
#ifndef PL_H
#define PL_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * initialize api
 * return handler of current map
 * error return -1;
 */
int plInit();

/*
 * uninitialize api
 * no return
 */
void plExit(int fd);
/*
 *功  能:获取随机数
 *参  数:fd  ---[in] 文件描述符
 *       len ---[in] 想要获取的字节长度
 *       ch  ---[out] 输出缓冲区，用于存放获得的随机数
 *返回值:成功返回0,失败返回-1
*/

int genRand( int fd, int len, unsigned char *pbRand);


/*
 *功  能:获取会话密钥
 *参  数:fd  ---[in] 文件描述符
 *       index --- [in] 要获取的会话密钥索引，范围是0--511
		key---[out] 获得的会话密钥，指向一个16字节长度的缓冲区
 *返回值:成功返回0,失败返回-1
*/
int getSessionKey(int fd,int index,unsigned char *key);


/*
 *功  能:设置某个会话密钥
 *参  数:fd  ---[in] 文件描述符
 *       index --- [in] 要设置的会话密钥索引，范围是0--511
		key---[in] 指向一个16字节长度的缓冲区，该缓冲区存放要设置的会话密钥
 *返回值:成功返回0,失败返回-1
*/
int setSessionKey(int fd,int index,unsigned char *key);


/*
 *功  能:获取会话密钥的状态
 *参  数:fd  ---[in] 文件描述符
		 index --- [in] 要获取状态的会话密钥索引，范围是0--511
 *       pst --- [out] 指向存储状态的缓冲区，用于获取的状态，长度是1个字节。0：表示不存在，1：表示存在
 *返回值:成功返回0,失败返回-1
*/
int getSessionKeySt(int fd,int index,unsigned char *pst);


/*
 *功  能:设置会话密钥的状态
 *参  数:fd  ---[in] 文件描述符
		 index --- [in] 要获取状态的会话密钥索引，范围是0--511
 *       st --- [in] 要设置的状态，0：表示不存在，1：表示存在
 *返回值:成功返回0,失败返回-1
*/
int setSessionKeySt(int fd,int index,unsigned char st);

/*
 *功  能:获取某个会话某个私钥的授权信息
 *参  数:fd  ---[in] 文件描述符
		 index --- [in] 要获取私钥授权信息的会话密钥索引，范围是0--511
		 keyindex---[in] 私钥索引，范围0-127
 *       pinfo --- [out] 存放获取到的信息，0：表示没有授权，1：表示已授权
 *返回值:成功返回0,失败返回-1
*/
int getPriKeyAuthInfo(int fd,int index,int keyindex,unsigned char  *pinfo);


/*
 *功  能:设置某个会话某个私钥的授权信息
 *参  数:fd  ---[in] 文件描述符
		 index --- [in] 要获取私钥授权信息的会话密钥索引，范围是0--511
		 keyindex---[in] 私钥索引，范围0-127
 *       info --- [in] 要设置的授权信息，0：表示没有授权，1：表示已授权
 *返回值:成功返回0,失败返回-1
*/
int setPriKeyAuthInfo(int fd,int index,int keyindex,unsigned char  info);



/*
 *功  能:获取SM2签名密钥
 *参  数:fd  ---[in] 文件描述符
		 index ----[in]密钥索引号
		 pst --- [out] 指向一个字节的缓冲区，存放要获取的密钥的状态，0：不存在，1：存在
		 prikey---[out] 指向32字节的缓冲区,存放要获取的私钥
 *       pubkey --- [out] 指向64字节的缓冲区，存放要获取的公钥
 *返回值:成功返回0,失败返回-1
*/
int getSm2SignKey(int fd,int index,unsigned char *pst,unsigned char *prikey,unsigned char  *pubkey);



/*
 *功  能:设置SM2签名密钥
 *参  数:fd  ---[in] 文件描述符
		 index ----[in]密钥索引号	
		 st --- [in] 密钥的状态，0：不存在，1：存在
		 prikey---[in] 指向32字节的缓冲区,存放私钥
 *       pubkey --- [in] 指向64字节的缓冲区，存放公钥
 *返回值:成功返回0,失败返回-1
*/
int setSm2SignKey(int fd,int index,unsigned char st,unsigned char *prikey,unsigned char  *pubkey);



/*
 *功  能:获取SM2加密密钥
 *参  数:fd  ---[in] 文件描述符
		 index ----[in]密钥索引号
		 pst --- [out] 指向一个字节的缓冲区，存放要获取的密钥的状态，0：不存在，1：存在
		 prikey---[out] 指向32字节的缓冲区,存放要获取的私钥
 *       pubkey --- [out] 指向64字节的缓冲区，存放要获取的公钥
 *返回值:成功返回0,失败返回-1
*/
int getSm2EnKey(int fd,int index,unsigned char *pst,unsigned char *prikey,unsigned char  *pubkey);



/*
 *功  能:设置SM2加密密钥
 *参  数:fd  ---[in] 文件描述符
		 index ----[in]密钥索引号	
		 st --- [in] 密钥的状态，0：不存在，1：存在
		 prikey---[in] 指向32字节的缓冲区,存放私钥
 *       pubkey --- [in] 指向64字节的缓冲区，存放公钥
 *返回值:成功返回0,失败返回-1
*/
int setSm2EnKey(int fd,int index,unsigned char st,unsigned char *prikey,unsigned char  *pubkey);


#ifdef __cplusplus
}
#endif

#endif
//PL与主机端的通信接口:给我两个通道的读写，一个低速的，一个高速度。
