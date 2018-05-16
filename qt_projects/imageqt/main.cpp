#include "imgwin.h"
#include <QApplication>
//#include "nkrypto.h"
#include <QLibrary>



int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    printf("just start\n");
    imgWin w;

    w.show();

    return a.exec();
}
