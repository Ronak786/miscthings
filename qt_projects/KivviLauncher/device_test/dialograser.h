#ifndef DIALOGRASER_H
#define DIALOGRASER_H

#include <QWidget>
#include <ui_dialograser.h>
#include <QSerialPort>

namespace Ui {
class DialogRaser;
}

class DialogRaser : public QDialog
{
public:
    DialogRaser(QWidget *parent = NULL);
    ~DialogRaser();
    void startRaser();
    void stopRaser();

private:
    Ui::DialogRaser *ui;
    QSerialPort *m_SerialPort;
    bool _opened;
    bool _started;

};

#endif // DIALOGRASER_H
