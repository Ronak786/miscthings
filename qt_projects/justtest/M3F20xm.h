
#ifndef		M3F20xm_H
#define		M3F20xm_H
#include <windows.h>

#define DllExport extern "C" __declspec(dllimport)

//#ifdef __cplusplus
//extern "C" {
//#endif

typedef struct
 {
 	/* 
 	 wSPIConfig:
     bit15  for SPI R/W access mode           1- Directed    , 0- DMA
     bit12  for SPI Frame length:             1- 16 bit      , 0- 8 bit
     bit7   for SPI Frame format:             1- LSB first   , 0- MSB first
     bit5~3 for SPI frequency devide factor:  SPI frequecy = SYSCLK / ( factor + 1)
     bit2   for SPI mode:                     1- Master      , 0- Slaver
     bit1   for SPI CLK level when idle:      1- High        , 0- Low
     bit0   for SPI CLK capture:              1- the Second edge,0- the First edge                   
 	*/
  WORD wSPIConfig;       
  /* the delay time after CS becomes low, unit us  */
  WORD wDelayCSLow;    
  /* the delay time after sends command, unit us */  
  WORD wDelayComEnd;      
  /* the delay time after sends or receives a frame */
  WORD wDelayFrameEnd;   
  /* the delay time after sends oe receives all the data */
  WORD wDelayDataEnd;     
  WORD wReserved;         //unused
 }SPI_CONFIG;


typedef struct
 {
 	/*
 	  byTrigCon:
 	    bit7   for Trig staus:                  1- Trig opened,  0- Trig closed
 	    bit0   got Trig type:                   1- IO Trig,      0- Timer Trig 	       
 	*/
 	BYTE  byTrigCon;
	/*
	  byIOTrigOptions:  for IO Trig type
	     0-- raising edge
	     1-- falling edge
	     2-- raising or falling edge
	*/                       
	BYTE  byIOTrigOptions;
                            
	BYTE  byUnused;
	
	/*
	 byActions: the action after trigged
	 0: Execute Read
	 1: Execute write and read
	 2: Execute read and write
	*/
	BYTE  byActions;
  /* dwPeriod: Timer trig period, unit us */
	DWORD dwPeriod;
	/* byRCmdSize: read commamd size, in bytes */
  BYTE  byRCmdSize;
  /* byWCmdSize: write commamd size, in bytes */
	BYTE  byWCmdSize;
	/* byReadCmd: read commad buff */
	BYTE  byReadCmd[16]; 
	/* byWriteCmd: write commad buff */  
  BYTE  byWriteCmd[16];   
  /* wReadSize: read data size, must be less than 3072 */
	WORD wReadSize;
	/* wTrigSize: trig buff size, must be less than 3072
	   it will send data to upport while read data is equal to wTrigSize
	 */
  WORD wTrigSize; 
  /* dwTrigCnt: current trig counter */
  DWORD dwTrigCnt;        
  /* dwMaxCnt: Max Enabled trig number , trig will exit if dwTrigCnt is equal to dwMaxCnt */
    DWORD dwMaxCnt;
 }TRIG_CONFIG;


/* the callback function define for USB hotplug */
 typedef	 VOID (* USB_DLL_CALLBACK )(BYTE iDevIndex, DWORD iDevStatus);


/** \ingroup desc
 * Retrieve a software information from the specified device 
 *
 * \param byIndex:  device index
 * \param byType:   software type; 0- lib, 1- sys, 2- MCU
 * \param lpBuffer: a char pointer to save software information
 * \returns true if call successful or false if call failed
 */
DllExport bool M3F20xm_GetVersion(BYTE byIndex,BYTE byType,char* lpBuffer);


/** \ingroup desc
 * Retrieve the number of devices run a PC in parallel 
 *
 * \param void
 * \returns number of devices
 */
DllExport BYTE M3F20xm_GetMaxNumofDev(void);


/** \ingroup desc
 * Retrieve the serial No.from the specified device 
 *
 * \param byIndex:  device index
 * \param lpBuff: char pointer to save serial No.
 * \returns true if call successful or false if call failed
 */
DllExport BYTE M3F20xm_GetSerialNo(BYTE byIndex, char* lpBuff);


/** \ingroup desc
 * Set a usb hotplug callback function pointer and log switch 
 *
 * \param bLog:  true for log on , false for log off
 * \param pUSB_CallBack: a callback function pointer for usb hotplug
 * \returns true if call successful or false if call failed
 */
DllExport bool  M3F20xm_SetUSBNotify(bool bLog,USB_DLL_CALLBACK pUSB_CallBack);
						   

