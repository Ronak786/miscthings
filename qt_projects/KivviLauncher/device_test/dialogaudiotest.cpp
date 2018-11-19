#include "dialogaudiotest.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <QDebug>

DialogAudioTest::DialogAudioTest() :
    _audioNum(0), _volume(12), _volumeMax(15), _volumeMin(0)
{

}

DialogAudioTest::~DialogAudioTest() {

}


void DialogAudioTest::keyPressEvent(QKeyEvent *event) {
    switch(event->key()) {
    case Qt::Key_1:
        system("/data/bin/pwm 1000 31");
        break;
    case Qt::Key_2:
        doAudioTest();
    case Qt::Key_3:
        setAudio(-1);
        break;
    case Qt::Key_5:
        setAudio(1);
        break;
    case Qt::Key_Escape:
        this->accept();
    default:
        break;
    }
}

void DialogAudioTest::doAudioTest() {
    QVector<QString> vstr = {" \"SPKL Mixer DACLSPKL Switch\" 1",
                             " \"SPKL Mixer DACRSPKL Switch\" 1",
                             " \"VBC DACL DG Switch\" 1",
                             " \"VBC DACR DG Switch\" 1",
                             " \"Speaker Function\" 1",
                             " \"SPKL Playback Volume\" " + QString::number(_volume),
                             " \"SPKR Playback Volume\" " + QString::number(_volume),
                             " \"Inter PA Config\" 792"
                         };
    int count;
    char buf[128];
    if ((count = readlink("/proc/asound/sprdphone", buf, sizeof(buf))) <= 0) {
        qDebug() << "can not read link of sprdphone, failed play audio";
        return;
    }
    _audioNum = atoi(&buf[count-1]);
    qDebug() << "audio device number is " << _audioNum;
    foreach (QString str, vstr) {
        sendCmd(QString("tinymix -D " + QString::number(_audioNum) + str));
    }
    sendCmd(QString("tinyplay /data/test/test.wav -D " + QString::number(_audioNum)));
    return;
}

void DialogAudioTest::sendCmd(QString cmd) {
    system(cmd.toStdString().c_str());
}


void DialogAudioTest::setAudio(int dir) {
    if (dir == -1 && _volume > _volumeMin) {
        _volume--;
    } else if (dir == 1 && _volume < _volumeMax) {
        _volume++;
    }
    sendCmd(QString("tinymix -D ") + QString::number(_audioNum) +
                    " \"SPKL Playback Volume\" " + QString::number(_volume));
    sendCmd(QString("tinymix -D ") + QString::number(_audioNum) +
                    " \"SPKR Playback Volume\" " + QString::number(_volume));

}
