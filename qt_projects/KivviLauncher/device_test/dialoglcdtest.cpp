#include "dialoglcdtest.h"
#include "ui_dialoglcdtest.h"
#include <QPalette>

DialogLCDTest::DialogLCDTest(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogLCDTest)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::FramelessWindowHint);

    m_Timer = new QTimer();
    m_Couter = 0;

    connect(m_Timer, SIGNAL(timeout()), this, SLOT(refreshLCD()));
    m_Timer->start(1000);
}

DialogLCDTest::~DialogLCDTest()
{
    delete ui;
    m_Timer->stop();
}

void DialogLCDTest::refreshLCD()
{
    m_Couter++;
    if (m_Couter == 1)
    {
        this->setAutoFillBackground(true);
        this->setPalette(QPalette(QColor(255, 0, 0)));
    }
    else if (m_Couter == 2)
    {
        this->setAutoFillBackground(true);
        this->setPalette(QPalette(QColor(0, 255, 0)));
    }
    else if (m_Couter == 3)
    {
        this->setAutoFillBackground(true);
        this->setPalette(QPalette(QColor(0, 0, 255)));
        m_Couter = 0;
    }
}

