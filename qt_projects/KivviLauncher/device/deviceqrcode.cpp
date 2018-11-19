#include "deviceqrcode.h"
#include "third_part/include/qrencode.h"
#include <QPixmap>
#include <QImage>
#include <QPainter>

DeviceQRCode::DeviceQRCode(QObject *parent) : QObject(parent)
{

}

DeviceQRCode::~DeviceQRCode()
{

}

QPixmap DeviceQRCode::generateQRcode(QString str, int width, int height)
{
    QRcode *qrcode = QRcode_encodeString(str.toStdString().c_str(), 2, QR_ECLEVEL_Q, QR_MODE_8, 1);

    qint32 qrcodeWidth = qrcode->width > 0 ? qrcode->width : 1;
    double scale_x = (double)width / (double)qrcodeWidth;
    double scale_y = (double)height / (double)qrcodeWidth;

    QImage image = QImage(width, height, QImage::Format_ARGB32);
    QPainter painter(&image);
    QColor background(Qt::white);
    painter.setBrush(background);
    painter.setPen(Qt::NoPen);
    painter.drawRect(0, 0, width, height);
    QColor foreground(Qt::black);
    painter.setBrush(foreground);

    for (int x = 0; x < width; x++)
    {
        for (int y = 0; y < height; y++)
        {
            unsigned char b = qrcode->data[x * qrcodeWidth + y];
            if (b & 0x01)
            {
                QRectF r(y * scale_x, x * scale_y, scale_x, scale_y);
                painter.drawRects(&r, 1);
            }
        }
    }

    QPixmap pixmap = QPixmap::fromImage(image);

    QRcode_free(qrcode);

    return pixmap;
}
