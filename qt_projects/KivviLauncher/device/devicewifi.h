#ifndef DEVICEWIFI_H
#define DEVICEWIFI_H

#include <QObject>

class DeviceWifi : public QObject
{
    Q_OBJECT
public:
    explicit DeviceWifi(QObject *parent = nullptr);

signals:

public slots:
};

#endif // DEVICEWIFI_H