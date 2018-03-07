#ifndef PEOTECT_CYNOVO_H
#define PEOTECT_CYNOVO_H

#ifdef __cplusplus
extern "C" {
#endif

#define MAXLENGTH 10240

int open_disk();
int close_disk();
int read_pages(int page,unsigned char *buf);
int erase_page(int page);
int write_page(int page,unsigned char *buf,int length);

#ifdef __cplusplus
}
#endif

#endif



