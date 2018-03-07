
#ifndef _ANALYSIS_MODULE_H
#define _ANALYSIS_MODULE_H

#ifdef __cplusplus
extern "C" {
#endif
#define ANALYSIS_DATA_SIZE      		10240
#define BUF_MAX_SIZE            		10240

#define LISTEN_MAX_NUM          		5

//local socket
#define ANDROID_SERVICE_SOCKET_PATH    	"protected_cynovo"
#define ANDROID_SERVICE

#if defined(PC_SERVICE)
#define SERVICE_SOCKET_PATH     		"payment-service.sock"
#else 
#define SERVICE_SOCKET_PATH				"/dev/socket/protected_cynovo"
#endif

//tcp udp socket
#define SERVICE_SOCKET_PORT   			"24000"

#define SOCKET_TYPE_LOCAL				0
#define SOCKET_TYPE_UDP					1
#define SOCKET_TYPE_TCP 				2

typedef struct 
{
    const char *name;
    int io;
    int datalen;
    int (*convert_func)(struct analysis_module *module, unsigned char *indata, int inlen, unsigned char *outdata);
}param_analysis_t;

typedef struct 
{
    const char *name;
	int namelen;
    const char *key;
    int keylen;
    param_analysis_t *param;
}cmd_analysis_t;

typedef struct analysis_module 
{
    unsigned char data[ANALYSIS_DATA_SIZE];
    int datalen;
    cmd_analysis_t *cmd;
	int socket_type;
	char socket_addr[64];
    void *_private_data;
}analysis_module_t;

#define PARAM(name, io, datalen, convert_func) \
            {name, io, datalen, (void*)convert_func}
#define PARAM_IN(name, datalen, convert_func) \
            PARAM(name, 1, datalen, convert_func)
#define PARAM_IN_NONE(name, datalen) \
            PARAM_IN(name, datalen, convert_none)
#define PARAM_OUT(name, datalen, convert_func) \
            PARAM(name, 0, datalen, convert_func)
#define PARAM_OUT_NONE(name, datalen) \
            PARAM_OUT(name, datalen, convert_none)

#define CMD(name, key, param)           {name, sizeof(name)-1, key, sizeof(key)-1, param}
#define END()                           {0}

int check(analysis_module_t *module);
int get(analysis_module_t *module, const char *name, unsigned char *data, int *datalen);
int set(analysis_module_t *module, const char *name, unsigned char *data, int datalen);
int set_string(analysis_module_t *module, const char *name, char *string);

void analysis_module_init(analysis_module_t *module);

int convert_none(analysis_module_t *module, unsigned char *indata, int inlen, unsigned char *outdata);
int convert_timeout(analysis_module_t *module, unsigned char *indata, int inlen, unsigned char *outdata);
int convert_r0_padding(analysis_module_t *module, unsigned char *indata, int inlen, unsigned char *outdata);
int convert_atob16(analysis_module_t *module, unsigned char *indata, int inlen, unsigned char *outdata);
int convert_b16toa(analysis_module_t *module, unsigned char *indata, int inlen, unsigned char *outdata);
int convert_atob32(analysis_module_t *module, unsigned char *indata, int inlen, unsigned char *outdata);
int convert_b32toa(analysis_module_t *module, unsigned char *indata, int inlen, unsigned char *outdata);
int convert_pack16bLen(analysis_module_t *module, unsigned char *indata, int inlen, unsigned char *outdata);
int convert_unpack16bLen(analysis_module_t *module, unsigned char *indata, int inlen, unsigned char *outdata);
int convert_amount(analysis_module_t *module, unsigned char *indata, int inlen, unsigned char *outdata);
int convert_filename(analysis_module_t *module, unsigned char *indata, int inlen, unsigned char *outdata);

#ifdef __cplusplus
}
#endif
#endif
