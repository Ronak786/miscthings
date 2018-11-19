#include "dialogledtest.h"
#include "ui_dialogledtest.h"
#include <QKeyEvent>

DialogLedTest::DialogLedTest(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogLedTest)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::FramelessWindowHint);

    m_DeviceLed = new DeviceLed();
}

DialogLedTest::~DialogLedTest()
{
    m_DeviceLed->deleteLater();
    delete ui;
}

void DialogLedTest::keyPressEvent(QKeyEvent *event)
{
    switch (event->key())
    {
    case Qt::Key_1:
        m_DeviceLed->LedOn(LED_RED);
        break;
    case Qt::Key_2:
        m_DeviceLed->LedOff(LED_RED);
        break;
    case Qt::Key_3:
        m_DeviceLed->LedOn(LED_GREEN);
        break;
    case Qt::Key_4:
        m_DeviceLed->LedOff(LED_GREEN);
        break;
    case Qt::Key_5:
        m_DeviceLed->LedOn(LED_BLUE);
        break;
    case Qt::Key_6:
        m_DeviceLed->LedOff(LED_BLUE);
        break;
    case Qt::Key_7:
        m_DeviceLed->LedOn(LED_RED);
        m_DeviceLed->LedOn(LED_GREEN);
        m_DeviceLed->LedOn(LED_BLUE);
        break;
    case Qt::Key_8:
        m_DeviceLed->LedOff(LED_RED);
        m_DeviceLed->LedOff(LED_GREEN);
        m_DeviceLed->LedOff(LED_BLUE);
        break;
    case Qt::Key_Escape:
        this->accept();
    default:
        break;
    }
}
