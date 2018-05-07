#include "imgwin.h"
#include <QApplication>
//#include "nkrypto.h"
#include <QLibrary>

typedef int (*Init)();
typedef int (*Open)();
typedef int (*Close)();
typedef int (*GetImage)(unsigned char *buf, int size);
typedef int (*GetSerial)(unsigned char *buf, int size);

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QLibrary qlib("../imageqt/nkrypto.dll");
    imgWin w;
    if (qlib.load()) {
        printf("load success\n");
        Init linit = (Init)qlib.resolve("Nkrypto_Init");
        Open lopen = (Open)qlib.resolve("Nkrypto_Open");
        Close lclose = (Close)qlib.resolve("Nkrypto_Close");
        GetImage lgetImage = (GetImage)qlib.resolve("Nkrypto_GetImageData");
        GetSerial lgetSerial = (GetSerial)qlib.resolve("Nkrypto_GetSerialNum");
        if (linit != NULL && lopen != NULL
                && lclose != NULL && lgetImage != NULL && lgetSerial != NULL) {
            printf("success load func\n");
        } else {
            printf("fail load func\n");
        }
    } else {
        printf("load fail\n");
    }
    qlib.unload();
 //   int res = Nkrypto_Init();
  //  printf("res is %d\n", res);
    w.show();

    return a.exec();
}
