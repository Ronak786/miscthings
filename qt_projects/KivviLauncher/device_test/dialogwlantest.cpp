#include "dialogwlantest.h"
#include "ui_dialogwlantest.h"

DialogWlanTest::DialogWlanTest(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogWlanTest)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::FramelessWindowHint);
}

DialogWlanTest::~DialogWlanTest()
{
    delete ui;
}

