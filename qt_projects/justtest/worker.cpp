#include "worker.h"
#include <QCoreApplication>
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

#include <windows.h>
#include <M3F20xm.h>

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
    _usleep(usleep), _pheight(pheight) {

}


void Worker::run() {

#ifdef __linux__
    runarm();
#elif _WIN32
    runpc();
#endif

}

#define WIDTH 100

#ifdef _WIN32
void Worker::runpc() {
    barr.reserve(1500*1000*10);
    int count = 1;

    int devid = 0xff;
    do {
        devid = M3F20xm_OpenDevice();
        if (devid == 0xff) {
            qDebug() << "open failed";
        }
    }while (devid == 0xff);

    BYTE result;
    M3F20xm_Verify(devid, &result);
    qDebug() << "verify result:" << result;

    emit startloop();
    // read loop
    QVector<QByteArray> qb;
    SPI_CONFIG cfg;
    memset(&cfg, 0, sizeof(cfg));
    if (!M3F20xm_SPIGetConfig(devid, &cfg)) {
            qDebug() << "can not get config from spiusb";
            return;
    }
    cfg.wSPIConfig &= ~0x3b;
    cfg.wSPIConfig |= 0x38;  // poch to 00, freq to 256
    if (!M3F20xm_SPISetConfig(devid, &cfg)) {
        qDebug() << "can not set config to spiusb";
        return;
    }

    int index;
    char txData[WIDTH +8];
    char rxData[sizeof(txData)];

    memset(txData, 0xa2, sizeof(txData));
    txData[0] = 0xa1;

    while (true) {
        if(M3F20xm_SPITransfer(devid, (BYTE*)txData, (BYTE*)rxData, sizeof(txData), 200))
        {
            for(index=0; index<5; index++)
            {
                if(!memcmp(rxData +index, "\x00\xff\x01", 3))
                {
                    cv::Mat tmpmat1(1, WIDTH, CV_8UC1, rxData +index+3);

//                    emit sendImgData(QByteArray((const char*)(tmpmat2.data), tmpmat2.total()), WIDTH, count);


                    barr.append(QByteArray((const char*)(tmpmat1.data), tmpmat1.total()));
//                    emit sendImgOne(QImage((const uchar*)barr.data(), WIDTH, count, QImage::Format_Indexed8));
                    if(!(count %100))
                        emit sendImgData(barr, WIDTH, count);

                    ++count;
                    break;
                }
            }
            if(index>=5)
                qDebug() << "data error";

        } else {
            qDebug() << "can not read";
        }


//        QThread::msleep(5);
    }
    M3F20xm_CloseDevice(devid);

    qDebug() << "done scan";
}

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

    char  dumb[WIDTH];
    memset(dumb, 0x0, WIDTH);

    while (true) {
        if(read(fd, rxData, sizeof(rxData)) == sizeof(rxData))
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
    qDebug() << "done scan";
}

#endif






