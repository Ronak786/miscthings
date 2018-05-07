#ifndef KTHREAD_H
#define KTHREAD_H
#include <QThread>
#include <QMutex>
#include <QWaitCondition>

class KThread : public QThread {
    Q_OBJECT
private:
    bool quit;
    QMutex *mutex;
    QWaitCondition * wait;
protected:
    void run() override;
signals:
    void tell();
public:
    explicit KThread(QObject *parent = Q_NULLPTR);
    ~KThread();

};

#endif // THREAD_H
