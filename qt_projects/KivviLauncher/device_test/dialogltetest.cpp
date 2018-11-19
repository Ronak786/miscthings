#include "dialogltetest.h"
#include "ui_dialogltetest.h"
#include <stdio.h>

#include <QStringList>
#include <QDebug>


DialogLteTest::DialogLteTest(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogLteTest),
    started(false)
{
    ui->setupUi(this);
    ui->plainTextEdit->clear();
    ui->plainTextEdit->setReadOnly(true);
    this->setWindowFlags(Qt::FramelessWindowHint);

}

DialogLteTest::~DialogLteTest()
{
    delete ui;
}

void DialogLteTest::start4G()
{
    if (started) {
        return;
    }
    FILE *fp = popen("/data/bin/start4G", "r");
    if (!fp) {
        ui->plainTextEdit->appendPlainText("failed");
        return;
    }
    char buf[128];
    int num = fread(buf, sizeof(char), sizeof(buf), fp);
    if (num <= 0) {
        ui->plainTextEdit->appendPlainText("can not read " + QString::number(num));
        return;
    }
    buf[num] = '\0';


    // set gw
    QString result(buf);
    qDebug() << "begin search not registered";
    if (result.indexOf(QString("card not")) != -1) {
        ui->plainTextEdit->appendPlainText("card not registered");
        return;
    }
    qDebug() << "end search";
    QString ipgw = result.split(",")[3];
    QStringList ipgwlst = ipgw.split(".");
    QStringList iplst(QStringList() << ipgwlst[0] << ipgwlst[1] << ipgwlst[2] << ipgwlst[3]);
    QString ip = iplst.join(".");
    qDebug() << "we get ip " << ip;
    QString cmd = QString("ip route add default via ") + ip + QString(" dev seth_lte0");
    system(cmd.toStdString().c_str());

    ui->plainTextEdit->appendPlainText(ip);
    started = true;

}

void DialogLteTest::stop4G() {
    if (!started) {
        return;
    }
    system("/data/bin/start4G x");
    ui->plainTextEdit->appendPlainText(QString("stop 4g\n"));
    started = false;
}

void DialogLteTest::keyPressEvent(QKeyEvent *event) {
    switch (event->key()) {
    case Qt::Key_1:
        start4G();
        break;
    case Qt::Key_2:
        stop4G();
        break;  // not yet implement stop 4g
    case Qt::Key_3:
        ui->plainTextEdit->clear();
        break;
    case Qt::Key_Escape:
        this->accept();
    default:
        break;
    }
}
