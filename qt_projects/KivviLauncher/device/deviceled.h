#ifndef DEVICELED_H
#define DEVICELED_H

#include <QObject>

typedef enum
{
    LED_RED,
    LED_GREEN,
    LED_BLUE
} LED_TypeDef;

class DeviceLed : public QObject
{
    Q_OBJECT
public:
    explicit DeviceLed(QObject *parent = nullptr);

    void LedOn(LED_TypeDef ledType);
    void LedOff(LED_TypeDef ledType);

signals:

public slots:
};

#endif // DEVICELED_H
