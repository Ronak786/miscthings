#include "kthread.h"

KThread::KThread(QObject *parent):
    QThread(parent), quit(false) {
    mutex = new QMutex();
    wait = new QWaitCondition();
    qWarning("begin thread now\n");
}

KThread::~KThread() {
    mutex->lock();
    quit = true;
    wait->wait(mutex);
    mutex->unlock();
    qWarning("end of thread");
}

void KThread::run()  {
    forever {
        if (quit) {
            qWarning("begin to break\n");
            break;
        }
        msleep(1000);
        mutex->lock();
        qWarning("begin to emit\n");
        mutex->unlock();
        emit tell();

    }
    wait->wakeAll();
}
