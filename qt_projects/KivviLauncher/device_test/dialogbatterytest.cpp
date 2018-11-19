#include "dialogbatterytest.h"
#include "ui_dialogbatterytest.h"

DialogBatteryTest::DialogBatteryTest(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogBatteryTest)
{
    ui->setupUi(this);

    this->setWindowFlags(Qt::FramelessWindowHint);

    m_DeviceBattery = new DeviceBattery();

    ui->label_CurrentBattery->setText(m_DeviceBattery->getDeviceBatteryCapacityString());
    ui->label_ChargingStatus->setText(m_DeviceBattery->isDeviceBatteryChargingString());
}

DialogBatteryTest::~DialogBatteryTest()
{
    delete ui;
}
