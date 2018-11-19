#ifndef DIALOGSYSTEMSETTING_H
#define DIALOGSYSTEMSETTING_H

#include <QDialog>
#include "system_setting/dialogclocksetting.h"
#include "system_setting/dialogsysteminfo.h"
#include "system_setting/dialogpassword.h"
#include "system_setting/dialogsystemupdate.h"

namespace Ui {
class DialogSystemSetting;
}

class DialogSystemSetting : public QDialog
{
    Q_OBJECT

public:
    explicit DialogSystemSetting(QWidget *parent = 0);
    ~DialogSystemSetting();

    void keyPressEvent(QKeyEvent *event);

private:
    Ui::DialogSystemSetting *ui;
    DialogClockSetting  *m_DialogClockSetting;
    DialogSystemInfo    *m_DialogSystemInfo;
    DialogPassword      *m_DialogPassword;
    DialogSystemUpdate  *m_DialogSystemUpdate;
};

#endif // DIALOGSYSTEMSETTING_H
