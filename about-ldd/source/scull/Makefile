#self makefile
DEBUG = y
ifeq ($(DEBUG),y)
DEBFLAGS = -O -g -DSCULL_DEBUG
else
DEBFLAGS = -O2
endif

EXTRA_CFLAGS += $(DEBFLAGS)
ifneq ($(LDDINC),)
EXTRA_CFLAGS += -I$(LDDINC)
endif

ifneq ($(KERNELRELEASE),)
#obj-m := hello.o
obj-m := scullc.o
else
KERNDIR ?= ${KERNDIR}
PWD := $(shell pwd)
#	KERNDIR ?= /lib/modules/$(shell uname -r)/build
default:
	$(MAKE) -C $(KERNDIR) M=$(PWD) modules
endif

clean:
	rm -rf *.o *~core .depend .*.cmd *.ko *.mod.c *.tmp_versions Module.symbers modules.order

depend .depend dep:
	$(CC) $(EXTRA_CFLAGS) -M *.c > .depend

ifeq (.depend,$(wildcard .depend))
include .depend
endif
