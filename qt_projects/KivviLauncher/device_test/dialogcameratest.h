#ifndef DIALOGCAMERATEST_H
#define DIALOGCAMERATEST_H

#include <QDialog>
#include <QImage>
#include <QtMultimedia/QCamera>
#include <QtMultimediaWidgets/QCameraViewfinder>
#include <QtMultimedia/QCameraImageCapture>
#include <QKeyEvent>
#include "device_test/dialograser.h"
#include "device_test/dialogcamera.h"
#include <QTimer>

namespace Ui {
class DialogCameraTest;
}

class DialogCameraTest : public QDialog
{
    Q_OBJECT

public:
    explicit DialogCameraTest(QWidget *parent = 0);
    ~DialogCameraTest();
//    void keyPressEvent(QKeyEvent *event) override;


private slots:
    void needquit();


private:
    Ui::DialogCameraTest *ui;
    DialogRaser *dialograser;
    DialogCamera *dialogcamera;
    QTimer *timer;

};

#endif // DIALOGCAMERATEST_H
