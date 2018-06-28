all: libhalutil.so

#LD_LIBRARY_PATH=/home/sora/Downloads/opencv300_x86/lib:.
PICFLAGS := -fPIC
LIBCFLAGS :=  -std=c++11 -I/home/sora/Downloads/opencv300_x86/include
LDFLAGS := -L. -L/home/sora/Downloads/opencv300_x86/lib -lopencv_world

halutil.o : halutil.cpp halutil.h wifiinfo.h camerainfo.h lteinfo.h
	g++ ${PICFLAGS} ${LIBCFLAGS} -c $< -o $@

wifiinfo.o: wifiinfo.cpp wifiinfo.h
	g++ ${PICFLAGS} ${LIBCFLAGS} -c $< -o $@

camerainfo.o: camerainfo.cpp camerainfo.h
	g++ ${PICFLAGS} ${LIBCFLAGS} -c $< -o $@

lteinfo.o: lteinfo.cpp lteinfo.h
	g++ ${PICFLAGS} ${LIBCFLAGS} -c $< -o $@

libhalutil.so: halutil.o wifiinfo.o camerainfo.o lteinfo.o
	g++ $^ -shared -o $@ ${LDFLAGS}

main: main.cpp libhalutil.so
	g++ ${LIBCFLAGS} $< -o $@  ${LDFLAGS} -lhalutil 

clean:
	rm -f *.so *.o main
