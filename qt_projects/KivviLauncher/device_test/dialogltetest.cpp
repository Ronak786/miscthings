#include "dialogltetest.h"
#include "ui_dialogltetest.h"
#include <stdio.h>

#include <QStringList>
#include <QDebug>
#include <QProcess>


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
    QProcess start4G;
    QString line;
    if (started) {
        return;
    }
    start4G.start("/data/bin/start4G");
    if (start4G.waitForFinished()) { // wait 30s
        while(start4G.canReadLine()) {
            line = start4G.readLine();
            qDebug() << "get result " << line;
            if (line.indexOf(QString("card not")) != -1) {
                ui->plainTextEdit->appendPlainText("card not registered");
                return;
            } else if (line.startsWith("+CGCONTRDP")) {
                QString ipgw = line.split(",")[3];
                QStringList ipgwlst = ipgw.split(".");
                QStringList iplst(QStringList() << ipgwlst[0] << ipgwlst[1] << ipgwlst[2] << ipgwlst[3]);
                QString ip = iplst.join(".");
                qDebug() << "we get ip " << ip;
                QString cmd = QString("ip route add default via ") + ip + QString(" dev seth_lte0");
                system(cmd.toStdString().c_str());

                ui->plainTextEdit->appendPlainText(ip);
                started = true;
            }

        }
    } else {
        qDebug() << "some start4g error?";
    }
    start4G.terminate();
}

void DialogLteTest::stop4G() {
    if (!started) {
        return;
    }
    qDebug() << "begin stop";
    system("/data/bin/start4G x");
    ui->plainTextEdit->appendPlainText(QString("stop 4g\n"));
    started = false;
}

void DialogLteTest::keyPressEvent(QKeyEvent *event) {
    QProcess ping;
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
    case Qt::Key_4:  // test ping
        static int count = 1;
        qDebug() << "begin test";
        ping.start("ping", QStringList() << "-q" << "-c 1 " << "8.8.8.8");
        if (ping.waitForFinished()) {
            while(ping.canReadLine()) {
                QString line = ping.readLine();
                qDebug() << "get result " << line;
                if (line.startsWith("1 packets transmitted")) {
                    ui->plainTextEdit->appendPlainText(QString::number(count++) + ":" + line);
                }
            }
        } else {
            qDebug() << "some ping error?";
        }
        ping.terminate();
        qDebug() << "end ping";
        break;
    case Qt::Key_Escape:
        this->accept();
    default:
        break;
    }
}
