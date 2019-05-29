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

#ifdef __cplusplus
}
#endif

#endif
