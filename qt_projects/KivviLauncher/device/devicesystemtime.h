#ifndef DEVICESYSTEMTIME_H
#define DEVICESYSTEMTIME_H

#include <QObject>

class DeviceSystemTime : public QObject
{
    Q_OBJECT
public:
    explicit DeviceSystemTime(QObject *parent = nullptr);

    QString getDeviceSystemDate();
    QString getDeviceSystemTime();

signals:

public slots:
};

#endif // DEVICESYSTEMTIME_H
