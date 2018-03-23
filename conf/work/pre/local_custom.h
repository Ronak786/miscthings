#ifndef LOCAL_CUSTOM_H
#define LOCAL_CUSTOM_H

typedef unsigned long size_t;
typedef unsigned long uintptr_t;
typedef uintptr_t addr_t;
typedef uintptr_t vaddr_t;
typedef uintptr_t paddr_t;
typedef unsigned char uint8_t;

#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

#endif
