#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QImage>
#include <mthread.h>
#include <worker.h>
#include <QTimer>
#include <QKeyEvent>
#include "ui_mainwindow.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(int gap, int usleep, int pheight, int flip, QWidget *parent = 0);
    ~MainWindow();
    bool test_decode(char *dataptr, int cols, int rows);
protected:
    void keyPressEvent(QKeyEvent* event) override;


private:
    Ui::MainWindow *ui;
    mthread *runner;
    QThread *qthread;
    Worker *mworker;
    QTimer *timer;

    QByteArray mImgData;
    int mImgWidth;
    int mImgHeight;
    int _gap;
    int _usleep;
    bool _flip;

public slots:
    void recvImg(QImage);
    void recvImgOne(QImage);
    void recvImgData(QByteArray data, int width, int height);
    void shout();
    void saveImg();
//    void change(int);
};

#endif // MAINWINDOW_H
