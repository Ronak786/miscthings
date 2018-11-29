#ifndef DIALOGGPSTEST_H
#define DIALOGGPSTEST_H

#include <QDialog>
#include <QTimer>
#include <QtSerialPort/QSerialPort>
#include <QString>
#include <QMutex>
#include <QTextStream>

namespace Ui {
class DialogGpsTest;
}

class DialogGpsTest : public QDialog
{
    Q_OBJECT

public:
    explicit DialogGpsTest(QWidget *parent = 0);
    ~DialogGpsTest();
    void getGpsFixData();
    void getRMC();

    QString gpsTime(QString str);
    QString gpsDate(QString str);

private slots:
    void getGpsInfo();
    void readGpsData();

private:
    Ui::DialogGpsTest *ui;
    QTimer      *m_Timer;
    QSerialPort *m_SerialPort;
    QString     m_GpsData;
    QMutex *mutex;
};

#endif // DIALOGGPSTEST_H
