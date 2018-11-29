#include "dialograser.h"
#include <QDebug>

DialogRaser::DialogRaser(QWidget *parent):
    QDialog(parent),
    ui(new Ui::DialogRaser),
    _opened(false),
    _started(false)
{
    ui->setupUi(this);
    m_SerialPort = new QSerialPort();
    m_SerialPort->setPortName("/dev/ttyS0");
    m_SerialPort->setBaudRate(QSerialPort::Baud115200);
//    m_SerialPort->setDataBits(QSerialPort::Data8);
//    m_SerialPort->setParity(QSerialPort::NoParity);
//    m_SerialPort->setStopBits(QSerialPort::OneStop);
//    m_SerialPort->setFlowControl(QSerialPort::NoFlowControl);

    if (m_SerialPort->open(QIODevice::ReadWrite)) {
        _opened = true;
    } else {
        _opened = false;
    }

}

DialogRaser::~DialogRaser() {
    m_SerialPort->clear();
    m_SerialPort->close();
    m_SerialPort->deleteLater();
}

void DialogRaser::startRaser() {
    qDebug() << "begin start raser";
    if (!_opened) {
        qDebug() << "open serial port first";
        return;
    }
    QByteArray bstart("Md 8001\r\n");
    if (m_SerialPort->write(bstart) != bstart.size()) {
        qDebug()  << "error write raser";
        return;
    }
    _started = true;
    qDebug() << "end start raser";

}

void DialogRaser::stopRaser() {
    qDebug() << "begin end raser";
    if (!_opened) {
        qDebug() << "open serial port first";
        return;
    }
    QByteArray bstop("Md 8000\r\n");
    if (m_SerialPort->write(bstop) != bstop.size()) {
        qDebug()  << "error write raser";
        return;
    }
    _started = false;
    qDebug() << "end end raser";
}
