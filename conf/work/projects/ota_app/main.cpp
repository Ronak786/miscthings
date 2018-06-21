#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    if (argc != 2) {
        qDebug("need config file as parameter");
        return -1;
    }
    QApplication a(argc, argv);
    MainWindow w(argv[1]);
    w.show();

    return a.exec();
}
