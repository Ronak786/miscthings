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
    connect(this, SIGNAL(shut()), this, SLOT(response()));

    thread = new KThread(this);
    connect(thread, SIGNAL(tell()), this, SLOT(info()));
    thread->start();
}

imgWin::~imgWin()
{
    delete ui;
}

void imgWin::info() {
    static int i = 0;
    ui->labelA->setText(QString::number(i));
    ui->labelB->setText(QString::number(i++));
    if (i == 10) {
        emit shut();
    }
}

void imgWin::response() {
    qWarning("begin shutdown\n");
    delete thread;
}
