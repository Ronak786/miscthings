#ifndef DIALOGKEYPADTEST_H
#define DIALOGKEYPADTEST_H

#include <QDialog>

namespace Ui {
class DialogKeypadTest;
}

class DialogKeypadTest : public QDialog
{
    Q_OBJECT

public:
    explicit DialogKeypadTest(QWidget *parent = 0);
    ~DialogKeypadTest();

    void keyPressEvent(QKeyEvent *event);

private:
    Ui::DialogKeypadTest *ui;
};

#endif // DIALOGKEYPADTEST_H
