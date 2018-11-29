#include "mainwindow.h"
#include <QApplication>
#include <QDesktopWidget>

int main(int argc, char *argv[])
{
    //Set env for embedded platform
    qputenv("QT_QPA_EVDEV_KEYBOARD_PARAMETERS", "/dev/event100");
    //qputenv("QT_QPA_PLATFORM", "linuxfb:size=800x480;mmsize=162x121");
    //qputenv("QT_QPA_PLATFORM", "linuxfb:size=800x480;mmsize=800x480");
    qputenv("QT_QPA_PLATFORM", "linuxfb:size=240x320;mmsize=240x320");
    system("ulimit -n 65536"); // set open file limit

    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    //w.move((QApplication::desktop()->width() - w.width()) / 2, (QApplication::desktop()->height() - w.height()) / 2);

    QFont font;
    //font.setPointSize(10);    //PC Test Font
    font.setPointSize(50);      //Dev Board Font
    font.setFamily(("simfang"));
    font.setBold(false);

    a.setFont(font);

    //Disable cursor
    a.setOverrideCursor(Qt::BlankCursor);

    return a.exec();
}
