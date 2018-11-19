#include "dialogclocksetting.h"
#include "ui_dialogclocksetting.h"
#include <QDate>
#include <QTime>
#include <QDateTime>

DialogClockSetting::DialogClockSetting(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogClockSetting)
{
    ui->setupUi(this);

    this->setWindowFlags(Qt::FramelessWindowHint);

    QDate currentDate = QDate::currentDate();
    QTime currentTime = QTime::currentTime();

    ui->lineEdit_Year->setText(QString::number(currentDate.year()));
    ui->lineEdit_Month->setText(QString::number(currentDate.month()));
    ui->lineEdit_Day->setText(QString::number(currentDate.day()));
    ui->lineEdit_Hour->setText(QString::number(currentTime.hour()));
    ui->lineEdit_Minute->setText(QString::number(currentTime.minute()));
    ui->lineEdit_Second->setText(QString::number(currentTime.second()));

    QDateTime dateTime = QDateTime::currentDateTime();

    QString strDateTime = dateTime.toString("yyyy-MM-dd hh:mm:ss ddd");

    ui->label_DateTime->setText(strDateTime);

    m_Timer = new QTimer();
    connect(m_Timer, SIGNAL(timeout()), this, SLOT(updateCurrentTime()));
    m_Timer->start(1000);
}

DialogClockSetting::~DialogClockSetting()
{
    delete ui;
}

void DialogClockSetting::updateCurrentTime()
{
    //QDate currentDate = QDate::currentDate();
    //QTime currentTime = QTime::currentTime();

    QDateTime dateTime = QDateTime::currentDateTime();

    QString strDateTime = dateTime.toString("yyyy-MM-dd hh:mm:ss ddd");

    ui->label_DateTime->setText(strDateTime);
}

void DialogClockSetting::setDateTime()
{
    bool ok = false;

    QString year = ui->lineEdit_Year->text();
    QString month = ui->lineEdit_Month->text();
    QString day = ui->lineEdit_Day->text();
    QString hour = ui->lineEdit_Hour->text();
    QString minute = ui->lineEdit_Minute->text();
    QString second = ui->lineEdit_Second->text();

//    QDateTime dateTime = QDateTime::currentDateTime();
//    QDate date = QDate::currentDate();
//    date.setDate(year.toInt(&ok, 10), month.toInt(&ok, 10), day.toInt(&ok, 10));
//    QTime time = QTime::currentTime();



//    dateTime.setDate(date);

    time_t time;

    struct tm t;

    t.tm_year = year.toInt(&ok, 10) - 1900;
    t.tm_mon = month.toInt(&ok, 10);
    t.tm_mday = day.toInt(&ok, 10);
    t.tm_hour = hour.toInt(&ok, 10);
    t.tm_min = minute.toInt(&ok, 10);
    t.tm_sec = second.toInt(&ok, 10);

    time = mktime(&t);
    stime(&time);
}
