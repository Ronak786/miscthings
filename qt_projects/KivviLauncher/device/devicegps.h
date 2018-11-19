#ifndef DEVICEGPS_H
#define DEVICEGPS_H

#include <QObject>

class DeviceGps : public QObject
{
    Q_OBJECT
public:
    explicit DeviceGps(QObject *parent = nullptr);

    void transformat(QString str);
    void parseGpsInfo(QString gpsInfo);


signals:

public slots:

public:
    QString m_Latitude;
    QString m_Longitude;
    QString m_Speed;
    QString m_Status;
};

#endif // DEVICEGPS_H
