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
    ui->labelImage->setText("wrong image path");

    connect(this, SIGNAL(shut()), this, SLOT(response()));

    thread = new KThread(this);
    connect(thread, SIGNAL(tell(QImage)), this, SLOT(info(QImage)));
    connect(thread, SIGNAL(showFps(int)), this, SLOT(displayFps(int)));
    thread->start();
}

imgWin::~imgWin()
{
    delete ui;
}

void imgWin::info(QImage img) {
    ui->labelImage->setPixmap(QPixmap::fromImage(img));
}

void imgWin::response() {
    qWarning("begin shutdown\n");
    delete thread;
}

void imgWin::displayFps(int fps) {
    ui->labelA->setText(QString::number(fps) + " fps");
}

void imgWin::closeEvent(QCloseEvent *event) {
    qDebug("we are now closing in event\n");
    delete thread;
    QMainWindow::closeEvent(event);
}
