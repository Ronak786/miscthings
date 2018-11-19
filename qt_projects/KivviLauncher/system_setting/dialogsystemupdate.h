#ifndef DIALOGSYSTEMUPDATE_H
#define DIALOGSYSTEMUPDATE_H

#include <QDialog>

namespace Ui {
class DialogSystemUpdate;
}

class DialogSystemUpdate : public QDialog
{
    Q_OBJECT

public:
    explicit DialogSystemUpdate(QWidget *parent = 0);
    ~DialogSystemUpdate();

private:
    Ui::DialogSystemUpdate *ui;
};

#endif // DIALOGSYSTEMUPDATE_H
