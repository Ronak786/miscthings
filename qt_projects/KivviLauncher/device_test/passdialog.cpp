#include "passdialog.h"
#include <QDebug>

PassDialog::PassDialog(QString ssid, QWidget *parent):
    QDialog(parent),
    ui(new Ui::PassDialog)
{
    ui->setupUi(this);
    ui->label->setText(ssid);
    _ssid = "not set yet";
}

PassDialog::~PassDialog() {
    delete ui;
}

void PassDialog::getresult(QString *ssid) {
    *ssid = _ssid;
    return;
}

void PassDialog::keyPressEvent(QKeyEvent *event) {
    switch(event->key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter:
        qDebug() << "get return";
        _ssid = ui->lineEdit->text();
        accept();
        break;
    case Qt::Key_Escape:
        qDebug() << "get escape";
        _ssid = ui->lineEdit->text();
        accept();
        break;
    default:
        qDebug() << "get " << event->key();
        QDialog::keyPressEvent(event);
        break;
    }
}
