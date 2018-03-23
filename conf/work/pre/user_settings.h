#include <config.h>

#define CHAR_BIT  8
#define NULL 0
#define WOLFSSL_ROWLEY_ARM
#define printf print

typedef unsigned long size_t;
typedef signed long  ssize_t;

#define XMALLOC(s, h, type)  custom_malloc((s))
#define XFREE(p, h, type)    custom_free((p))
#define XREALLOC(p, n, h, t) custom_realloc((p), (n))
