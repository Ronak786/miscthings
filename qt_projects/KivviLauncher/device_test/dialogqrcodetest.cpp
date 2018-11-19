#include "dialogqrcodetest.h"
#include "ui_dialogqrcodetest.h"
#include <QKeyEvent>

DialogQRCodeTest::DialogQRCodeTest(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogQRCodeTest)
{
    ui->setupUi(this);

    this->setWindowFlags(Qt::FramelessWindowHint);

    m_DeviceQRCode = new DeviceQRCode();

    int width = ui->label_QRCode->width();
    int height = ui->label_QRCode->height();

    m_DeviceQRCode = new DeviceQRCode();

    QPixmap pixmap = m_DeviceQRCode->generateQRcode("Hello World", width, height);
    ui->label_QRCode->setPixmap(pixmap);
}

DialogQRCodeTest::~DialogQRCodeTest()
{
    delete ui;
}

void DialogQRCodeTest::keyPressEvent(QKeyEvent *event)
{
    switch (event->key())
    {
    case Qt::Key_Escape:
        this->close();
        break;
    }
}


