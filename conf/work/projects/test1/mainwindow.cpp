#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    mtDetect = new QThread();
    workerDetect = new mthread();
    workerDetect->moveToThread(mtDetect);
    connect(mtDetect, SIGNAL(started()), workerDetect, SLOT(process()));
    connect(workerDetect, SIGNAL(finished()), mtDetect, SLOT(quit()));
    connect(workerDetect, SIGNAL(returnData(QString)), this, SLOT(mainshow(QString)));
    mtDetect->start();
}

MainWindow::~MainWindow()
{
    mtDetect->deleteLater();
    mtLoop->deleteLater();
    if (workerDetect) delete workerDetect;
    if (workerLoop) delete workerLoop;

    delete ui;

}

void MainWindow::mainshow(QString str) {
    qDebug() << "main begin show: " << str;

    mtLoop = new QThread();
    workerLoop = new mthread(str);
    workerLoop->moveToThread(mtLoop);
    connect(mtLoop, SIGNAL(started()), workerLoop, SLOT(processTarget()));
    connect(workerLoop, SIGNAL(returnDataTarget(QString)), this, SLOT(mainshowTarget(QString)));
    connect(workerLoop, SIGNAL(finished()), mtLoop, SLOT(quit()));

    mtLoop->start();
}

void MainWindow::mainshowTarget(QString str) {
    mutex.lock();
    flagData = str;
    mutex.unlock();
}



