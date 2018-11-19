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

namespace Ui {
class DialogCameraTest;
}

class DialogCameraTest : public QDialog
{
    Q_OBJECT

public:
    explicit DialogCameraTest(QWidget *parent = 0);
    ~DialogCameraTest();
    void keyPressEvent(QKeyEvent *event) override;

private slots:


private:
    Ui::DialogCameraTest *ui;
    DialogRaser *dialograser;
    DialogCamera *dialogcamera;

};

#endif // DIALOGCAMERATEST_H
