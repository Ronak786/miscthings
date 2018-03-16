#ifndef _ANALYSIS_PACKAGE_H
#define _ANALYSIS_PACKAGE_H

#define ANALYSIS_MSG_SIZE      		    4096

#define Value_Type_BIN 0x30
#define Value_Type_NUM 0x20
#define Value_Type_STR 0x10


int pack_data(unsigned char *buf,int len,unsigned char * name,int namelen,unsigned char* value,int valuelen,int valuetype);
int pack_data_headtail(unsigned char serial,unsigned char *buf,int buflen);


#endif
