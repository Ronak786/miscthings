#ifndef CUSTOMKEY_H
#define CUSTOMKEY_H

#include <QTextEdit>
#include <QKeyEvent>

class CustomKey : public QTextEdit {
    Q_OBJECT

public:
    void keyPressEvent(QKeyEvent* key) override;
};
#endif // CUSTOMKEY_H
