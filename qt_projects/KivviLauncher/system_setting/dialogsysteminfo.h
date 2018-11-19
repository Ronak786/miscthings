#ifndef DIALOGSYSTEMINFO_H
#define DIALOGSYSTEMINFO_H

#include <QDialog>
#include "device/devicesysteminfo.h"

namespace Ui {
class DialogSystemInfo;
}

class DialogSystemInfo : public QDialog
{
    Q_OBJECT

public:
    explicit DialogSystemInfo(QWidget *parent = 0);
    ~DialogSystemInfo();

    void keyPressEvent(QKeyEvent *event);

private:
    Ui::DialogSystemInfo *ui;
    DeviceSystemInfo    *m_DeviceSystemInfo;
};

#endif // DIALOGSYSTEMINFO_H
