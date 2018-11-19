#ifndef DIALOGCLOCKSETTING_H
#define DIALOGCLOCKSETTING_H

#include <QDialog>
#include <QTimer>

namespace Ui {
class DialogClockSetting;
}

class DialogClockSetting : public QDialog
{
    Q_OBJECT

public:
    explicit DialogClockSetting(QWidget *parent = 0);
    ~DialogClockSetting();

    void setDateTime();

private slots:
    void updateCurrentTime();

private:
    Ui::DialogClockSetting *ui;
    QTimer      *m_Timer;
};

#endif // DIALOGCLOCKSETTING_H
