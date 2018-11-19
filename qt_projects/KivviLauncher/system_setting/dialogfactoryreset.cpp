#include "dialogfactoryreset.h"
#include "ui_dialogfactoryreset.h"

DialogFactoryReset::DialogFactoryReset(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogFactoryReset)
{
    ui->setupUi(this);

    this->setWindowFlags(Qt::FramelessWindowHint);
}

DialogFactoryReset::~DialogFactoryReset()
{
    delete ui;
}
