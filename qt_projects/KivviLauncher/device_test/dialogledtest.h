#ifndef DIALOGLEDTEST_H
#define DIALOGLEDTEST_H

#include <QDialog>
#include "device/deviceled.h"

namespace Ui {
class DialogLedTest;
}

class DialogLedTest : public QDialog
{
    Q_OBJECT

public:
    explicit DialogLedTest(QWidget *parent = 0);
    ~DialogLedTest();
    void keyPressEvent(QKeyEvent *event);

private:
    Ui::DialogLedTest *ui;
    DeviceLed   *m_DeviceLed;
};

#endif // DIALOGLEDTEST_H
