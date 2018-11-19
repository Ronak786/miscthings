#include "deviceled.h"

DeviceLed::DeviceLed(QObject *parent) : QObject(parent)
{
    //LED Red
    system("echo 23 > /sys/class/gpio/export");
    system("echo out > /sys/class/gpio/gpio23/direction");

    //LED Green
    system("echo 28 > /sys/class/gpio/export");
    system("echo out > /sys/class/gpio/gpio28/direction");

    //LED Blue
    system("echo 10 > /sys/class/gpio/export");
    system("echo out > /sys/class/gpio/gpio10/direction");
}

void DeviceLed::LedOn(LED_TypeDef ledType)
{
    switch (ledType) {
    case LED_RED:
        system("echo 1 > /sys/class/gpio/gpio23/value");
        break;
    case LED_GREEN:
        system("echo 1 > /sys/class/gpio/gpio28/value");
        break;
    case LED_BLUE:
        system("echo 1 > /sys/class/gpio/gpio10/value");
        break;
    default:
        break;
    }
}

void DeviceLed::LedOff(LED_TypeDef ledType)
{
    switch (ledType) {
    case LED_RED:
        system("echo 0 > /sys/class/gpio/gpio23/value");
        break;
    case LED_GREEN:
        system("echo 0 > /sys/class/gpio/gpio28/value");
        break;
    case LED_BLUE:
        system("echo 0 > /sys/class/gpio/gpio10/value");
        break;
    default:
        break;
    }
}
