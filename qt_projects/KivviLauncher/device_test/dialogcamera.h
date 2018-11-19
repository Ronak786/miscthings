#ifndef DIALOGCAMERA_H
#define DIALOGCAMERA_H

#include <QWidget>
#include <QDialog>
#include <QThread>
#include <QTimer>
#include <QByteArray>
#include <ui_dialogcamera.h>

#include "worker.h"

namespace Ui {
class DialogCamera;
}

class DialogCamera : public QDialog
{
    Q_OBJECT
public:
    DialogCamera(int gap, int usleep, int pheight, int flip, QWidget *parent = 0);
    ~DialogCamera();
    bool test_decode(char *dataptr, int cols, int rows);

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    Ui::DialogCamera *ui;
    QThread *qthread;
    Worker *mworker;
    QTimer *timer;

    QByteArray mImgData;
    int mImgWidth;
    int mImgHeight;
    int _gap;
    int _usleep;
    bool _flip;

signals:
    void setquit();

public slots:
    void recvImgData(QByteArray data, int width, int height);
    void shout();
    void saveImg();
};

#endif // DIALOGCAMERA_H
