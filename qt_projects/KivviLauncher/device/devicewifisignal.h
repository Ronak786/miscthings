#ifndef DEVICEWIFISIGNAL_H
#define DEVICEWIFISIGNAL_H

#include <QObject>

class DeviceWifiSignal : public QObject
{
    Q_OBJECT
public:
    explicit DeviceWifiSignal(QObject *parent = nullptr);
    int getDeviceWifiSignal();

signals:

public slots:
};

#endif // DEVICEWIFISIGNAL_H
