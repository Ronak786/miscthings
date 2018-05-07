#ifndef TOOL_H
#define TOOL_H

#ifdef  BUILDING
#include "debug.h" 
#define DLLPORT __declspec(dllexport)
#else    
#define DLLPORT __declspec(dllimport)
#endif

/*
 * error code
 */
typedef int		Nkrypto_Status;

#define E_OK			0	 // success
#define E_UNKNOWN		-1	 // unknown error
#define E_NOMEMORY		-2   // memory alloc fail
#define E_DEVINIT		-3   // init device fail
#define E_DEVNOTFOUND	-4   // can not enumerate device
#define E_THREAD		-5   // thread start fail
#define E_START			-6   // can not start work
#define E_SMALL			-7   // buffer size not enough
#define E_NULLPTR		-8   // null pointer passed in
#define E_NOIMAGE		-9   // image data currently not available, possibly capture thread is not running
#define E_DLL			-10  // can not load dll, possibly file or its dependency not found
#define E_CONFNOTFOUND  -11  // yml configuration file not found
#define E_DECODE		-12  // decode serial number error

/*
 * init camere and do some prestart work
 * success return E_OK
 * see above macros for error code interpretation
 */
extern "C" DLLPORT Nkrypto_Status Nkrypto_Init();

/*
 * start camera and background process
 * success return E_OK
 * see above macros for error code interpretation
 */
extern "C" DLLPORT Nkrypto_Status Nkrypto_Open(); 

/*
 * close camera and stop background process
 * success return E_OK
 * see above macros for error code interpretation
 */
extern "C" DLLPORT Nkrypto_Status Nkrypto_Close(); 

/*
 * capture one image and return image data
 * success return the size copied
 * see above macros for error code interpretation
 *
 * if error is E_SMALL, then param "size" will store the  
 * smallest buffer size needed 
 */
extern "C" DLLPORT int Nkrypto_GetImageData(unsigned char* recvBuf, int size);

/*
 * get serial number from captured image
 * success return E_OK
 * see above macros for error code interpretation
 *
 * every call of this function should follow one call of Nkrypto_GetImageData,
 * otherwise serial number will not change
 *
 * if error is E_SMALL, then param "size" will store the  
 * smallest buffer size needed 
 */
extern "C" DLLPORT int Nkrypto_GetSerialNum(unsigned char *recvBuf, int size);

#endif
