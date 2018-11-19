#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include "device/devicemobilesignal.h"
#include "device/devicesystemtime.h"
#include "device/devicebattery.h"

#include "device/devicesysteminfo.h"
#include "widget/widgetstatusbar.h"
#include "device_test/dialogdevicetest.h"
#include "system_setting/dialogsystemsetting.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void keyPressEvent(QKeyEvent *event);

private slots:
    void updateSignalLevel();
    void updateSystemDatetime();
    void updateBetteryLevel();

private:
    Ui::MainWindow *ui;

    QTimer                  *m_Timer;

    DeviceMobileSignal      *m_DeviceMobileSignal;
    DeviceSystemTime        *m_DeviceSystemTime;
    DeviceBattery           *m_DeviceBattery;

    DeviceSystemInfo        *m_DeviceSystemInfo;
    WidgetStatusBar         *m_WidgetStatusBar;

    DialogDeviceTest        *m_DialogDeviceTest;
    DialogSystemSetting     *m_DialogSystemSetting;
};

#endif // MAINWINDOW_H
