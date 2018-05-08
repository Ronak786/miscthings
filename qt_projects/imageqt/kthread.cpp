#include "kthread.h"
#include <cstdlib>
#include <cstdio>


KThread::KThread(QObject *parent):
    QThread(parent), quit(false), flag(false), fps(0) {

    if (init_load() != 0) {
        return;
    }
    mutex = new QMutex();
    wait = new QWaitCondition();
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(sendFps()));
    timer->start(1000);
    qDebug("begin thread now\n");
}

KThread::~KThread() {
    mutex->lock();
    quit = true;
    wait->wait(mutex);
    mutex->unlock();
    lclose();
    qlib->unload();
    qDebug("end of thread");
}

int KThread::init_load() {
    qlib = new QLibrary("nkrypto.dll");
    if (qlib->load()) {
        qDebug("load success\n");
        linit = (Init)qlib->resolve("Nkrypto_Init");
        lopen = (Open)qlib->resolve("Nkrypto_Open");
        lclose = (Close)qlib->resolve("Nkrypto_Close");
        lgetImage = (GetImage)qlib->resolve("Nkrypto_GetImageData");
        lgetSerial = (GetSerial)qlib->resolve("Nkrypto_GetSerialNum");
        if (linit != NULL && lopen != NULL
                && lclose != NULL && lgetImage != NULL && lgetSerial != NULL) {
            qDebug("success load func\n");
        } else {
            qDebug("fail load func\n");
            return -1;
        }
    } else {
        qDebug("load fail lib\n");
        return -1;
    }
    return 0;

}

void KThread::run()  {
    int res;
    res = linit();
    if (res != 0) {
        qDebug("error init %d\n", res);
        return;
    }
    res = lopen();
    if (res != 0) {
        qDebug("error open %d\n", res);
        return;
    }
    int needed = 640*480*1;
    unsigned char *data = (unsigned char*)malloc(needed);
    unsigned char serial[9] = {0};
    if (data == NULL) {
        qDebug("wrong alloc mem\n");
        exit(0);
    }
    forever {
        if (quit) {
            qDebug("begin to break\n");
            break;
        }
        if ((res = lgetImage(data, needed)) != needed) {
            qDebug("get image fail %d\n", res);
            continue;
        }
        QImage img = QImage(data, 640, 480, QImage::Format_Indexed8);
        emit tell(img);

        if ((res = lgetSerial(serial, 9)) != 9) {
            qDebug("get serial number fail %d\n", res);
        } else {
            qDebug("get serial num %s\n", serial);
        }
        mutex->lock();
        fps++;
        mutex->unlock();
    }
    wait->wakeAll();
}

// qtimer's timeout connect to  this function
// then this function emit to window and show on lable every 1sec
void KThread::sendFps() {
    emit showFps(fps);
    mutex->lock();
    fps = 0;
    mutex->unlock();
}
