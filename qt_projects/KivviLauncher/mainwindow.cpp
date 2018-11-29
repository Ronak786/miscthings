#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QStatusBar>
#include <QKeyEvent>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->setWindowFlags(Qt::FramelessWindowHint);

    m_DeviceMobileSignal = new DeviceMobileSignal();
    m_DeviceSystemTime = new DeviceSystemTime();
    m_DeviceBattery = new DeviceBattery();



    updateSignalLevel();
    updateSystemDatetime();
    updateBetteryLevel();

    m_Timer = new QTimer();
    connect(m_Timer, SIGNAL(timeout()), this, SLOT(updateSignalLevel()));
    connect(m_Timer, SIGNAL(timeout()), this, SLOT(updateSystemDatetime()));
    connect(m_Timer, SIGNAL(timeout()), this, SLOT(updateBetteryLevel()));
    m_Timer->start(1000);

    //Remove Title Bar
    //this->setWindowFlags(Qt::FramelessWindowHint);

    //QHBoxLayout *hBoxLayout = new QHBoxLayout();
    //QVBoxLayout *vBoxLayout = new QVBoxLayout();

    //m_WidgetStatusBar = new WidgetStatusBar();
    //m_WidgetStatusBar->show();

    //vBoxLayout->addWidget(m_WidgetStatusBar);
    //vBoxLayout->addWidget(label);


    //this->setLayout(vBoxLayout);
    //this->show();

    //this->setStatusBar(statusBar);

    //m_WidgetStatusBar = new WidgetStatusBar();

    //ui->label->setText(m_DeviceSystemInfo->getBuildCpuArchitecture());

    //


}

MainWindow::~MainWindow()
{
    delete ui;

    m_Timer->deleteLater();
    m_DeviceMobileSignal->deleteLater();
    m_DeviceSystemTime->deleteLater();
    m_DeviceBattery->deleteLater();

}

void MainWindow::updateSignalLevel()
{
    ui->label_MobileSignal->setPixmap(m_DeviceMobileSignal->getDeviceMobileSignalLevel());
}

void MainWindow::updateSystemDatetime()
{
    ui->label_SystemTime->setText(m_DeviceSystemTime->getDeviceSystemTime());
}

void MainWindow::updateBetteryLevel()
{
    ui->label_Battery->setPixmap(m_DeviceBattery->getDeviceBatteryCapacity());
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    switch (event->key())
    {
    case Qt::Key_0:
        break;
    case Qt::Key_1:       
        m_DialogSystemSetting = new DialogSystemSetting();
        m_DialogSystemSetting->exec();
        m_DialogSystemSetting->deleteLater();
        break;
    case Qt::Key_2:
        m_DialogDeviceTest = new DialogDeviceTest();
        m_DialogDeviceTest->exec();        
        m_DialogDeviceTest->deleteLater();
        break;
    case Qt::Key_3:
        break;
    case Qt::Key_4:
        break;
    case Qt::Key_5:
        break;
    case Qt::Key_6:
        break;
    case Qt::Key_7:
        break;
    case Qt::Key_8:
        break;
    case Qt::Key_9:
        break;
    case Qt::Key_Delete:
        break;
    case Qt::Key_Escape:
        break;
    case Qt::Key_Space:
        break;
    case Qt::Key_F1:
        break;
    }
}

