#ifndef MTHREAD_H
#define MTHREAD_H
#include <QThread>
#include <QTcpSocket>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class mthread : public QObject{
    Q_OBJECT

public:
    mthread(QString str = "");
    ~mthread();

public slots:
    void process();
    void showData(QNetworkReply*);
    void showDataTarget(QNetworkReply*);
    void processTarget();

signals:
    void finished();
    void error(QString err);
    void returnData(QString str);
    void returnDataTarget(QString str);

private:
    QString posAddr;

};
#endif // MTHREAD_H
