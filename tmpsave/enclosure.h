#ifndef PATH_H
#define PATH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define		LEN		80	
#define		SLEN		10
#define		SLOTNUM		12
#define		SASNUM		20
#define		VENDOR		"LSI"
#define		SKIPNUM		16
#define		GB		1000000000

/*status of slot*/
enum dev_state {
	OFFLINE,
	ONLINE,
	EMPTY,
};

struct enclosure {
	uint64_t size;  // in GB
	enum dev_state  state; //0 1 2
	char devname[LEN];
};

int get_enclosure_info(struct enclosure *enclosure, int num);
#ifdef __cplusplus
}
#endif
#endif
