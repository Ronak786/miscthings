#ifndef DIALOGWLANTEST_H
#define DIALOGWLANTEST_H

#include <QDialog>
#include <QString>
#include <QStringList>

namespace Ui {
class DialogWlanTest;
}

class DialogWlanTest : public QDialog
{
    Q_OBJECT

public:
    explicit DialogWlanTest(QWidget *parent = 0);
    ~DialogWlanTest();

private:
    Ui::DialogWlanTest *ui;
    QStringList m_SSID;
    QString     m_Password;
    int         m_SsidIndex;
};

#endif // DIALOGWLANTEST_H
