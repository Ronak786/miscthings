all: libpl.so  pltest
CFLAGS := -std=gnu99
LDFLAGS := -L. -lpl

## just for test
MACRO := -D__TEST

pl.o : pl.c pl2e.h plinternal.h
	gcc -c $(MACRO) $(CFLAGS) -fpic $<  -o $@
pltest.o : pltest.c pl2e.h plinternal.h
	gcc -c $(MACRO) $(CFLAGS) -fpic $<  -o $@

libpl.so : pl.o
	gcc  -shared $^ -o $@

pltest :  pltest.o libpl.so
	gcc  $< -o $@ $(LDFLAGS)

mkloop:
	dd if=/dev/zero of=loopbackfile.img bs=10M count=1
	sudo losetup -fP loopbackfile.img

clean:
	rm -f *.o libpl.so pltest
