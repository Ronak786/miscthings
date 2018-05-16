#ifndef KTHREAD_H
#define KTHREAD_H
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QImageReader>
#include <QImage>
#include <QLibrary>
#include <QTimer>


typedef int (*Init)();
typedef int (*Open)();
typedef int (*Close)();
typedef int (*GetImage)(unsigned char *buf, int size);
typedef int (*GetSerial)(unsigned char *buf, int size);

class KThread : public QThread {
    Q_OBJECT
private:
    bool quit;
    QMutex *mutex;
    QWaitCondition * wait;
    QImage img1, img2;
    bool flag;
    QLibrary *qlib;
    Init linit;
    Open lopen;
    Close lclose;
    GetImage lgetImage;
    GetSerial lgetSerial;
    QTimer *timer;
    int fps;
protected:
    void run() override;
signals:
    void tell(QImage img);
    void showFps(int fps);
public:
    explicit KThread(QObject *parent = Q_NULLPTR);
    ~KThread();
    int init_load();
private slots:
    void sendFps();

};

#endif // THREAD_H
