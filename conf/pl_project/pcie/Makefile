KERNELDIR ?= /usr/src/kernels/3.10.0-862.el7.x86_64/
#/usr/src/kernels/linux-3.8.y_biwei/
EXTRA_CFLAGS += -w
PWD := $(shell pwd)

obj-m := pcie56.o
pcie56-objs := pcie.o

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
modules_install:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules_install
clean:
	rm -rf *.o *~core .depend .*.cmd *.ko *.mod.c .tmp_versions Module.symvers modules.order
	
