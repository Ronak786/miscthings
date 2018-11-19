#include "devicegps.h"
#include <QDebug>

DeviceGps::DeviceGps(QObject *parent) : QObject(parent)
{

}

void DeviceGps::transformat(QString str)
{
//    double ret = 0.0;
//    QStringList list = str.split('.');
//    QString x = list.at(0);
//    x.mid(0, )
}

void DeviceGps::parseGpsInfo(QString gpsInfo)
{
    QStringList list = gpsInfo.split(",");
    if (list.size() >= 12)
    {
        if (list.at(0) == "$GNRMC")
        {
            qDebug() << "UTC Time: " + list.at(1);
            qDebug() << "GPS Status: " + list.at(2);
            qDebug() << "Latitude: " + list.at(3);
            qDebug() << "北纬/南纬: " + list.at(4);
            qDebug() << "Longitude: " + list.at(5);
            qDebug() << "东经/西经: " + list.at(6);
            qDebug() << "速度: " + list.at(7);
            qDebug() << "方位角: " + list.at(8);
            qDebug() << "UTC Date: " + list.at(9);
            qDebug() << "偏磁角: " + list.at(10);
        }
    }
}
