#include "dialograser.h"

DialogRaser::DialogRaser(QWidget *parent):
    QDialog(parent),
    ui(new Ui::DialogRaser)
{
    ui->setupUi(this);

}

DialogRaser::~DialogRaser() {
}

void DialogRaser::startRaser() {

}

void DialogRaser::stopRaser() {

}
