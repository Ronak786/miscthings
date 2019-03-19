
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include "hlog.h"
#define ANDROID_LOG_EN 1
#if ANDROID_LOG_EN == 1
#include <android/log.h>
#define __LOG_TAG__     "CYNOVO-HAL"
#endif 

#define CACHE_MAX_SIZE  2048

#define MASK_TAG        (1)
#define MASK_TIME       (1<<1)
#define MASK_PID        (1<<2)
#define MASK_FUNH_NAME  (1<<3)
#define MASK_SRH_PATH   (1<<4)

static int _hlog_fd = 1;//stdout
static enum hlog_level _hlog_level = H_SYS;

static struct { char *name;  enum hlog_level level; int mask} _level_tab[] = 
{
    {"error",       H_ERROR,    MASK_TAG |MASK_TIME},
    {"warning",     H_WARNING,  MASK_TAG},
    {"info",        H_INFO,     MASK_TAG},
    {"debug",       H_DEBUG,    MASK_TAG |MASK_TIME |MASK_FUNH_NAME},
    {"sysdebug",    H_SYS,      MASK_TAG |MASK_TIME |MASK_PID |MASK_SRH_PATH},
    {}
};  

int hlog_set_level(char *level_name)
{
    int i;

    for(i=0; _level_tab[i].name; i++)
    {
        if(!strncmp(level_name, _level_tab[i].name, strlen(_level_tab[i].name)+1))
        {
           _hlog_level = _level_tab[i].level;
           return 0;
        }
    }
    _hlog_level = H_ERROR;
    return -1;
}

void _hlog(const char *_file, const char *_func, int _line, unsigned char level, char *string, ...)
{
    if(level > _hlog_level)
        return;
    if(_hlog_level == H_SYS && level == H_DEBUG)
        level = H_SYS;

    int cache_datalen = 0;
    char cache[CACHE_MAX_SIZE];
    time_t timep;
    struct tm *p;
    va_list var_arg;
    va_start(var_arg, string);

    //<tag>
    if(_level_tab[level].mask &MASK_TAG)
    {
        snprintf(cache +cache_datalen, sizeof(cache) -cache_datalen, 
                "<%s> ", _level_tab[level].name);
        cache_datalen += strlen(cache +cache_datalen);
    }

    //[yy/mm/dd hh:MM:ss] 
    if(_level_tab[level].mask &MASK_TIME)
    {
        time(&timep);
        p = localtime(&timep);
        snprintf(cache +cache_datalen, sizeof(cache) -cache_datalen, "[%d/%d/%d %d:%d:%d] ", 
                                                                    1900 +p->tm_year, 
                                                                    1 +p->tm_mon, 
                                                                    p->tm_mday,
                                                                    p->tm_hour, 
                                                                    p->tm_min, 
                                                                    p->tm_sec);
        cache_datalen += strlen(cache +cache_datalen);
    }
    
    //{pid}
    if(_level_tab[level].mask &MASK_PID)
    {
        snprintf(cache +cache_datalen, sizeof(cache) -cache_datalen, "{%d} ",  getpid());
        cache_datalen += strlen(cache +cache_datalen);
    }
    
    //<function_name>
    if(_level_tab[level].mask &MASK_FUNH_NAME)
    {
        snprintf(cache +cache_datalen, sizeof(cache) -cache_datalen, "<%s> ", _func);
        cache_datalen += strlen(cache +cache_datalen);
    }

    //<source_path  source_line>
    if(_level_tab[level].mask &MASK_SRH_PATH)
    {
        snprintf(cache +cache_datalen, sizeof(cache) -cache_datalen, "<%s  %d> ", _file, _line);
        cache_datalen += strlen(cache +cache_datalen);
    }

    //string
    vsnprintf(cache +cache_datalen, sizeof(cache) -cache_datalen, string, var_arg);
    cache_datalen += strlen(cache +cache_datalen);

#if ANDROID_LOG_EN == 1
    int android_log_level;
    switch(level)
    {
        case H_ERROR: android_log_level = ANDROID_LOG_ERROR; break;
        case H_WARNING: android_log_level = ANDROID_LOG_WARN; break;
        case H_INFO: android_log_level = ANDROID_LOG_INFO; break;
        case H_DEBUG: android_log_level = ANDROID_LOG_DEBUG; break;
        case H_SYS: android_log_level = ANDROID_LOG_DEBUG; break;
    }
    __android_log_print(android_log_level, __LOG_TAG__, "%s", cache);
    __android_log_print(android_log_level, __LOG_TAG__, "\n");
#endif

    printf("%s", cache);
    //write(_hlog_fd, cache, cache_datalen);
    
    va_end(var_arg);
}

void _hlog_data(const char *_file, const char *_func, int _line, unsigned char level, char *info, unsigned char *data, int datalen)
{
    if(level > _hlog_level)
        return;

    int i, len, cache_datalen = 0;
    char cache[CACHE_MAX_SIZE -CACHE_MAX_SIZE/3];

    //info [datalen]
    sprintf(cache +cache_datalen, "%s [%d] ", info, datalen);
    cache_datalen += strlen(cache +cache_datalen);

    //data
    len = (sizeof(cache) - cache_datalen)/3;
    if(datalen > len)
        datalen = len;
    for(i=0; i<datalen; i++)
    {
        sprintf(cache +cache_datalen, "%02x ", data[i]);
        cache_datalen += 3;//strlen(cache +cache_datalen);
    }
    snprintf(cache +cache_datalen, sizeof(cache) -cache_datalen, "\n");

    _hlog(_file, _func, _line, level, cache);
}


void cyn_info(const char* pMessage, ...)
{
#if ANDROID_LOG_EN == 1
	va_list lVarArgs;
	va_start(lVarArgs, pMessage);
	__android_log_vprint(ANDROID_LOG_INFO, __LOG_TAG__, pMessage, lVarArgs);
	//__android_log_print(ANDROID_LOG_INFO, LOG_TAG, "\n");
	va_end(lVarArgs);
#endif
}

void cyn_dump(unsigned char * pBuf, unsigned int len)
{
        int i =0,j=0;
        char strLine[64];

        unsigned int nLine = 0,nRemain=0;
        nLine = len / 16;
        for(i = 0; i < nLine; i++)
        {
                memset(strLine, 0, 64);
                for(j = 0; j < 16; j++)
                        sprintf(strLine + 3 * j, "%02X ", *pBuf++);
                cyn_info("%s",strLine);
        }
        
        nRemain = len % 16;
        if(nRemain>0){
                memset(strLine, 0, 64);
                for(j = 0; j < nRemain; j++)
                        sprintf(strLine + 3 * j, "%02X ", *pBuf++);
                cyn_info("%s",strLine);        
        }
        return;
}

