#include "devicesysteminfo.h"
#include <QSysInfo>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QNetworkInterface>

DeviceSystemInfo::DeviceSystemInfo(QObject *parent) : QObject(parent)
{

}

QString DeviceSystemInfo::getProductName()
{
    //TODO
    return "";
}

QString DeviceSystemInfo::getBuildDatetime()
{
    //TODO
    return "";
}

QString DeviceSystemInfo::getBuildAbi()
{
    return QSysInfo::buildAbi();
}

QString DeviceSystemInfo::getBuildCpuArchitecture()
{
    return QSysInfo::buildCpuArchitecture();
}

QString DeviceSystemInfo::getCurrentCpuArchitecture()
{
    return QSysInfo::currentCpuArchitecture();
}

QString DeviceSystemInfo::getKernelType()
{
    return QSysInfo::kernelType();
}

QString DeviceSystemInfo::getKernelVersion()
{
    return QSysInfo::kernelVersion();
}

QString DeviceSystemInfo::getMachineHostName()
{
    return QSysInfo::machineHostName();
}

QString DeviceSystemInfo::getprettyProductName()
{
    return QSysInfo::prettyProductName();
}

QString DeviceSystemInfo::getProductType()
{
    return QSysInfo::productType();
}

QString DeviceSystemInfo::getProductVersion()
{
    return QSysInfo::productVersion();
}

QString DeviceSystemInfo::getHostIpAddress()
{
    QString strIpAddress;
    QList<QHostAddress> listIpAddress = QNetworkInterface::allAddresses();

    for (int i = 0; i < listIpAddress.size(); i++)
    {
        if (listIpAddress.at(i) != QHostAddress::LocalHost &&
                listIpAddress.at(i).toIPv4Address())
        {
            strIpAddress = listIpAddress.at(i).toString();
            break;
        }
    }

    if (strIpAddress.isEmpty())
    {
        strIpAddress = QHostAddress(QHostAddress::LocalHost).toString();
    }

    return strIpAddress;
}

QString DeviceSystemInfo::getHostMacAddress()
{
    QString strMacAddr = "";
    QList<QNetworkInterface> listNets = QNetworkInterface::allInterfaces();

    for (int i = 0; i < listNets.count(); i++)
    {
        // 如果此网络接口被激活并且正在运行并且不是回环地址，则就是我们需要找的Mac地址
        if (listNets[i].flags().testFlag(QNetworkInterface::IsUp) &&
                listNets[i].flags().testFlag(QNetworkInterface::IsRunning) &&
                !listNets[i].flags().testFlag(QNetworkInterface::IsLoopBack))
        {
            strMacAddr = listNets[i].hardwareAddress();
            break;
        }
    }

    return strMacAddr;
}
