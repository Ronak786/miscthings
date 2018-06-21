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
    std::string privpath = "/home/sora/gitbase/miscthings/conf/work/projects/dw_inst/localpkgs/privkey";
    std::string pubpath = "/home/sora/gitbase/miscthings/conf/work/projects/dw_inst/localpkgs/pubkey";

private slots:
    void handleinstall();
    void handleuninstall();
    void handlegenerate();
    void handlesign();
    void handleverify();
};

#endif // MAINWINDOW_H
