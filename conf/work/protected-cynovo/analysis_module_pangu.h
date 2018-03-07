
#ifndef _ANALYSIS_MODULE_PANGU_H
#define _ANALYSIS_MODULE_PANGU_H

#include "analysis_module.h"
#ifdef __cplusplus
extern "C" {
#endif

#define ARRAY_SIZE(x) sizeof(x)/sizeof(x[0])

#define CMD_WRITEPAGE           "writepage"
#define CMD_READPAGE            "readpage"
#define CMD_ERASEPAGE           "erasepage"
#define CMD_UPDATELOGO          "updatelogo"

typedef struct protected_cynovo_cmd{
    unsigned char * cmd;
	int ( * handlecmd)(analysis_module_t * module);
}protected_cynovo_cmd_t;

int get_protected_cynovo_cmd_length();
protected_cynovo_cmd_t * get_cynovo_device(int id);

extern analysis_module_t analysis_module_pangu;

#ifdef __cplusplus
}
#endif
#endif
