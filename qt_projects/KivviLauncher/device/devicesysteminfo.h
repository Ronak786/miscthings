#ifndef DEVICESYSTEMINFO_H
#define DEVICESYSTEMINFO_H

#include <QObject>
#include <QString>

class DeviceSystemInfo : public QObject
{
    Q_OBJECT
public:
    explicit DeviceSystemInfo(QObject *parent = nullptr);

    QString getProductName();
    QString getBuildDatetime();
    QString getBuildAbi();
    QString getBuildCpuArchitecture();
    QString getCurrentCpuArchitecture();
    QString getKernelType();
    QString getKernelVersion();
    QString getMachineHostName();
    QString getprettyProductName();
    QString getProductType();
    QString getProductVersion();
    QString getHostIpAddress();
    QString getHostMacAddress();

signals:

public slots:
};

#endif // DEVICESYSTEMINFO_H
