#include "mainwindow.h"
#include <QApplication>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    if (argc !=5 ) {
        qDebug() << "need gap sleep_time height flip";
        return -1;
    }
    QApplication a(argc, argv);
    MainWindow w(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]), atoi(argv[4]));
    w.show();

    return a.exec();
}
