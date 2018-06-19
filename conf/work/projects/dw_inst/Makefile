All: pkgmgnt input createsig

#DFLAG := -g
CFLAGS := -D_DEBUG
CPPFLAGS := -DNOSSL -std=c++11 -Wall
PKGLIBLDFLAGS := -L. -lcrypto
CREATESIGLDFLAGS := -L. -lpkglib -lcrypto
SIGLDFLAGS := -lcrypto

pkghandle.o : pkghandle.cpp pkghandle.h
	g++ ${DFLAG} -fPIC   ${CFLAGS} ${CPPFLAGS} -o $@ -c $<
pkginfo.o : pkginfo.cpp pkginfo.h
	g++ ${DFLAG} -fPIC   ${CFLAGS} ${CPPFLAGS} -o $@ -c $<
ftplib.o: ftplib.cpp ftplib.h
	g++ ${DFLAG} -fPIC -Wno-unused-variable -Wno-format -Wno-unused-but-set-variable -Wno-restrict ${CFLAGS} ${CPPFLAGS} -c $< -o $@
pkgmgnt.o : pkgmgnt.cpp pkgmgnt.h
	g++ ${DFLAG} -fPIC  ${CFLAGS} ${CPPFLAGS} -o $@ -c $< 
sigutil.o: sigutil.cpp sigutil.h
	g++ ${DFLAG} -fPIC ${CFLAGS} ${CPPFLAGS} -o $@ -c $<


libpkglib.so: pkghandle.o pkginfo.o ftplib.o sigutil.o
	g++ ${DFLAG} -o $@ $^ ${PKGLIBLDFLAGS} -shared

pkgmgnt: pkgmgnt.o libpkglib.so
	g++ ${DFLAG} -o $@ $<  -L. -lpkglib -lsigutil

input: createjsonfromtext.cpp json.h
	g++ ${DFLAG} ${CFLAGS} ${CPPFLAGS} -o $@ $<

createsig: createsig.cpp libpkglib.so
	g++ ${DFLAG} ${CFLAGS} ${CPPFLAGS} -o $@ $< ${CREATESIGLDFLAGS}

clean:
	rm -f *.o *.so input pkgmgnt createsig arm/*
dataclean:
	rm -rf destdir/*
	rm -rf localpkgs/*
	rm -rf remotepkgs/*.tar.gz*  remotepkgs/*.pem remotepkgs/*.json
