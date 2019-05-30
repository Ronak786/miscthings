all: libpl.so  pltest
CFLAGS := -std=gnu99
LDFLAGS := -L. -lpl
CROSS := /home/sora/Downloads/xilinx_ziliao/04-Linux_Source/Toolchain/CodeSourcery/Sourcery_CodeBench_Lite_for_Xilinx_GNU_Linux/bin/arm-xilinx-linux-gnueabi-
CC := $(CROSS)gcc
STRIP := $(CROSS)strip

## just for test
#MACRO := -D__TEST

pl.o : pl.c pl2e.h plinternal.h
	$(CC) -c $(MACRO) $(CFLAGS) -fpic $<  -o $@
pltest.o : pltest.c pl2e.h plinternal.h
	$(CC) -c $(MACRO) $(CFLAGS) -fpic $<  -o $@

libpl.so : pl.o
	$(CC)  -shared $^ -o $@
	$(STRIP) -s $@

pltest :  pltest.o libpl.so
	$(CC)  $< -o $@ $(LDFLAGS)

mkloop:
	dd if=/dev/zero of=loopbackfile.img bs=10M count=1
	sudo losetup -fP loopbackfile.img

clean:
	rm -f *.o libpl.so pltest
