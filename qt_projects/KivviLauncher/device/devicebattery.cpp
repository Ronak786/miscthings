#include "devicebattery.h"
#include <QDebug>
#include <QStringList>

DeviceBattery::DeviceBattery(QObject *parent) : QObject(parent)
{

}

QString DeviceBattery::getDeviceBatteryCapacityString()
{
    QString strFilePath;
    char buffer[1024];
    QString strBatteryCapacity;
    //int batteryCapacity = 0;
    int batteryLevel = 0;
    bool result = false;

    //FILE *fp = fopen("/sys/class/power_supply/BAT0/capacity", "r");
    FILE *fp = fopen("/sys/class/power_supply/battery/capacity", "r");
    if (fp != NULL)
    {
        fread(buffer, 1, sizeof(buffer), fp);
    }

    strBatteryCapacity = buffer;


    QStringList strList = strBatteryCapacity.split('\n');
    //qDebug() << "Battery Cap: " << static_cast<QString>(strList.at(0));

    strBatteryCapacity = static_cast<QString>(strList.at(0));
    //batteryCapacity = strBatteryCapacity.toInt(&result, 10);

    return strBatteryCapacity;
}

QPixmap DeviceBattery::getDeviceBatteryCapacity()
{
    QString strFilePath;
    char buffer[1024];
    QString strBatteryCapacity;
    int batteryCapacity = 0;
    int batteryLevel = 0;
    bool result = false;

    //FILE *fp = fopen("/sys/class/power_supply/BAT0/capacity", "r");
    FILE *fp = fopen("/sys/class/power_supply/battery/capacity", "r");
    if (fp != NULL)
    {
        fread(buffer, 1, sizeof(buffer), fp);
    }

    strBatteryCapacity = buffer;


    QStringList strList = strBatteryCapacity.split('\n');
    //qDebug() << "Battery Cap: " << static_cast<QString>(strList.at(0));

    strBatteryCapacity = static_cast<QString>(strList.at(0));
    batteryCapacity = strBatteryCapacity.toInt(&result, 10);

    if (batteryCapacity >= 0 && batteryCapacity <= 10)
        batteryLevel = 1;
    else if (batteryCapacity >= 11 && batteryCapacity <= 20)
        batteryLevel = 2;
    else if (batteryCapacity >= 21 && batteryCapacity <= 30)
        batteryLevel = 3;
    else if (batteryCapacity >= 31 && batteryCapacity <= 40)
        batteryLevel = 4;
    else if (batteryCapacity >= 41 && batteryCapacity <= 50)
        batteryLevel = 5;
    else if (batteryCapacity >= 51 && batteryCapacity <= 60)
        batteryLevel = 6;
    else if (batteryCapacity >= 61 && batteryCapacity <= 70)
        batteryLevel = 7;
    else if (batteryCapacity >= 71 && batteryCapacity <= 80)
        batteryLevel = 8;
    else if (batteryCapacity >= 81 && batteryCapacity <= 90)
        batteryLevel = 9;
    else if (batteryCapacity >= 91 && batteryCapacity <= 100)
        batteryLevel = 10;

    switch (batteryLevel) {
    case 1:
        strFilePath = "://resource/battery1.png";
        break;
    case 2:
        strFilePath = "://resource/battery2.png";
        break;
    case 3:
        strFilePath = "://resource/battery3.png";
        break;
    case 4:
        strFilePath = "://resource/battery4.png";
        break;
    case 5:
        strFilePath = "://resource/battery5.png";
        break;
    case 6:
        strFilePath = "://resource/battery6.png";
        break;
    case 7:
        strFilePath = "://resource/battery7.png";
        break;
    case 8:
        strFilePath = "://resource/battery8.png";
        break;
    case 9:
        strFilePath = "://resource/battery9.png";
        break;
    case 10:
        strFilePath = "://resource/battery10.png";
        break;
    default:
        break;
    }

    QPixmap *pixmap = new QPixmap();
    pixmap->load(strFilePath, 0, Qt::AvoidDither | Qt::ThresholdDither | Qt::ThresholdAlphaDither);

    return *pixmap;
}

QString DeviceBattery::isDeviceBatteryChargingString()
{
    char buffer[1024];
    bool isCharging = false;

    FILE *fp = fopen("sys/class/power_supply/BAT0/status", "r");
    //FILE *fp = fopen("sys/class/power_supply/battery/status", "r");
    if (fp != NULL)
    {
        fread(buffer, 1, sizeof(buffer), fp);
    }

    return buffer;
}

bool DeviceBattery::isDeviceBatteryCharging()
{
    char buffer[1024];
    bool isCharging = false;

    //FILE *fp = fopen("sys/class/power_supply/BAT0/status", "r");
    FILE *fp = fopen("sys/class/power_supply/battery/status", "r");
    if (fp != NULL)
    {
        fread(buffer, 1, sizeof(buffer), fp);
    }

    //Full Discharging Charging

    if (memcmp(buffer, "Full", strlen("Full")) == 0)
    {
        isCharging = false;
    }
    else if (memcmp(buffer, "Discharging", strlen("Discharging")) == 0)
    {
        isCharging = false;
    }
    else if (memcmp(buffer, "Charging", strlen("Charging")) == 0)
    {
        isCharging = true;
    }

    return isCharging;
}
