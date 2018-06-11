All: client

CPPFLAGS := -std=c++11 -Wall
pkghandle.o : pkghandle.cpp pkghandle.h
client.o: client.cpp client.h json.h
pkginfo.o : pkginfo.cpp pkginfo.h

client: pkghandle.o client.o pkginfo.o
	g++ ${CPPFLAGS} -o $@ $^

input: createjsonfromtext.cpp json.h
	g++ ${CPPFLAGS} -o $@ $<
