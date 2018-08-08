#include "mainwindow.h"
#include <QApplication>
#include <QUrl>

int main(int argc, char *argv[])
{
 //   qputenv("QT_QPA_EVDEV_KEYBOARD_PARAMETERS", "/dev/event100");
 //   qputenv("QT_QPA_PLATFORM", "linuxfb:size=800x600;mmsize=162x121");
//    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
//    qputenv("QT_SCALE_FACTOR", "4");

    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}

