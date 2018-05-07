#include "imgwin.h"
#include "ui_imgwin.h"
#include "nkrypto.h"

imgWin::imgWin(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::imgWin)
{
    ui->setupUi(this);
    ui->labelA->setText("labelA");
    ui->labelB->setText("labelB");
    QImageReader reader(QString("../imageqt/image1.bmp"));
    reader.setAutoDetectImageFormat(true);
    QImage img = reader.read();
    if (!img.isNull()) {
        ui->labelImage->setPixmap(QPixmap::fromImage(img));
    } else {
        ui->labelImage->setText("wrong image path");
    }

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(info()));
    timer->start(1000);

}

imgWin::~imgWin()
{
    delete ui;
}

void imgWin::info() {
    static int i = 0;
    ui->labelA->setText(QString::number(i));
    ui->labelB->setText(QString::number(i++));
}
