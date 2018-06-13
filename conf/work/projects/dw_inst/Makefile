All: client input createsig

CFLAGS := -fpermissive
CPPFLAGS := -DNOSSL -std=c++11 -Wall
LDFLAGS := -L. -lsigutil -lcrypto
SIGCPPFLAGS := -fPIC
SIGLDFLAGS := -shared -lcrypto
pkghandle.o : pkghandle.cpp pkghandle.h
client.o: client.cpp client.h json.h
	g++ ${CFLAGS} ${CPPFLAGS} -c $< -o $@
pkginfo.o : pkginfo.cpp pkginfo.h
ftplib.o: ftplib.cpp ftplib.h
	g++ -Wno-unused-variable -Wno-format -Wno-unused-but-set-variable -Wno-restrict ${CPPFLAGS} -c $<

client: pkghandle.o client.o pkginfo.o ftplib.o libsigutil.so
	g++ -o $@ $^ ${LDFLAGS}

input: createjsonfromtext.cpp json.h
	g++ ${CPPFLAGS} -o $@ $<

sig: libsigutil.so
sigutil.o: sigutil.cpp sigutil.h
	g++ ${CPPFLAGS} ${SIGCPPFLAGS} -o $@ -c $<
libsigutil.so: sigutil.o
	g++  -o $@ $< ${SIGLDFLAGS} 

createsig: createsig.cpp libsigutil.so 
	g++ ${CPPFLAGS} ${CFLAGS} -o $@ $< ${LDFLAGS}

clean:
	rm -f *.o *.so input client createsig
