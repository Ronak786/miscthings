#include "dialogdevicetest.h"
#include "ui_dialogdevicetest.h"
#include <QKeyEvent>

DialogDeviceTest::DialogDeviceTest(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogDeviceTest)
{
    ui->setupUi(this);

    this->setWindowFlags(Qt::FramelessWindowHint);

    m_DialogKeypadTest = new DialogKeypadTest();
    m_DialogQRCodeTest = new DialogQRCodeTest();
    m_DialogLCDTest = new DialogLCDTest();
    m_DialogBatteryTest = new DialogBatteryTest();
    m_DialogGpsTest = new DialogGpsTest();
//    m_DialogLedTest = new DialogLedTest();
    m_DialogCameraTest = new DialogCameraTest();
    m_DialogWlanTest = new DialogWlanTest();
    m_DialogLteTest = new DialogLteTest();
    m_DialogBtTest = new DialogBtTest();
}

DialogDeviceTest::~DialogDeviceTest()
{
    delete ui;
}

void DialogDeviceTest::keyPressEvent(QKeyEvent *event)
{
    switch (event->key())
    {
    case Qt::Key_0:
        system("/data/bin/pwm 1000 31");
        break;
    case Qt::Key_1:
        m_DialogLedTest = new DialogLedTest();
        m_DialogLedTest->exec();
        m_DialogLedTest->deleteLater();
        break;
    case Qt::Key_2:
        m_DialogKeypadTest->exec();
        break;
    case Qt::Key_3:
        m_DialogLCDTest->exec();
        break;
    case Qt::Key_4:
        m_DialogBatteryTest->exec();
        break;
    case Qt::Key_5:
        m_DialogCameraTest->exec();
        break;
    case Qt::Key_6:
        m_DialogGpsTest->exec();
        break;
    case Qt::Key_7:
        m_DialogWlanTest->exec();
        //m_DialogQRCodeTest->exec();
        break;
    case Qt::Key_8:
        m_DialogBtTest->exec();
        break;
    case Qt::Key_9:
        m_DialogLteTest->exec();
        break;
    case Qt::Key_Escape:
        this->close();
        break;
    }
}
