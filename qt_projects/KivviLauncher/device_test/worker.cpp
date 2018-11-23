#include "worker.h"
#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QTime>
#include <QTimer>
#include <QThread>
#include <QMatrix>
#include <QSize>
#include <QFile>

#ifdef __linux__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <linux/ioctl.h>
#include <sys/ioctl.h>
#include <linux/types.h>



#elif _WIN32
#endif


#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/videoio/videoio.hpp>

#define IOTYPE 'W'
#define LINECAMNONE_START _IO(IOTYPE,0x01)
#define LINECAMNONE_STOP  _IO(IOTYPE,0x02)
#define LINECAMGET_INDEX  _IOR(IOTYPE,0x11,__u32)
#define LINECAMPUT_INDEX  _IOW(IOTYPE,0x21,__u32)


// while get fixed length of data from spi
// append data into image data buf, add count value,
// if count % interval == 0
//      do a recognition



Worker::Worker(int usleep, int pheight):
    _usleep(usleep), _pheight(pheight), _quitflag(false), _needprint(false) {
    qDebug() << "worker start";

    if (QFile::exists("/data/_tmplist")) {
        _needprint = true;
    }

}


void Worker::run() {

#ifdef __linux__
    runarm();
#elif _WIN32

#endif

}

// we have 3 bytes head of every 5000 bytes
#define WIDTH 500
#define HEIGHT 20
#define SHOWHEIGHT 480
#define NUMBUFS 2
#define FRAMEHEAD 8

#ifdef _WIN32

#elif __linux__

void Worker::runarm() {
    int fd;
    int index;
    int size = HEIGHT * WIDTH + FRAMEHEAD;   //every 10 lines have 8 bytes head, 50 times 8
    char *mmapbuf = NULL;
    int ret;
    if ((fd = open("/dev/cam", O_RDWR)) == -1) {
        qDebug() << "can not open camere device";
        return;
    }

    if ((mmapbuf = (char*)mmap(NULL, size*NUMBUFS, PROT_READ, MAP_SHARED, fd, 0)) == (void*)-1) {
        qDebug() << "mmap failed\n" << strerror(errno);
        return;
    }


    if ((ret = ioctl(fd, LINECAMNONE_START, 0)) != 0) {
        qDebug() << "can not start, just break out" << ret;
        return;
    }
    while (!isInterruptionRequested()) {
//        qDebug() << "in worker loop";
            QApplication::processEvents();
            barr.clear();

            // currently receving will mix with valid and invalid lines
            int curline = 0;
            while (!isInterruptionRequested() && curline < SHOWHEIGHT / HEIGHT) {   // 480 / 10 == 48
                if (ioctl(fd, LINECAMGET_INDEX, &index))
                {
                    qDebug() << "some error happened inside kernel get index";
                    continue;
                } else {
                    if (_needprint) {
                        qDebug() << "begin print from node " << index;
                    }
                    // read from mmap's index buffer[index], print out, over
                    debugPrint(mmapbuf + (index-1) * size, size);
                    char *startpos = mmapbuf + (index-1) * size;
                    if (startpos[0] == 0x00 && startpos[1] == 0xff && startpos[2] == 0x01) {
                        if (_needprint) {
                            qDebug() << "append line " << curline;
                        }
                        if (curline == 0) {
                            barr = QByteArray(startpos+3, size - FRAMEHEAD);
//                            barr.append(3-FRAMEHEAD, (char)0x00);
                        } else {
                            barr.append(startpos+3, size-FRAMEHEAD); // the first will be share , not copy !!
//                            barr.append(3 - FRAMEHEAD, (char)0x00); // fill up to 10 lines with black
                        }
                        ++curline;
                    }
                }

                if (ioctl(fd, LINECAMPUT_INDEX, &index)) {
                    qDebug() << "error write back index " << index;
                    break;
                }
            }

            if (isInterruptionRequested()) {
                break;
            }

            qDebug() << "begin to emit signal";
            emit sendImgData(barr, WIDTH, SHOWHEIGHT);

        if (_usleep) {
            QThread::usleep(_usleep);
        }
    }

    if (munmap(mmapbuf, size*NUMBUFS) != 0) {
        qDebug() << "unmap failed";
        return;
    }
    if (ioctl(fd, LINECAMNONE_STOP, 0)) {
        qDebug() << "can not stop, just break out";
        return;
    }
    close(fd);
//    emit finished();
    qDebug() << "done scan";
}

#endif

void Worker::setquit() {
    _quitflag = true;
}

void Worker::debugPrint(char *buf, int size) {
    int line = 32;
    static int count = 0;
    if (_needprint) {
        qDebug() << "print start" << ++count;
        // print every head place

        for (int i = 0; i < line; ++i) {
            printf("%02x ", buf[i]);
        }
        putchar('\n');
        for (int i = line; i >0 ; --i) {
            printf("%02x ", buf[size - i]);
        }
        putchar('\n');
        putchar('\n');

        qDebug() << "print done";
    }
}






