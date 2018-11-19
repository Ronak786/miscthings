#include "dialogpassword.h"
#include "ui_dialogpassword.h"
#include <QKeyEvent>

DialogPassword::DialogPassword(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogPassword)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::FramelessWindowHint);

}

DialogPassword::~DialogPassword()
{
    delete ui;
}

void DialogPassword::keyPressEvent(QKeyEvent *event)
{
    switch (event->key())
    {
    case Qt::Key_Escape:
        //this->destroy();
        this->close();
        break;
    }
}
