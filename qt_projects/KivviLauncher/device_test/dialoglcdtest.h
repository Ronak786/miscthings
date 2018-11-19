#ifndef DIALOGLCDTEST_H
#define DIALOGLCDTEST_H

#include <QDialog>
#include <QTimer>

namespace Ui {
class DialogLCDTest;
}

class DialogLCDTest : public QDialog
{
    Q_OBJECT

public:
    explicit DialogLCDTest(QWidget *parent = 0);
    ~DialogLCDTest();

private slots:
    void refreshLCD();

private:
    Ui::DialogLCDTest *ui;

    QTimer      *m_Timer;
    int         m_Couter;
};

#endif // DIALOGLCDTEST_H
