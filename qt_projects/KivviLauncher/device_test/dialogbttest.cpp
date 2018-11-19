#include "dialogbttest.h"
#include "ui_dialogbttest.h"

DialogBtTest::DialogBtTest(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogBtTest)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::FramelessWindowHint);
}

DialogBtTest::~DialogBtTest()
{
    delete ui;
}
