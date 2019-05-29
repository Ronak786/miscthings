#ifndef PLINTERNAL_H
#define PLINTERNAL_H


/*
#define Fatal(ret) do \
{ fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n",\
__LINE__, __FILE__, errno, strerror(errno)); return (ret); } while(0)
*/

#define Fatal(ret, fmt, ...) do \
{ fprintf(stderr, fmt, ##__VA_ARGS__); return (ret);} while(0)

#define Log(fmt, ...) do \
{ fprintf(stderr, fmt, ##__VA_ARGS__);} while(0)

// mmap info of current process
typedef struct MapInfo {
	int fd;
	unsigned char *mapBase;
	unsigned long mapLen;
	pid_t pid;
} MapInfo;

#endif
