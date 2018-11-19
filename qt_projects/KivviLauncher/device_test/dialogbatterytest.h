#ifndef DIALOGBATTERYTEST_H
#define DIALOGBATTERYTEST_H

#include <QDialog>
#include "device/devicebattery.h"

namespace Ui {
class DialogBatteryTest;
}

class DialogBatteryTest : public QDialog
{
    Q_OBJECT

public:
    explicit DialogBatteryTest(QWidget *parent = 0);
    ~DialogBatteryTest();

private:
    Ui::DialogBatteryTest *ui;
    DeviceBattery       *m_DeviceBattery;
};

#endif // DIALOGBATTERYTEST_H
