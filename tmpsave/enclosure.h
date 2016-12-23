#ifndef PATH_H
#define PATH_H

#include <stdint.h>

#define		LEN		80	
#define		SLOTNUM		12
#define		SASNUM		20
#define		VENDOR		"LSI"
#define		SKIPNUM		16
#define		GB			1000000000

enum dev_state {
	OFFLINE,
	ONLINE,
	EMPTY,
};

struct sys_sas_dev {
	uint64_t block;
	uint64_t sas_address;
	enum dev_state  state;
	char devname[LEN];
};

struct enclosure {
	uint64_t size;  // in GB
	enum dev_state  state; //0 1 2
	char devname[LEN];
};

#endif