/** \ingroup desc
 * Retrieve a device index from the specified serial N0. 
 *
 * \param pSerialString: char pointer to save serial No.
 * \returns the device index. if is 0xFF, the device index is invalid
 */
DllExport BYTE M3F20xm_OpenDeviceByNumber(char* pSerialString );


/** \ingroup desc
 * Open a device and get the device index. 
 *
 * \param void
 * \returns the device index. if is 0xFF, the device index is invalid
 */
DllExport  BYTE M3F20xm_OpenDevice(void);


/** \ingroup desc
 * Verify a specified device if registed or not 
 *
 * \param byIndex:   device index
 * \param pResult:   a unsigned char pointer to save the verified result
 * \returns the device index. if is 0xFF, the device index is invalid
 */
DllExport bool M3F20xm_Verify(BYTE byIndex,BYTE* pResult);


/** \ingroup desc
 * force the specified device enter DFU mode
 *
 * \param byIndex:   device index
 * \returns the device index. if is 0xFF, the device index is invalid
 */
DllExport bool M3F20xm_DFUMode(BYTE byIndex);


/** \ingroup desc
 * Close specified device by device index. 
 *
 * \param byIndex:  device index
 * \returns true if call successful or false if call failed
 */
DllExport bool M3F20xm_CloseDevice(BYTE byIndex);


/** \ingroup desc
 * Close specified device by device serial No. 
 *
 * \param pSerialString:  char pointer to save serial No.
 * \returns true if call successful or false if call failed
 */
DllExport bool M3F20xm_CloseDeviceByNumber (char* pSerialString);


/** \ingroup desc
 * Set SPI Config 
 *
 * \param byIndex:  device index
 * \param psConfig: A pointer to save the SPI_CONFIG
 * \returns true if call successful or false if call failed
 */
 DllExport bool M3F20xm_SPISetConfig(BYTE byIndex,SPI_CONFIG* psConfig);


/** \ingroup desc
 * Get SPI Config
 *
 * \param byIndex:  device index
 * \param psConfig: A pointer to save the SPI_CONFIG
 * \returns true if call successful or false if call failed
 */ 
 DllExport bool M3F20xm_SPIGetConfig(BYTE byIndex,SPI_CONFIG* psConfig);
 

  /** \ingroup desc
 * write and read data from SPI port in fulldepux mode
 * Note:
 * 1. wBuffSize must be less than 3072
 * \param byIndex:     device index
 * \param lpWriteBuffer: A pointer to save the command to send to SPI port
 * \param lpReadBuffer:    A pointer to save the read data from SPI port
 * \param wBuffSize:  the data length request for writing and reading
 * \param dwTimeout:   the timeout for read/write data
 * \returns true if call successful or false if call failed
 */
 DllExport bool  M3F20xm_SPITransfer(BYTE byIndex,BYTE* lpWriteBuffer,BYTE* lpReadBuffer,WORD wBuffSize,DWORD dwTimeout);

 /** \ingroup desc
 * Read data from SPI port in DMA  or Directed method
 * Note:
 * 1. byComSize must be less than 56
 * 2. dwBuffSize must be less than 2*1024*1024
 * \param byIndex:     device index
 * \param lpComBuffer: A pointer to save the command to send to SPI port
 * \param byComSize:   the length of lpComBuffer
 * \param lpBuffer:    A pointer to save the read data from SPI port
 * \param dwBuffSize:  the data length request for reading
 * \param dwTimeout:   the timeout for read data
 * \returns true if call successful or false if call failed
 */ 
 DllExport bool M3F20xm_SPIRead(BYTE byIndex,BYTE* lpComBuffer,BYTE  byComSize,BYTE* lpBuffer,DWORD dwBuffSize,DWORD dwTimeout);


 /** \ingroup desc
 * Write data to SPI port in DMA or Directed method
 * Note:
 * 1. byComSize must be less than 56
 * 2. dwBuffSize must be less than 2*1024*1024
 * \param byIndex:     device index
 * \param lpComBuffer: A pointer to save the command to send to SPI port
 * \param byComSize:   the length of lpComBuffer
 * \param lpBuffer:    A pointer to save the write data to SPI port
 * \param dwBuffSize:  the data length requested for writting
 * \param dwTimeout:   the timeout for write data
 * \returns true if call successful or false if call failed
 */  
 DllExport bool M3F20xm_SPIWrite(BYTE byIndex,BYTE* lpComBuffer,BYTE  byComSize,BYTE* lpBuffer,DWORD dwBuffSize,DWORD dwTimeout);


