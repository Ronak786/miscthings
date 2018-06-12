All: client

CPPFLAGS := -DNOSSL -std=c++11 -Wall
#SIGCPPFLAGS := -I/opt/openssl/include
#SIGLDFLAGS := -L/opt/openssl/lib -lcrypto
SIGLDFLAGS := -lcrypto
pkghandle.o : pkghandle.cpp pkghandle.h
client.o: client.cpp client.h json.h
pkginfo.o : pkginfo.cpp pkginfo.h
ftplib.o: ftplib.cpp ftplib.h
	g++ -Wno-unused-variable -Wno-format -Wno-unused-but-set-variable -Wno-restrict ${CPPFLAGS} -c $<

client: pkghandle.o client.o pkginfo.o ftplib.o
	g++ ${CPPFLAGS} -o $@ $^

input: createjsonfromtext.cpp json.h
	g++ ${CPPFLAGS} -o $@ $<

sig: createsig
createsig: createsig.o
	g++ ${CPPFLAGS} ${SIGCPPFLAGS} -o $@ $< ${SIGLDFLAGS} 

clean:
	rm -f *.o input client createsig
