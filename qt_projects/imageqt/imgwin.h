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

namespace Ui {
class imgWin;
}

class imgWin : public QMainWindow
{
    Q_OBJECT

public:
    explicit imgWin(QWidget *parent = 0);
    ~imgWin();

public slots:
    void info();
private:
    Ui::imgWin *ui;
    QTimer * timer;
};

#endif // IMGWIN_H
