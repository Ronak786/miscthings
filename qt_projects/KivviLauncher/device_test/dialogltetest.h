#ifndef DIALOGLTETEST_H
#define DIALOGLTETEST_H

#include <QDialog>
#include <QKeyEvent>
#include <QGraphicsScene>


namespace Ui {
class DialogLteTest;
}

class DialogLteTest : public QDialog
{
    Q_OBJECT

public:
    explicit DialogLteTest(QWidget *parent = 0);
    ~DialogLteTest();
    void keyPressEvent(QKeyEvent *event) override;

private:
    void start4G();
    void stop4G();

private:
    Ui::DialogLteTest *ui;
    bool started;
};

#endif // DIALOGLTETEST_H
