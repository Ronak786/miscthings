#include "dialogsystemsetting.h"
#include "ui_dialogsystemsetting.h"
#include <QKeyEvent>

DialogSystemSetting::DialogSystemSetting(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogSystemSetting)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::FramelessWindowHint);

    m_DialogClockSetting = new DialogClockSetting();
    m_DialogSystemInfo = new DialogSystemInfo();
    m_DialogPassword = new DialogPassword();
    m_DialogSystemUpdate = new DialogSystemUpdate();
}

DialogSystemSetting::~DialogSystemSetting()
{
    delete ui;
}

void DialogSystemSetting::keyPressEvent(QKeyEvent *event)
{
    switch (event->key())
    {
    case Qt::Key_1:
        m_DialogSystemUpdate->exec();
        break;
    case Qt::Key_2:
        m_DialogClockSetting->exec();
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
        m_DialogPassword->exec();
        break;
    case Qt::Key_Escape:
        this->accept();
        break;
    }
}

