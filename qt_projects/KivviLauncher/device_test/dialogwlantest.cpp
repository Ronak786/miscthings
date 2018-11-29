#include "dialogwlantest.h"
#include "ui_dialogwlantest.h"
#include <stdio.h>
#include <QThread>
#include <QDebug>
#include <QNetworkConfigurationManager>
#include <QProcess>
#include <QFile>
#include <QDir>



DialogWlanTest::DialogWlanTest(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogWlanTest), _mlist(new MyListWidget)
{
    ui->setupUi(this);
    ui->verticalLayout->addWidget(_mlist);
    _showedit = new QPlainTextEdit;
    _showedit->setReadOnly(true);
    ui->verticalLayout->addWidget(_showedit);
    this->setWindowFlags(Qt::FramelessWindowHint);


    if (!QFile::exists("/tmp/.kivvi_network_added")) {
        system(ifup);
        system(addnework);
        system("touch /tmp/.kivvi_network_added");
    }
    connect(_mlist, SIGNAL(itemActivated(QListWidgetItem*)), this, SLOT(handlePress(QListWidgetItem*)));
    _mlist->setFocus();
    qDebug() << "after constructor";
}

DialogWlanTest::~DialogWlanTest()
{
    delete ui;
}

void DialogWlanTest::keyPressEvent(QKeyEvent *event) {
    QProcess ping;
    FILE *fp;

    switch(event->key()) {
    case Qt::Key_1:
        _mlist->clear();
        system(scan);
        QThread::msleep(5000);
        fp = popen(scanresult, "r");
qDebug() << "begin read result";
        char tmpline[256];
        while (fgets(tmpline, sizeof(tmpline), fp)) {
            qDebug () << "read: " << tmpline;
            QString tmpstr(tmpline);
            QString tmpssid = tmpstr.split(QRegExp("\\s+"))[4];
            QListWidgetItem *newItem = new QListWidgetItem;
            newItem->setText(tmpssid);
            _mlist->addItem(newItem);


            // add number into set
        }
        pclose(fp);
        qDebug() << "after read result";
        break;
    case Qt::Key_2:
        system(disablenetwork);
        break;
    case Qt::Key_3:
        static int count = 1;
        qDebug() << "begin test";
        ping.start("ping", QStringList() << "-q" << "-c 1 " << "8.8.8.8");
        if (ping.waitForFinished(250)) {
            while(ping.canReadLine()) {

                QString line = ping.readLine();
                qDebug() << "get result " << line;
                if (line.startsWith("1 packets transmitted")) {
                    _showedit->appendPlainText(QString::number(count++) + ":" + line);
                }
            }
        } else {
            qDebug() << "some ping error?";
        }
        ping.terminate();
        break;
    default:
        QDialog::keyPressEvent(event);
        break;
    // if keycode is one of key in hashmap, select that network
    // show a dialog for
    }
}

void DialogWlanTest::setupnetwork(QString _ssid, QString _pass) {
    char tmpbuf[256];
    strcpy(tmpbuf, setnetwork);
    sprintf(tmpbuf, setnetwork, _ssid.toStdString().c_str(), _pass.toStdString().c_str());
    system(tmpbuf);
    system(dhcpcd);
    qDebug() << "success enable network, now can ping ip";
}

void DialogWlanTest::handlePress(QListWidgetItem *item) {
    PassDialog *dialog;
    dialog = new PassDialog(item->text(), this);
    dialog->exec();
    dialog->getresult(&pass);
    dialog->deleteLater();
    qDebug() << "args " << item->text() << pass;
    setupnetwork(item->text(), pass);

}

