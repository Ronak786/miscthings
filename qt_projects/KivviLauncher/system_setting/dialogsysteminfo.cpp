#include "dialogsysteminfo.h"
#include "ui_dialogsysteminfo.h"
#include <QKeyEvent>

DialogSystemInfo::DialogSystemInfo(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogSystemInfo)
{
    ui->setupUi(this);

    this->setWindowFlags(Qt::FramelessWindowHint);

    m_DeviceSystemInfo = new DeviceSystemInfo();

    ui->label_BuildAbi->setText(m_DeviceSystemInfo->getBuildAbi());
    ui->label_CurrentCpuArchitecture->setText(m_DeviceSystemInfo->getCurrentCpuArchitecture());
    ui->label_KernelVersion->setText(m_DeviceSystemInfo->getKernelVersion());
    ui->label_ProductType->setText(m_DeviceSystemInfo->getProductType());
    ui->label_ProductVersion->setText(m_DeviceSystemInfo->getProductVersion());
    ui->label_IpAddress->setText(m_DeviceSystemInfo->getHostIpAddress());
    ui->label_WlanMacAddress->setText(m_DeviceSystemInfo->getHostMacAddress());
}

DialogSystemInfo::~DialogSystemInfo()
{
    delete ui;
}

void DialogSystemInfo::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape)
    {
        this->close();
    }
}
