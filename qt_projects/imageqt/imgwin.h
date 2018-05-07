#ifndef IMGWIN_H
#define IMGWIN_H

#include <QMainWindow>
#include <QImageReader>
#include <QImage>
#include <QPixmap>
#include <QVBoxLayout>
#include <QLabel>
#include <QWidget>
#include <QStatusBar>
#include <QTimer>
#include "kthread.h"

namespace Ui {
class imgWin;
}

class imgWin : public QMainWindow
{
    Q_OBJECT

public:
    explicit imgWin(QWidget *parent = 0);
    ~imgWin();
signals:
    void shut();
public slots:
    void info();
    void response();
private:
    Ui::imgWin *ui;
    QTimer * timer;
    KThread * thread;
};

#endif // IMGWIN_H
