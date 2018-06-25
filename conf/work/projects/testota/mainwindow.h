#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "pkghandle.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QString confpath, QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    PkgHandle *handle;
    QString privpath = "/home/sora/gitbase/miscthings/conf/work/projects/dw_inst/localpkgs/privkey";
    QString pubpath = "/home/sora/gitbase/miscthings/conf/work/projects/dw_inst/localpkgs/pubkey";

private slots:
    void handleinstall();
    void handleuninstall();
    void handlegenerate();
    void handlesign(); // verify done during install

private:
    void getListFromFrame(QVector<QString>& installlist, int install);
    void signOnePkg(QString from, QString to);
};

#endif // MAINWINDOW_H
