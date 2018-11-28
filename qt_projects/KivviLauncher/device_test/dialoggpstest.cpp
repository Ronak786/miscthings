#include "dialoggpstest.h"
#include "ui_dialoggpstest.h"
#include <QString>
#include <QDebug>

DialogGpsTest::DialogGpsTest(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogGpsTest)
{
    ui->setupUi(this);

    this->setWindowFlags(Qt::FramelessWindowHint);

    m_Timer = new QTimer();
    m_SerialPort = new QSerialPort();
    m_SerialPort->setPortName("/dev/ttyS2");
//    m_SerialPort->setBaudRate(QSerialPort::Baud9600);
//    m_SerialPort->setDataBits(QSerialPort::Data8);
//    m_SerialPort->setParity(QSerialPort::NoParity);
//    m_SerialPort->setStopBits(QSerialPort::OneStop);
//    m_SerialPort->setFlowControl(QSerialPort::NoFlowControl);
    if (m_SerialPort->open(QIODevice::ReadWrite))
        qDebug() << "Serial Port Open Succ";
    else
        qDebug() << "Serial Port Open Fail";

    connect(m_SerialPort, SIGNAL(readyRead()), this, SLOT(readGpsData()));
    connect(m_Timer, SIGNAL(timeout()), this, SLOT(getGpsInfo()));
    m_Timer->start(1000);

//    ui->lineEdit_Latitude->setText("31.50529000N");
//    ui->lineEdit_Longitude->setText("120.36581167E");
//    ui->lineEdit_Altitude->setText("8.7");
//    ui->lineEdit_UtcTime->setText("03:37:01.000");
//    ui->lineEdit_UtcDate->setText("2018-08-23");
}

DialogGpsTest::~DialogGpsTest()
{
    delete ui;
    m_Timer->deleteLater();
    m_SerialPort->close();
    qDebug() << "Serial Port Close Succ";
}

void DialogGpsTest::readGpsData()
{
    //QString str;
    QByteArray buffer;
    buffer = m_SerialPort->readAll();
    if (!buffer.isEmpty())
    {
        m_GpsData += tr(buffer);
    }


    buffer.clear();

}

//获取GPS定位信息
//$GPGGA,<1>,<2>,<3>,<4>,<5>,<6>,<7>,<8>,<9>,M,<10>,M,<11>,<12>*hh<CR><LF>
//<1> UTC时间，hhmmss（时分秒）格式
//<2> 纬度ddmm.mmmm（度分）格式（前面的0也将被传输）
//<3> 纬度半球N（北半球）或S（南半球）
//<4> 经度dddmm.mmmm（度分）格式（前面的0也将被传输）
//<5> 经度半球E（东经）或W（西经）
//<6> GPS状态：0=未定位，1=非差分定位，2=差分定位，6=正在估算
//<7> 正在使用解算位置的卫星数量（00~12）（前面的0也将被传输）
//<8> HDOP水平精度因子（0.5~99.9）
//<9> 海拔高度（-9999.9~99999.9）
//<10> 地球椭球面相对大地水准面的高度
//<11> 差分时间（从最近一次接收到差分信号开始的秒数，如果不是差分定位将为空）
//<12> 差分站ID号0000~1023（前面的0也将被传输，如果不是差分定位将为空）
void DialogGpsTest::getGpsFixData()
{

}

//$GPRMC,<1>,<2>,<3>,<4>,<5>,<6>,<7>,<8>,<9>,<10>,<11>,<12>*hh<CR><LF>
//<1> UTC时间，hhmmss（时分秒）格式
//<2> 定位状态，A=有效定位，V=无效定位
//<3> 纬度ddmm.mmmm（度分）格式（前面的0也将被传输）
//<4> 纬度半球N（北半球）或S（南半球）
//<5> 经度dddmm.mmmm（度分）格式（前面的0也将被传输）
//<6> 经度半球E（东经）或W（西经）
//<7> 地面速率（000.0~999.9节，前面的0也将被传输）
//<8> 地面航向（000.0~359.9度，以真北为参考基准，前面的0也将被传输）
//<9> UTC日期，ddmmyy（日月年）格式
//<10> 磁偏角（000.0~180.0度，前面的0也将被传输）
//<11> 磁偏角方向，E（东）或W（西）
//<12> 模式指示（仅NMEA0183 3.00版本输出，A=自主定位，D=差分，E=估算，N=数据无效）
void DialogGpsTest::getRMC()
{


}


QString DialogGpsTest::gpsTime(QString str)
{
    QString hour;
    char h;
    QString temp;

    hour = str.mid(0, 2);
    h = hour.toInt(NULL, 10);
    h += 8;
    hour = QString::number(h, 10);

    temp += hour;
    temp += ":";
    temp += str.mid(2, 2);
    temp += ":";
    temp += str.mid(4, 2);
    temp += ".";
    temp += str.mid(7, 3);

    return temp;
}

QString DialogGpsTest::gpsDate(QString str)
{
    QString year;
    QString temp;
    int y = 2000;

    year = str.mid(4, 2);
    y = year.toInt(NULL, 10);
    y += 2000;
    year = QString::number(y, 10);

    temp += year;
    temp += "-";
    temp += str.mid(2, 2);
    temp += "-";
    temp += str.mid(0, 2);

    return temp;
}

void DialogGpsTest::getGpsInfo()
{
    QString temp, str;

    for (int i = 0; i < 100; i++)
    {
        QByteArray array = m_SerialPort->readLine();

        if (array.length() != 0)
        {
            str = array;

            QStringList list = str.split(",");
            if (list.size() >= 12)
            {
                if (list.at(0) == "$GNRMC")
                {
                    qDebug() << str;

                    qDebug() << "UTC Time: " + list.at(1);
                    qDebug() << "纬度: " + list.at(3);
                    qDebug() << "北纬/南纬: " + list.at(4);
                    qDebug() << "经度: " + list.at(5);
                    qDebug() << "东经/西经: " + list.at(6);
                    qDebug() << "速度: " + list.at(7);
                    qDebug() << "方位角: " + list.at(8);
                    qDebug() << "UTC Date: " + list.at(9);
                    qDebug() << "偏磁角: " + list.at(10);

                    ui->lineEdit_Latitude->setText(list.at(3) + list.at(4));
                    ui->lineEdit_Longitude->setText(list.at(5) + list.at(6));
                    ui->lineEdit_Altitude->setText("8.7");
                    ui->lineEdit_UtcTime->setText(gpsTime(list.at(1)));
                    ui->lineEdit_UtcDate->setText(gpsDate(list.at(9)));
                }
                else if (list.at(0) == "$GNGGA")
                {
                    qDebug() << str;

//                    qDebug() << "UTC Time: " + list.at(1);
//                    qDebug() << "纬度: " + list.at(3);
//                    qDebug() << "北纬/南纬: " + list.at(4);
//                    qDebug() << "经度: " + list.at(5);
//                    qDebug() << "东经/西经: " + list.at(6);
                    qDebug() << "Count: " + list.at(7);
//                    qDebug() << "方位角: " + list.at(8);
//                    qDebug() << "UTC Date: " + list.at(9);
//                    qDebug() << "偏磁角: " + list.at(10);
                }
            }
        }

    }

    //array.clear();
}
