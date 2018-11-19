#ifndef DEVICEMOBILESIGNAL_H
#define DEVICEMOBILESIGNAL_H

#include <QObject>
#include <QString>
#include <QPixmap>

class DeviceMobileSignal : public QObject
{
    Q_OBJECT
public:
    explicit DeviceMobileSignal(QObject *parent = nullptr);
    virtual ~DeviceMobileSignal();

    QPixmap getDeviceMobileSignalLevel();

signals:

public slots:
};

#endif // DEVICEMOBILESIGNAL_H