/** \ingroup desc
 * Get GPIO Config
 *
 * \param byIndex:  device index
 * \param pdwValue: A unsigned int pointer to save the GPIO Config
 * \returns true if call successful or false if call failed
 */ 
 DllExport bool M3F20xm_GPIOGetConfig(BYTE byIndex,DWORD* pdwValue);


/** \ingroup desc
 * Set GPIO Config 
 *
 * \param byIndex:  device index
 * \param dwValue:  GPIO config value
 * \returns true if call successful or false if call failed
 */
DllExport bool M3F20xm_GPIOSetConfig(BYTE byIndex,DWORD dwValue);


/** \ingroup desc
 * Write GPIO Port 
 *
 * \param byIndex:  device index
 * \param byValue:  GPIO port value
 * \param byMask:   GPIO Mask value
 * \returns true if call successful or false if call failed
 */
 DllExport bool M3F20xm_GPIOWrite(BYTE byIndex,BYTE byValue,BYTE byMask);


/** \ingroup desc
 * Read GPIO Port 
 *
 * \param byIndex:  device index
 * \param pbyValue: a unsigned char pointer to save GPIO port value
 * \returns true if call successful or false if call failed
 */
DllExport bool M3F20xm_GPIORead(BYTE byIndex,BYTE* pbyValue);


/** \ingroup desc
 * Get Trig Config
 *
 * \param byIndex:  device index
 * \param pCfg:    A pointer to save the Trig Config
 * \returns true if call successful or false if call failed
 */ 
DllExport bool M3F20xm_TrigGetConfig(BYTE byIndex,TRIG_CONFIG* pCfg);


/** \ingroup desc
 * Set Trig Config
 *
 * \param byIndex:  device index
 * \param pCfg:    A pointer to save the Trig Config
 * \returns true if call successful or false if call failed
 */ 

DllExport bool M3F20xm_TrigSetConfig(BYTE byIndex,TRIG_CONFIG* pCfg);


/** \ingroup desc
 * Start trig or stop trig
 *
 * \param byIndex:  device index
 * \param byOn:     trig switch
 * \returns true if call successful or false if call failed
 */ 
DllExport bool M3F20xm_EnableTrig(BYTE byIndex,BYTE byOn);


/** \ingroup desc
 * Initialize FIFO
 *
 * \param byIndex:  device index
 * \returns true if call successful or false if call failed
 */ 
DllExport bool M3F20xm_InitFIFO(BYTE byIndex);


/** \ingroup desc
 * Read data from FIFO
 * Note:

 * \param byIndex:     device index
 * \param lpBuffer:    A pointer to save the read data from FIFO
 * \param dwBuffSize:  the requested length for reading
 * \param pdwReadSize: the actual length of read back
 * \returns true if call successful or false if call failed
 */  
DllExport bool M3F20xm_ReadFIFO(BYTE byIndex,BYTE* lpBuffer,DWORD dwBuffSize,DWORD* pdwReadSize);


/** \ingroup desc
 * Retrieve a un-read data length of FIFO 
 *
 * \param byIndex:      device index
 * \param pdwBuffsize:  a unsigned int pointer to save the un-read data length of FIFO 
 * \returns true if call successful or false if call failed
 */
DllExport bool M3F20xm_GetFIFOLeft(BYTE byIndex,DWORD* pdwBuffsize);

                              
/** \ingroup desc
 * Write data to SPI buff
 * Note:
 * 1. wAddr must be less than 0xC00
 * 2. wAddr +  wBuffSize must be less than 0xC00
 * \param byIndex:     device index
 * \param wAddr:       buff offset
 * \param lpBuffer:    A pointer to save the write data to SPI buff
 * \param wBuffSize:   the data length requested for writting 
 * \param dwTimeout:   the timeout for write buff
 * \returns true if call successful or false if call failed
 */  
DllExport bool M3F20xm_BuffWrite(BYTE byIndex,WORD wAddr,BYTE* lpBuffer,WORD wBuffSize,DWORD dwTimeout);


/** \ingroup desc
 * Read data from SPI buff
 * Note:
 * 1. wAddr must be less than 0xC00
 * 2. wAddr +  wBuffSize must be less than 0xC00
 * \param byIndex:     device index
 * \param wAddr:       buff offset
 * \param lpBuffer:    A pointer to save the read data from SPI buff
 * \param wBuffSize:   the data length requested for reading 
 * \param dwTimeout:   the timeout for read buff
 * \returns true if call successful or false if call failed
 */  
DllExport bool M3F20xm_BuffRead(BYTE byIndex,WORD wAddr,BYTE* lpBuffer,WORD wBuffSize,DWORD dwTimeout);


//#ifdef __cplusplus
//}
//#endif

#endif		
