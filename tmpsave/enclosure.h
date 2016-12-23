#ifndef PATH_H
#define PATH_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>
#include <string.h>

#define		LEN			40	
#define		SLOTNUM		12
#define		SAS_NUM		20
#define		VENDOR		"LSI"
#define		SKIPNUM		9
#define		GB			1000000000

struct sas_dev {
	uint64_t block;
	uint64_t sas_address;
	enum dev_state  state;
	char devname[LEN];
};

struct enclosure {
	uint64_t size;  // in GB
	enum dev_state  state; //0 1 2
	int slot;
	char devname[LEN];
};

enum dev_state {
	OFFLINE,
	ONLINE,
	EMTPY,
};

#endif
