#ifndef DEVICEBATTERY_H
#define DEVICEBATTERY_H

#include <QObject>
#include <QPixmap>

class DeviceBattery : public QObject
{
    Q_OBJECT
public:
    explicit DeviceBattery(QObject *parent = nullptr);
    QString getDeviceBatteryCapacityString();
    QPixmap getDeviceBatteryCapacity();
    QString isDeviceBatteryChargingString();
    bool isDeviceBatteryCharging();

signals:

public slots:

};

#endif // DEVICEBATTERY_H
