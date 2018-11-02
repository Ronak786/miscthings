#ifndef WORKER_H
#define WORKER_H

#include <QQueue>
#include <QImage>
#include <QByteArray>
#include <QDebug>


class Worker : public QObject {
    Q_OBJECT
public:
    Worker(int usleep = 0, int pheight = 0);

public slots:
    void run();

signals:
    void sendImg(QImage);
    void sendImgOne(QImage);
    void sendImgData(QByteArray data, int width, int height);
    void startloop();

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
};

#endif // WORKER_H
