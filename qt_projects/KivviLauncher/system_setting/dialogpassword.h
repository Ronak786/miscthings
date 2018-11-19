#ifndef DIALOGPASSWORD_H
#define DIALOGPASSWORD_H

#include <QDialog>

namespace Ui {
class DialogPassword;
}

class DialogPassword : public QDialog
{
    Q_OBJECT

public:
    explicit DialogPassword(QWidget *parent = 0);
    ~DialogPassword();

    void keyPressEvent(QKeyEvent *event);

private:
    Ui::DialogPassword *ui;
};

#endif // DIALOGPASSWORD_H
