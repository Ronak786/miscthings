#include "customkey.h"

void CustomKey::keyPressEvent(QKeyEvent* key) {
    qDebug("key pressed 0x%8x\n", key->key());

    int tmpkey = key->key();
    if (tmpkey != Qt::Key_A) {
        QTextEdit::keyPressEvent(key);
    }

}
