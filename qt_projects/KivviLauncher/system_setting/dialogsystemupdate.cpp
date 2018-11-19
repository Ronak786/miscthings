#include "dialogsystemupdate.h"
#include "ui_dialogsystemupdate.h"

#include <QDebug>

DialogSystemUpdate::DialogSystemUpdate(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogSystemUpdate)
{
    ui->setupUi(this);

    //ui->label_CurrentVersion->setText();

    //PkgHandle handle;
    //if (0 != init_handle(&handle))
    //{
    //    qDebug() << "init handle fail";
    //}


    ui->label_LatestVersion->setText("The latest version");

}

DialogSystemUpdate::~DialogSystemUpdate()
{
    delete ui;
}


