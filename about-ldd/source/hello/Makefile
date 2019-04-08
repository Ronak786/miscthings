ifeq ($(KERNELRELEASE),)
KERNDIR := /home/sora/gitbase/linux-4.4-kernel/linux-4.4
all:
	$(MAKE) -C $(KERNDIR) M=$(PWD) modules
clean:
	$(MAKE) -C $(KERNDIR) M=$(PWD) modules
else
obj-m := hello.o
endif
