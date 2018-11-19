#ifndef DEVICEQRCODE_H
#define DEVICEQRCODE_H

#include <QObject>

class DeviceQRCode : public QObject
{
    Q_OBJECT
public:
    explicit DeviceQRCode(QObject *parent = nullptr);
    virtual ~DeviceQRCode();

    QPixmap generateQRcode(QString str, int width, int height);

signals:

public slots:
};

#endif // DEVICEQRCODE_H
