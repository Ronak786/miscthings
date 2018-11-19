#include "devicesystemtime.h"
#include <QDateTime>

DeviceSystemTime::DeviceSystemTime(QObject *parent) : QObject(parent)
{

}

QString DeviceSystemTime::getDeviceSystemDate()
{
    QDateTime dateTime = QDateTime::currentDateTime();

    return dateTime.date().toString();
}

QString DeviceSystemTime::getDeviceSystemTime()
{
    QDateTime dateTime = QDateTime::currentDateTime();

    return dateTime.time().toString();
}
