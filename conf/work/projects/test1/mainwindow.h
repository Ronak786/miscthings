#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <mthread.h>
#include <QKeyEvent>
#include <QTcpSocket>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QMutex>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    QThread *mtDetect;
    QThread *mtLoop;
    QString flagData;
    QMutex mutex;
    mthread *workerDetect;
    mthread *workerLoop;

private slots:
    void mainshow(QString str);
    void mainshowTarget(QString str);


};

#endif // MAINWINDOW_H
