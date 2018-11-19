#ifndef DIALOGDEVICETEST_H
#define DIALOGDEVICETEST_H

#include <QDialog>
#include "device_test/dialogkeypadtest.h"
#include "device_test/dialogqrcodetest.h"
#include "device_test/dialoglcdtest.h"
#include "device_test/dialogbatterytest.h"
#include "device_test/dialoggpstest.h"
#include "device_test/dialogledtest.h"
#include "device_test/dialogcameratest.h"
#include "device_test/dialogwlantest.h"
#include "device_test/dialogltetest.h"
#include "device_test/dialogbttest.h"

namespace Ui {
class DialogDeviceTest;
}

class DialogDeviceTest : public QDialog
{
    Q_OBJECT

public:
    explicit DialogDeviceTest(QWidget *parent = 0);
    ~DialogDeviceTest();

    void keyPressEvent(QKeyEvent *event);

private:
    Ui::DialogDeviceTest *ui;
    DialogKeypadTest    *m_DialogKeypadTest;
    DialogQRCodeTest    *m_DialogQRCodeTest;
    DialogLCDTest       *m_DialogLCDTest;
    DialogBatteryTest   *m_DialogBatteryTest;
    DialogGpsTest       *m_DialogGpsTest;
    DialogLedTest       *m_DialogLedTest;
    DialogCameraTest    *m_DialogCameraTest;
    DialogWlanTest      *m_DialogWlanTest;
    DialogLteTest       *m_DialogLteTest;
    DialogBtTest        *m_DialogBtTest;
};

#endif // DIALOGDEVICETEST_H
