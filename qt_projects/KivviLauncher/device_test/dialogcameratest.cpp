#include "dialogcameratest.h"
#include "ui_dialogcameratest.h"
#include <QProcess>


DialogCameraTest::DialogCameraTest(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogCameraTest)
{
    ui->setupUi(this);

    this->setWindowFlags(Qt::FramelessWindowHint);

    dialograser = new DialogRaser;
}

DialogCameraTest::~DialogCameraTest()
{
    delete ui;
    dialograser->deleteLater();
}

void DialogCameraTest::keyPressEvent(QKeyEvent *event) {
    switch(event->key()) {
    case Qt::Key_Escape:
        accept();
        break;
    case Qt::Key_1: // set raser
        dialograser->exec();
        break;
    case Qt::Key_2: // start camera, currently just ignore
        qDebug() << "before start dialogcamera";
        dialogcamera = new DialogCamera(10,1000,140,1);
        dialogcamera->exec();
        qDebug() << "after close dialogcamera";
        break;
    default:
        break;
    }
}
