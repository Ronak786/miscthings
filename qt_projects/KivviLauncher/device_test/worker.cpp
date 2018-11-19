#include "worker.h"
#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QTime>
#include <QTimer>
#include <QThread>
#include <QMatrix>
#include <QSize>

#ifdef __linux__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>



#elif _WIN32
#endif


#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/videoio/videoio.hpp>


// while get fixed length of data from spi
// append data into image data buf, add count value,
// if count % interval == 0
//      do a recognition



Worker::Worker(int usleep, int pheight):
    _usleep(usleep), _pheight(pheight), _quitflag(false) {
    qDebug() << "worker start";

}


void Worker::run() {

#ifdef __linux__
    runarm();
#elif _WIN32

#endif

}

#define WIDTH 498

#ifdef _WIN32

#elif __linux__

void Worker::runarm() {
    barr.reserve(1500*1000*10);

    int fd;
    if ((fd = open("/dev/cam", O_RDONLY)) == -1) {
        qDebug() << "can not open camere device";
        return;
    }

    int index;
    char txData[WIDTH +8];
    char rxData[sizeof(txData)];

    memset(txData, 0xa2, sizeof(txData));
    txData[0] = 0xa1;

    // 0 before start record or after check,
    //1 continue to add picture
    int condition = 0;
    int picHeight = _pheight; // height is just 150 pixels
    int collectedHeight = 0;
    int count = 0;

    char  dumb[WIDTH];
    memset(dumb, 0x0, WIDTH);

    while (!isInterruptionRequested()) {
        qDebug() << "in worker loop";
         if (++count % 100 == 0) {
            QApplication::processEvents();
         }/*
         if (write(fd, "0", 1) != 0) {
             qDebug() << "can not start, just break out";
             break;
         }
         */
        if(read(fd, NULL, sizeof(rxData)) == sizeof(rxData))
        {

            for(index=0; index<5; index++)
            {
                if(!memcmp(rxData +index, "\x00\xff\x01", 3))
                {
                    switch (condition) {
                        case 0: // check if can start, pixels
                            if (blkscr(rxData + index + 3, WIDTH)) {
                                break; // not start and still black, just continue
                            }
                            condition = 1; // not black screen, start to attach

                            barr.clear(); // start collect from scratch
                            // append five line of black, make picture not at top border line
    //                        barr.append(QByteArray(dumb, WIDTH));
                            collectedHeight = 0; // reset height to 0

                        // fall through
                        case 1: // continue to add
//                            cv::Mat tmpmat1(1, WIDTH, CV_8UC1, rxData +index+3);

//                            barr.append(QByteArray((const char*)(tmpmat1.data), tmpmat1.total()));
                            barr.append(QByteArray((const char*)(rxData+index+3), WIDTH));
                            if (++collectedHeight >= picHeight) {
                                emit sendImgData(barr, WIDTH, picHeight);
                                condition = 0; // return to state black screen
                            }

                            break;
                        default:
                            qDebug() << "condition error";
                            break;
                    }
                    break; // break of for
                }
            }
            if(index>=5) {
//                qDebug() << "data error";
            }

        } else {
            qDebug() << "can not read";
        }

        if (_usleep) {
            QThread::usleep(_usleep);
        }
    }

    close(fd);
    emit finished();
    qDebug() << "done scan";
}

#endif

void Worker::setquit() {
    _quitflag = true;
}






