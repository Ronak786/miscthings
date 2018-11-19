#ifndef WORKER_H
#define WORKER_H

#include <QQueue>
#include <QImage>
#include <QByteArray>
#include <QDebug>
#include <QThread>


//class Worker : public QObject {
class Worker : public QThread {

    Q_OBJECT
public:
    Worker(int usleep = 0, int pheight = 0);
protected:
    void run() override;

public slots:
//    void run();
    void setquit();

signals:
    void sendImg(QImage);
    void sendImgOne(QImage);
    void sendImgData(QByteArray data, int width, int height);
    void startloop();
    void finished();

private:
#ifdef __linux__
    void runarm();
#elif _WIN32
    void runpc();
#endif
    inline bool blkscr(const char* data, int len) {
        for(int i = 0; i < len; i+=2) {
            if (data[i] > 0x0b) {
                qDebug() << "not black screen, start";
                return false;
            }
        }
        return true;
    }



private:
    QVector<QByteArray>  qb;
    QByteArray barr;
    int _usleep;
    int _pheight;

public:
    bool _quitflag;
};

#endif // WORKER_H
