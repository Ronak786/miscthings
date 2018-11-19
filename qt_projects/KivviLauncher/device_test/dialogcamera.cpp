#include "dialogcamera.h"

#include <QDebug>
#include <QFile>
#include <QMenu>
#include <QScrollBar>
#include <QImageWriter>
#include <QKeyEvent>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/videoio/videoio.hpp>


#include "Decoder.h"
#include "NDCode_Image_Recognition.h"

DialogCamera::DialogCamera(int gap, int usleep, int pheight, int flip, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogCamera),
    _gap(gap), _usleep(usleep), _flip(flip == 1 ? true : false)
{
    qDebug() << "start dialog camera";
    ui->setupUi(this);

//    qthread = new QThread();
    mworker = new Worker(_usleep, pheight);
    connect(mworker, SIGNAL(sendImgData(QByteArray,int,int)), this, SLOT(recvImgData(QByteArray,int,int)), Qt::BlockingQueuedConnection);

//    mworker->moveToThread(qthread);

//    connect(qthread, SIGNAL(started()), mworker, SLOT(run()));
//    connect(mworker, SIGNAL(finished()), qthread, SLOT(quit()));
    connect(this, SIGNAL(setquit()), mworker, SLOT(setquit()));
//    qthread->start();
    mworker->start();

    // start timer when start loop of get from spi
    timer = new QTimer;
    connect(timer, SIGNAL(timeout()), this, SLOT(shout()));
    timer->setInterval(1000);
    timer->start(1000);
}

DialogCamera::~DialogCamera() {
    delete ui;
    delete timer;
    delete mworker;
//    delete qthread;
}

void DialogCamera::recvImgData(QByteArray data_orig, int width_orig, int height)
{

//    QByteArray data;
//    int interval = 2;
//    int width = width_orig / interval;
//    int total = width_orig / interval * height;
//    data.reserve(total);
//    int gap = _gap;
//    for (int line = 0; line < height; ++line) {
//        for (int col = 0; col < width_orig; ++col) {
//            QByteRef tmpchar = data_orig[line*width_orig + col];
//            if ((int)data_orig[line*width_orig + col ] > gap) {
//                tmpchar = 255;
//                break;
//            } else {
//                tmpchar = 0;
//            }
//            if (line % interval == 0 && col % interval == 0) {
//                data.append(tmpchar);
//            }
//        }
//    }

//    ui->label->setPixmap(QPixmap::fromImage(QImage((const uchar*)data.data(), width_orig / interval, height / interval ,QImage::Format_Indexed8)));
//      ui->label->setPixmap(QPixmap::fromImage(QImage((const uchar*)data_orig.data(), width_orig, height  ,QImage::Format_Indexed8).transformed(QMatrix().rotate(90))));
    QImage showimg = QImage ((const uchar*)data_orig.data(), width_orig, height  ,QImage::Format_Indexed8).mirrored(_flip, false);

    ui->label->setPixmap(QPixmap::fromImage(showimg));

    // set count
    static int picCount = 0;
    ui->label_2->setText(QString("count: ") + QString::number(++picCount));

    static int count = 0;
    QImage decodeImg = showimg.scaledToWidth(4*width_orig);
//    test_decode(data_orig.data(), width_orig, height);
    qDebug() << "the width is " << decodeImg.width() << " the height is " << decodeImg.height();
    if (!test_decode((char*)decodeImg.bits(), decodeImg.width(), decodeImg.height()) ) { // test code not sucess with desired flip
        QImage decodeImgInnerFlip = decodeImg.mirrored(true, false);
        if (test_decode((char*)decodeImgInnerFlip.bits(), decodeImgInnerFlip.width(), decodeImgInnerFlip.height())) {
            qDebug() << "flip recognize success";
        }
    } else {
        qDebug() << "origin recoginze success";
    }
    QImageWriter writer("/data/tmp/tmpimg" + QString::number(++count) + QString(".png"));
    writer.setFormat("png");
    writer.write(showimg);


}

void DialogCamera::shout()
{
//    mFlagImgDataUsed = true;

//    ui->label->setPixmap(QPixmap::fromImage(QImage((const uchar*)mImgData.data(), mImgWidth, mImgHeight, QImage::Format_Indexed8)));

//    ui->scrollArea->verticalScrollBar()->setSliderPosition(ui->scrollArea->verticalScrollBar()->maximum());

//    mFlagImgDataUsed = false;
    qDebug() << "1s";

}

void DialogCamera::keyPressEvent(QKeyEvent* event) {
    qDebug() << "before close  dialog camera";
    if (event->key() == Qt::Key_1 || event->key() == Qt::Key_Escape) {
        qDebug() << "begin close camera";
//        emit setquit();
//        mworker->_quitflag = true;
        mworker->requestInterruption();
        timer->stop();
        this->accept();
    }
}

void DialogCamera::saveImg() {
    QImageWriter writer("C:/Users/muryliang/Pictures/savedImg.png");
    writer.setFormat("png");
    writer.write(ui->label->pixmap()->toImage());
}

bool DialogCamera::test_decode(char* dataptr, int cols, int rows) {
    std::string ymlname = "/data/conf/algorithm_parameters_v01.yml";
    int bitarray[91], bound[4];
    uint8_t model_id = 0;
    char serial[9] = {0};
    static int count = 0;

/*	for(int i = 0; i < 10; ++i)*/ {
        int ret = nanoid_hexagon_recognition((unsigned char*)dataptr, rows, cols, 1,
            const_cast<char*>(ymlname.c_str()), bitarray, 91,
            bound, 4);
        if (ret < 0) {
            printf("can not decode %d\n", ret);
            return false;
        }

//        printf("get byte code: \n");
//        for (int k = 0; k < 91; ++k) {
//            printf("num %d: %2x\n", k, bitarray[k]);
//            if (k % 8 == 7) {
//                putchar('\n');
//            }
//        }

        set_decoder_paras(0,5,0,0);
        ret = decoder(bitarray, 91, serial, 8, &model_id);
        if (ret < 0) {
            printf("can not get serial %d\n", ret);
        } else {
            count++;
            printf("count %d, success get serial :%s:\n", count, serial);
            ui->label_3->setText(QString("serial :") + QString(serial) + QString(" count: " ) + QString::number(count));
            return true;
        }
    }

    return false;
}

