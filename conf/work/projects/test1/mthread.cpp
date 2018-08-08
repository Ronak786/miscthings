#include "mthread.h"
#include <QNetworkRequest>
#include <QSslSocket>
#include <QCoreApplication>
#include <QNetworkInterface>
#include <QtMath>



mthread::mthread(QString str) : posAddr(str) {

}

mthread::~mthread() {

}


void mthread::process() {

    posAddr = QString(); // set to empty

    QUrl url = QUrl::fromUserInput(QString("http://192.168.0.11:8092"));
    QNetworkAccessManager *man = new QNetworkAccessManager();
    QNetworkRequest req(url);

    connect(man, SIGNAL(finished(QNetworkReply*)), this, SLOT(showData(QNetworkReply*)));

    foreach(const QNetworkInterface &netInterface, QNetworkInterface::allInterfaces()) {
        QNetworkInterface::InterfaceFlags flags = netInterface.flags();
        if ( (bool)(flags & QNetworkInterface::IsRunning) && !(bool)(flags & QNetworkInterface::IsLoopBack)) {
            foreach(const QNetworkAddressEntry &address, netInterface.addressEntries()) {
                if (address.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                    qDebug() << address.ip().toString() << ":" << address.prefixLength();
                    QString ipbase = address.ip().toString().mid(0, address.ip().toString().lastIndexOf(".") + 1);
                    int range = qPow(2, 32 - address.prefixLength());
                    for (int i = 2; i < range -1; ++i) {
                        qDebug() << ipbase + QString::number(i) + ":8092";
                        url = QUrl::fromUserInput("http://" + ipbase + QString::number(i) + ":8092");
                        req.setUrl(url);
                        man->get(req);

                    }
                }
            }
        }
    }

}

void mthread::processTarget() {
    qDebug() << "in processtarget  target address is " << posAddr;
    QNetworkAccessManager *man = new QNetworkAccessManager();
    QUrl url = QUrl::fromUserInput(posAddr);
    QNetworkRequest req(url);
    connect(man, SIGNAL(finished(QNetworkReply*)), this, SLOT(showDataTarget(QNetworkReply*)));
    while(true) {
        man->get(req);

        QThread::msleep(2000);
        QCoreApplication::processEvents();
    }
    emit finished();
}

void mthread::showData(QNetworkReply *reply) {
    QString result(reply->readAll());
    if (result.startsWith("{\"longitude")) {
        posAddr = reply->url().toString();
        emit returnData(reply->url().toString());
    }
}

void mthread::showDataTarget(QNetworkReply *reply) {
    qDebug() << "get Data" << reply->readAll();
    QString result(reply->readAll());
    if (result.startsWith("{\"longitude")) {
        emit returnDataTarget(result);
    }
}

