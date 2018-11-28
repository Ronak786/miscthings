#ifndef PASSDIALOG_H
#define PASSDIALOG_H

#include <QWidget>
#include <QString>
#include <QKeyEvent>
#include "ui_passdialog.h"

namespace Ui {
class PassDialog;
}

class PassDialog : public QDialog
{
    Q_OBJECT
public:
    PassDialog(QString ssid, QWidget *parent = nullptr);
    ~PassDialog();
    void getresult(QString *);
    void keyPressEvent(QKeyEvent *);
private:
    Ui::PassDialog *ui;
    QString _ssid;
};

#endif // PASSDIALOG_H
