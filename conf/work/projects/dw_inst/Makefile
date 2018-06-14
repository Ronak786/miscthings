All: pkgmgnt input createsig

sig: libsigutil.so

#DFLAG := -g
CFLAGS := -D_DEBUG
CPPFLAGS := -DNOSSL -std=c++11 -Wall
PKGLIBLDFLAGS := -L. -lsigutil -lcrypto
CREATESIGLDFLAGS := -L. -lsigutil -lcrypto
SIGLDFLAGS := -lcrypto

pkghandle.o : pkghandle.cpp pkghandle.h
	g++ ${DFLAG} -fPIC   ${CFLAGS} ${CPPFLAGS} -o $@ -c $<
pkginfo.o : pkginfo.cpp pkginfo.h
	g++ ${DFLAG} -fPIC   ${CFLAGS} ${CPPFLAGS} -o $@ -c $<
ftplib.o: ftplib.cpp ftplib.h
	g++ ${DFLAG} -fPIC -Wno-unused-variable -Wno-format -Wno-unused-but-set-variable -Wno-restrict ${CFLAGS} ${CPPFLAGS} -c $< -o $@
pkglib.o: pkglib.cpp pkgmgnt.h json.h 
	g++ ${DFLAG} -fPIC  ${CFLAGS} ${CPPFLAGS} -o $@ -c $<
pkgmgnt.o : pkgmgnt.cpp pkgmgnt.h
	g++ ${DFLAG} -fPIC  ${CFLAGS} ${CPPFLAGS} -o $@ -c $< 
sigutil.o: sigutil.cpp sigutil.h
	g++ ${DFLAG} -fPIC ${CFLAGS} ${CPPFLAGS} -o $@ -c $<


libpkglib.so: pkghandle.o pkglib.o pkginfo.o ftplib.o libsigutil.so
	g++ ${DFLAG} -o $@ $^ ${PKGLIBLDFLAGS} -shared
libsigutil.so: sigutil.o
	g++ ${DFLAG}  -o $@ $< ${SIGLDFLAGS}  -shared

pkgmgnt: pkgmgnt.o libpkglib.so
	g++ ${DFLAG} -o $@ $<  -L. -lpkglib -lsigutil

input: createjsonfromtext.cpp json.h
	g++ ${DFLAG} ${CFLAGS} ${CPPFLAGS} -o $@ $<

createsig: createsig.cpp libsigutil.so 
	g++ ${DFLAG} ${CFLAGS} ${CPPFLAGS} -o $@ $< ${CREATESIGLDFLAGS}

clean:
	rm -f *.o *.so input pkgmgnt createsig arm/*
dataclean:
	rm -rf destdir/*
	rm -rf localpkgs/*
	rm -rf remotepkgs/*.tar.gz*  remotepkgs/*.pem remotepkgs/*.json
