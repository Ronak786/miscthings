#include "devicemobilesignal.h"
#include "iputil.h"
#include <QDebug>

DeviceMobileSignal::DeviceMobileSignal(QObject *parent) : QObject(parent)
{
    //lte_open_modem();
}

DeviceMobileSignal::~DeviceMobileSignal()
{
    //lte_close_modem();
}

QPixmap DeviceMobileSignal::getDeviceMobileSignalLevel()
{
    QString strFilePath;
    int level = 4;

    //int level = lte_get_siglevel();
    //qDebug() << "" + QString::number(level, 10);

    switch (level) {
    case 1:
        strFilePath = "://resource/signal1.png";
        break;
    case 2:
        strFilePath = "://resource/signal2.png";
        break;
    case 3:
        strFilePath = "://resource/signal3.png";
        break;
    case 4:
        strFilePath = "://resource/signal4.png";
        break;
    case 5:
        strFilePath = "://resource/signal.png";
        break;
    default:
        break;
    }

    QPixmap *pixmap = new QPixmap();
    pixmap->load(strFilePath, 0, Qt::AvoidDither | Qt::ThresholdDither | Qt::ThresholdAlphaDither);

    return *pixmap;
}
