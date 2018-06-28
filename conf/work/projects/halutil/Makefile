all: libhalutil.so

LIBCFLAGS := -fPIC

halutil.o : halutil.cpp halutil.h wifiinfo.h camerainfo.h
	gcc ${LIBCFLAGS} -c $< -o $@

wifiinfo.o: wifiinfo.cpp wifiinfo.h
	gcc ${LIBCFLAGS} -c $< -o $@

camerainfo.o: camerainfo.cpp camerainfo.h
	gcc ${LIBCFLAGS} -c $< -o $@

libhalutil.so: halutil.o wifiinfo.o camerainfo.o
	gcc $^ -shared -o $@
