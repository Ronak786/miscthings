#include "dialogkeypadtest.h"
#include "ui_dialogkeypadtest.h"
#include <QKeyEvent>

DialogKeypadTest::DialogKeypadTest(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogKeypadTest)
{
    ui->setupUi(this);

    this->setWindowFlags(Qt::FramelessWindowHint);
}

DialogKeypadTest::~DialogKeypadTest()
{
    delete ui;
}

void DialogKeypadTest::keyPressEvent(QKeyEvent *event)
{
    switch (event->key())
    {
    case Qt::Key_0:
        ui->pushButton_0->setStyleSheet("background-color:red");
        break;
    case Qt::Key_1:
        ui->pushButton_1->setStyleSheet("background-color:red");
        break;
    case Qt::Key_2:
        ui->pushButton_2->setStyleSheet("background-color:red");
        break;
    case Qt::Key_3:
        ui->pushButton_3->setStyleSheet("background-color:red");
        break;
    case Qt::Key_4:
        ui->pushButton_4->setStyleSheet("background-color:red");
        break;
    case Qt::Key_5:
        ui->pushButton_5->setStyleSheet("background-color:red");
        break;
    case Qt::Key_6:
        ui->pushButton_6->setStyleSheet("background-color:red");
        break;
    case Qt::Key_7:
        ui->pushButton_7->setStyleSheet("background-color:red");
        break;
    case Qt::Key_8:
        ui->pushButton_8->setStyleSheet("background-color:red");
        break;
    case Qt::Key_9:
        ui->pushButton_9->setStyleSheet("background-color:red");
        break;
    case Qt::Key_Space:
        ui->pushButton_Enter->setStyleSheet("background-color:red");
        break;
    case Qt::Key_Escape:
        //emit;
        ui->pushButton_0->setStyleSheet("background-color:white");
        ui->pushButton_1->setStyleSheet("background-color:white");
        ui->pushButton_2->setStyleSheet("background-color:white");
        ui->pushButton_3->setStyleSheet("background-color:white");
        ui->pushButton_4->setStyleSheet("background-color:white");
        ui->pushButton_5->setStyleSheet("background-color:white");
        ui->pushButton_6->setStyleSheet("background-color:white");
        ui->pushButton_7->setStyleSheet("background-color:white");
        ui->pushButton_8->setStyleSheet("background-color:white");
        ui->pushButton_9->setStyleSheet("background-color:white");
        ui->pushButton_Enter->setStyleSheet("background-color:white");
        this->close();
        break;
    case Qt::Key_Delete:
        break;
    }
}
