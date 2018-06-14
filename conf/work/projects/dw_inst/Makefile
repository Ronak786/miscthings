All: client input createsig

#CFLAGS := -D_DEBUG
CPPFLAGS := -DNOSSL -std=c++11 -Wall
LDFLAGS := -L. -lsigutil -lcrypto
SIGCPPFLAGS := -fPIC
SIGLDFLAGS := -shared -lcrypto
pkghandle.o : pkghandle.cpp pkghandle.h
pkginfo.o : pkginfo.cpp pkginfo.h
ftplib.o: ftplib.cpp ftplib.h
	g++ -Wno-unused-variable -Wno-format -Wno-unused-but-set-variable -Wno-restrict ${CPPFLAGS} -c $<
client.o: client.cpp client.h json.h libsigutil.so
	g++ ${CFLAGS} ${CPPFLAGS} -c $< -o $@


client: pkghandle.o client.o pkginfo.o ftplib.o
	g++ -o $@ $^ ${LDFLAGS}

input: createjsonfromtext.cpp json.h
	g++ ${CPPFLAGS} -o $@ $<

sig: libsigutil.so
libsigutil.so: sigutil.o
	g++  -o $@ $< ${SIGLDFLAGS} 
sigutil.o: sigutil.cpp sigutil.h
	g++ ${CFLAGS} ${CPPFLAGS} ${SIGCPPFLAGS} -o $@ -c $<

createsig: createsig.cpp libsigutil.so 
	g++ ${CPPFLAGS} ${CFLAGS} -o $@ $< ${LDFLAGS}

clean:
	rm -f *.o *.so input client createsig arm/*
dataclean:
	rm -rf destdir/*
	rm -rf localpkgs/*
	rm -rf remotepkgs/*.tar.gz*  remotepkgs/*.pem remotepkgs/*.json
