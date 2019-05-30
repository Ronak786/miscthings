#ifndef _PCIE_H_
#define _PCIE_H_

#define ERR_PARAMETER           0x80561001
#define ERR_NOTSUPPORTED        0x80561002
#define ERR_DMAREAD_OVERTIME    0x80561003
#define ERR_DMAWRITE_OVERTIME   0x80561004
#define ERR_READFLAG            0x80562005
#define ERR_DMAREADLENGTH       0x80562006
#define ERR_UNLOCK              0x80562007

#define		PCIE_MAJOR       	66
#define		PCIE_NAME		"pcie56"
#define 	JMK_BUFFER_SIZE		(65*1024)

//huang bo version1
#define	VENDOR_ID_V1	0x10EE
#define	DEVICE_ID_V1	0x7024

//huang bo version2
#define	VENDOR_ID_V2	0x5644
#define	DEVICE_ID_V2	0x1509

#define MAX_COMMAND_BUFF_LENGTH		10240

#define SEM_COMM 0
#define SEM_SYM0501 1
#define SEM_SYM1005 2
#define SEM_HASH 3
#define SEM_DMAREAD 4
#define SEM_DMAWRITE 5
#define SET_BAR 0x10

struct recv_descriptor{
	u32 PhAddr_low;				//send buffer busical address 0 -31  bit ;the bit[1:0] must be '00'
	u32 PhAddr_hig;				//send buffer busical address 32 - 63 bit
	u32 status;					//the bit[32] ='1',the buffer had been filled;the bit[0-31] is the length of the buffer had receved
	u32 NextDesc_low;				//next descriptor address 0-31bit  ;if bit[0]=1,this the end descriptor
	u32 NextDesc_hig;				//nest descriptor address 32-63bit
	u32 RecvFlagtag;
	u32 RecvFlagtag1;
	u32 RecvFlagtag2;
	}*recv_list,*sf_list;
//send descriptor list for S/G DMA
struct send_descriptor{
	u32 PhAddr_low;				//send buffer busical address 0-31 bit ;the bit[1:0] must be '00'
	u32 PhAddr_hig;				//send buffer busical address 32-63 bit
	u32 length;					// the send buffer length
	u32 NextDesc_low;				//next descriptor address 0-31bit  ;if bit[0]=1,this the end descriptor
	u32 NextDesc_hig;				//nest descriptor address 32-63bit
	u32 SendFlagtag;
	u32 SendFlagtag1;
	u32 SendFlagtag2;
	}*send_list;

struct sem_driver
{
	struct semaphore sem;
	pid_t pid;
	int used;//1 for used, 0 for unused
};

struct pcie_conf
{
	char *dmaWRBuffer;
	long long dmaWRBufAddr;
	char *dmaRDBuffer;
	long long dmaRDBufAddr;
	char *dmaWRBuffercommand;
	long long dmaWRBufAddrcommand;
	char *dmaRDBuffercommand;
	long long dmaRDBufAddrcommand;
	char *descWRBuffer;
	long long descWRAddr;
	char *descRDBuffer;
	long long descRDAddr;
	char *bar[6];
};

struct pciedata
{
	int type;//0 for dma, 1 for reg, 2 for command
	int regaddr;
	int length;
	char *data;
};

struct Communication	
{
	char sourceid;
	char destinationid;
	short project;	
	short packagelength;
	short returnlength;
	char subcommand;
	char algno;	
	short command;
	int loop;	
	short key0_num;
	char key0_reserve;
	unsigned key0_len:6;
	unsigned key0_en:2;
	short key1_num;
	char key1_reserve;	
	unsigned key1_len:6;
	unsigned key1_en:2;	
	long long int reserve;
};
#endif

