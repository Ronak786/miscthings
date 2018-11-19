#ifndef DIALOGFACTORYRESET_H
#define DIALOGFACTORYRESET_H

#include <QDialog>

namespace Ui {
class DialogFactoryReset;
}

class DialogFactoryReset : public QDialog
{
    Q_OBJECT

public:
    explicit DialogFactoryReset(QWidget *parent = 0);
    ~DialogFactoryReset();

private:
    Ui::DialogFactoryReset *ui;
};

#endif // DIALOGFACTORYRESET_H
