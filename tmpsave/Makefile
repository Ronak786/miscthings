STDLIB = /usr/lib/libsgutils2.a
STDSOLIB = -lsgutils2
LDPATH = -L.
PRIVSOLIB = -lenclosure
PRIVLIB = libenclosure.a


all: main
main: main.o libenclosure.so
	gcc -Wall -fno-common $< -o $@ $(LDPATH) $(PRIVSOLIB) $(STDSOLIB) -Wl,-rpath=.

#stmain: main.o $(PRIVLIB) 
#	gcc $^ $(STDLIB) -o $@


enclosure.o : enclosure.h
sg_ses.o : sg_unaligned.h

libenclosure.so: enclosure.o sg_ses.o
	gcc -fPIC --shared -o $@ $^

libenclosure.a: enclosure.o sg_ses.o
	ar rcs $@ $^
	
clean:
	rm -f *.o stmain main *.so hi *.a
