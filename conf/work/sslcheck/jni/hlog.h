

#ifndef _HLOG_H
#define _HLOG_H

#ifdef __cplusplus
extern "C" {
#endif

#define HLOG_EN         1
#define ANDROID_LOG_EN  1

enum hlog_level
{
    H_ERROR = 0,
    H_WARNING,
    H_INFO,
    H_DEBUG,
    H_SYS,
};

int hlog_set_level(char *level_name);

void _hlog(const char *_file, const char *_func, int _line, unsigned char level, char *string, ...);
void _hlog_data(const char *_file, const char *_func, int _line, unsigned char level, char *info, unsigned char *data, int datalen);

#if HLOG_EN == 1
#define hlog(level, ...)                           _hlog( __FILE__, __func__, __LINE__, level, __VA_ARGS__)
#define hlog_data(level, info, data, datalen)      _hlog_data( __FILE__, __func__, __LINE__, level, info, data, datalen)
#else
#define hlog(level, ...)
#define hlog_data(level, info, data, datalen)
#endif

#ifdef __cplusplus
}
#endif
#endif
