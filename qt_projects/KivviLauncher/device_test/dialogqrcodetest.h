#ifndef DIALOGQRCODETEST_H
#define DIALOGQRCODETEST_H

#include <QDialog>
#include "device/deviceqrcode.h"

namespace Ui {
class DialogQRCodeTest;
}

class DialogQRCodeTest : public QDialog
{
    Q_OBJECT

public:
    explicit DialogQRCodeTest(QWidget *parent = 0);
    ~DialogQRCodeTest();

    void keyPressEvent(QKeyEvent *event);

private:
    Ui::DialogQRCodeTest *ui;

    DeviceQRCode    *m_DeviceQRCode;
};

#endif // DIALOGQRCODETEST_H
