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

#include <exception>

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

    connect(this, SIGNAL(setquit()), mworker, SLOT(setquit()));
//    qthread->start();
    mworker->start();

    // start timer when start loop of get from spi
    timer = new QTimer;
    connect(timer, SIGNAL(timeout()), this, SLOT(testquit()));// timer test quit
    timer->setInterval(1000);
    timer->start(1000);
}

DialogCamera::~DialogCamera() {
    delete ui;
    delete timer;
    delete mworker;
}

void DialogCamera::recvImgData(QByteArray data_orig, int width_orig, int height)
{
    QImage decodeImg = QImage ((const uchar*)data_orig.data(), width_orig, height, QImage::Format_Indexed8);

    ui->label->setPixmap(QPixmap::fromImage(decodeImg).scaledToWidth(width_orig / 4));

    // set count
    static int picCount = 0;
    ui->label_2->setText(QString("count: ") + QString::number(++picCount));

    if (!test_decode((char*)decodeImg.bits(), decodeImg.width(), decodeImg.height()) ) { // test code not sucess with desired flip
//        QImage decodeImgInnerFlip = decodeImg.mirrored(true, false);
//        if (test_decode((char*)decodeImgInnerFlip.bits(), decodeImgInnerFlip.width(), decodeImgInnerFlip.height())) {
//            qDebug() << "flip recognize success";
//        }
        qDebug() << "recognize failed";
    } else {
        qDebug() << "origin recoginze success";
    }
//    QImageWriter writer("/data/tmp/tmpimg" + QString::number(++count) + QString(".png"));
//    writer.setFormat("png");
//    writer.write(decodeImg);
}

void DialogCamera::testquit()
{
    char tmpbuf[128];
    QFile file("/sys/class/gpio/gpio145/value");
    file.open(QIODevice::ReadOnly);
    file.read(tmpbuf, sizeof(tmpbuf));
    file.close();
    qDebug("value is %d\n",  tmpbuf[0]);

    if(tmpbuf[0] == 0x31) { // begin unpress
        qDebug() << "begin end dialog";
        mworker->requestInterruption();
        timer->stop();
        this->accept();
    }


}

//void DialogCamera::keyPressEvent(QKeyEvent* event) {
//    qDebug() << "before close  dialog camera";
//    if (event->key() == Qt::Key_1 || event->key() == Qt::Key_Escape) {
//        qDebug() << "begin close camera";
// //        emit setquit();
// //        mworker->_quitflag = true;
//        mworker->requestInterruption();
//        timer->stop();
//        this->accept();
//    }
//}

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
    int ret = 0;

    try {
        ret = nanoid_hexagon_recognition((unsigned char*)dataptr, rows, cols, 1,
            const_cast<char*>(ymlname.c_str()), bitarray, 91,
            bound, 4);
        if (ret < 0) {
//                printf("can not decode %d\n", ret);
            return false;
        }
    } catch (std::exception &e) {
        printf("exception happened in recognition\n");
        return false;
    }

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


    return false;
}

