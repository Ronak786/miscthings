#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "debug.h"
#include "sigutil.h"
#include <fcntl.h>
#include <unistd.h>
#include <QString>
#include <QByteArray>

MainWindow::MainWindow(QString confpath, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    handle = new PkgHandle(confpath);
    handle->init();
    connect(ui->install, SIGNAL(clicked(bool)), this, SLOT(handleinstall()));
    connect(ui->uninstall, SIGNAL(clicked(bool)), this, SLOT(handleuninstall()));
    connect(ui->generate, SIGNAL(clicked(bool)), this, SLOT(handlegenerate()));
    connect(ui->sign, SIGNAL(clicked(bool)), this, SLOT(handlesign()));
}

MainWindow::~MainWindow()
{
    delete ui;
    handle->uninit();
}

void MainWindow::handleinstall() {
    pr_info("begin handle install");
    QVector<QString> installlist;
    installlist.push_back(QString("one-1.2.tgz"));
//    installlist.push_back(QString("two-1.1.tgz"));
    handle->install(installlist, pubpath);
}

void MainWindow::handleuninstall() {
    pr_info("begin handle uninstall");
    QVector<QString> uninstalllist;
    uninstalllist.push_back("one");
//    uninstalllist.push_back("two");
    handle->uninstall(uninstalllist);
}

void MainWindow::handlegenerate() {
    pr_info("begin generate self keys");
    SigUtil::generate_new_keypairs(privpath, pubpath);
}

void MainWindow::handlesign() {
    pr_info("begin sign\n");
    QByteArray sigarray;
    unsigned char tmpbuf[4096];
    unsigned int tmplen;
    int ret = SigUtil::sign(QString("/home/sora/gitbase/miscthings/conf/work/projects/dw_inst/localpkgs/one-1.2.tar.gz"),
                 sigarray, privpath, pubpath);
    if (ret != 0) {
        pr_info("sign failed\n");
        return;
    }

    pr_info("save sig to file");
    int fdin, fdout;
    fdin = open("/home/sora/gitbase/miscthings/conf/work/projects/dw_inst/localpkgs/one-1.2.tar.gz", O_RDONLY);
    if (fdin == -1) {
        pr_info("can not open input file for read\n");
        return;
    }
    fdout = open("/home/sora/gitbase/miscthings/conf/work/projects/dw_inst/localpkgs/one-1.2.tgz", O_WRONLY|O_CREAT|O_TRUNC, 0600);

    // write original file
    while ((tmplen = read(fdin, tmpbuf, sizeof(tmpbuf))) > 0) {
        if (write(fdout, tmpbuf, tmplen) != tmplen) {
            pr_info("write tgz file failed\n");
            return;
        }
    }

    // write sig data
    if (write(fdout, sigarray.data(), (unsigned int)sigarray.size()) != (unsigned int)sigarray.size()) {
        pr_info("write sig failed\n");
        return;
    }

    // write sig length as last byte
    char ch = sigarray.size(); // siglength is about (70,72)
    if (write(fdout, &ch, 1) != 1) {
        pr_info("can not append length into file\n");
        return;
    }
    pr_info("write in sig len is %d\n", ch);
}


