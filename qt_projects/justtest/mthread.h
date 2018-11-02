#ifndef MTHREAD_H
#define MTHREAD_H

#include <QThread>
#include <QImage>


class mthread : public QThread {
    Q_OBJECT
protected:
    virtual void run() override;

signals:
    void sendImg(QImage img);
};
#endif // MTHREAD_H
