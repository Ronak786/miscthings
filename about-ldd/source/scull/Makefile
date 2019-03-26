ifneq ($(KERNELRELEASE),)
obj-m := hello.o
else
KERNDIR ?= ${KERNDIR}
PWD := $(shell pwd)
#	KERNDIR ?= /lib/modules/$(shell uname -r)/build
default:
	$(MAKE) -C $(KERNDIR) M=$(PWD) modules
endif
