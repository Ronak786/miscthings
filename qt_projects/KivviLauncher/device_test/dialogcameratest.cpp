#include "dialogcameratest.h"
#include "ui_dialogcameratest.h"
#include <QProcess>


// setup a qtimer scan and down
DialogCameraTest::DialogCameraTest(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogCameraTest)
{
    ui->setupUi(this);

    this->setWindowFlags(Qt::FramelessWindowHint);

    timer = new QTimer();
    timer->setInterval(1000);
    connect(timer, SIGNAL(timeout()), this , SLOT(needquit()));
    system("echo 145 > /sys/class/gpio/export");
    timer->start();


}

DialogCameraTest::~DialogCameraTest()
{
    delete ui;
    delete timer;
}

//void DialogCameraTest::keyPressEvent(QKeyEvent *event) {
//    switch(event->key()) {
//    case Qt::Key_Escape:
//        accept();
//        break;
//    case Qt::Key_1:
//        break;
//    case Qt::Key_2:
//        break;
//    default:
//        break;
//    }
//}

void DialogCameraTest::needquit() {
    char tmpbuf[128];
    QFile file("/sys/class/gpio/gpio145/value");
    file.open(QIODevice::ReadOnly);
    file.read(tmpbuf, sizeof(tmpbuf));
//    qDebug("value is %d\n",  tmpbuf[0]);
    file.close();
    if(tmpbuf[0] == 0x30) { // begin press
        qDebug() << "before start dialogcamera";
        dialograser = new DialogRaser;
        dialograser->startRaser();
        dialogcamera = new DialogCamera(10,0,140,1);
        dialogcamera->exec();

        dialograser->stopRaser();
        dialograser->deleteLater();
        dialogcamera->deleteLater();
        qDebug() << "after close dialogcamera";
    }
}

