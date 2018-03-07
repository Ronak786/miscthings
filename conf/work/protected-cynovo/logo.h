#ifndef PEOTECT_CYNOVO_H
#define PEOTECT_CYNOVO_H

#ifdef __cplusplus
extern "C" {
#endif


#define CYNOVO_DISK "/dev/logo"

int open_logo(char * disk);
int close_logo();
int read_logo();
int erase_logo();
int write_logo(char * logopath);


#ifdef __cplusplus
}
#endif

#endif
