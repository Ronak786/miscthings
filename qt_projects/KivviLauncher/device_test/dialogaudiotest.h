#ifndef DIALOGAUDIOTEST_H
#define DIALOGAUDIOTEST_H

#include <QDialog>
#include <QKeyEvent>
#include <QString>

namespace Ui {
class DialogAudioTest;
}

class DialogAudioTest : public QDialog
{
    Q_OBJECT
public:
    DialogAudioTest();
    ~DialogAudioTest();
    void keyPressEvent(QKeyEvent *event) override;

private:
    void doAudioTest();
    void sendCmd(QString cmd);
    void setAudio(int direction);

private:
    Ui::DialogAudioTest *ui;
    int _audioNum;
    int _volume;
    int _volumeMax;
    int _volumeMin;
};

#endif // DIALOGAUDIOTEST_H
