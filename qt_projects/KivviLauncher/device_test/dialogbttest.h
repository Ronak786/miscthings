#ifndef DIALOGBTTEST_H
#define DIALOGBTTEST_H

#include <QDialog>

namespace Ui {
class DialogBtTest;
}

class DialogBtTest : public QDialog
{
    Q_OBJECT

public:
    explicit DialogBtTest(QWidget *parent = 0);
    ~DialogBtTest();

private:
    Ui::DialogBtTest *ui;
};

#endif // DIALOGBTTEST_H
