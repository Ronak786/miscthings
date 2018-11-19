#ifndef DEVICEUART_H
#define DEVICEUART_H

#include <QObject>

class DeviceUart : public QObject
{
    Q_OBJECT
public:
    explicit DeviceUart(QObject *parent = nullptr);

signals:

public slots:
};

#endif // DEVICEUART_H