#ifndef DIALOGRASER_H
#define DIALOGRASER_H

#include <QWidget>
#include <ui_dialograser.h>

namespace Ui {
class DialogRaser;
}

class DialogRaser : public QDialog
{
public:
    DialogRaser(QWidget *parent = NULL);
    ~DialogRaser();

private:
    Ui::DialogRaser *ui;

};

#endif // DIALOGRASER_H
