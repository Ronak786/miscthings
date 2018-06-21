#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "debug.h"
#include "sigutil.h"
#include <fcntl.h>
#include <unistd.h>

MainWindow::MainWindow(QString confpath, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    handle = new PkgHandle();
    if (handle->loadConfig(confpath.toStdString()) != 0) {
        qDebug("can not load config");
    }
    handle->init();
    connect(ui->install, SIGNAL(clicked(bool)), this, SLOT(handleinstall()));
    connect(ui->uninstall, SIGNAL(clicked(bool)), this, SLOT(handleuninstall()));
    connect(ui->generate, SIGNAL(clicked(bool)), this, SLOT(handlegenerate()));
    connect(ui->sign, SIGNAL(clicked(bool)), this, SLOT(handlesign()));
    connect(ui->verify, SIGNAL(clicked(bool)), this, SLOT(handleverify()));
}

MainWindow::~MainWindow()
{
    delete ui;
    handle->uninit();
}

void MainWindow::handleinstall() {
    pr_info("begin handle install");
    std::vector<std::string> installlist;
    installlist.push_back("one-1.2.tgz");
    installlist.push_back("two-1.1.tgz");
    handle->install(installlist);
}

void MainWindow::handleuninstall() {
    pr_info("begin handle uninstall");
    std::vector<std::string> uninstalllist;
    uninstalllist.push_back("one");
    uninstalllist.push_back("two");
    handle->uninstall(uninstalllist);
}

void MainWindow::handlegenerate() {
    pr_info("begin generate self keys");
    SigUtil::generate_new_keypairs(privpath.c_str(), pubpath.c_str());
}

void MainWindow::handlesign() {
    pr_info("begin sign\n");
    unsigned char buf[128];
    unsigned int siglen;
    unsigned char tmpbuf[4096];
    unsigned int tmplen;
    int ret = SigUtil::sign("/home/sora/gitbase/miscthings/conf/work/projects/dw_inst/localpkgs/one-1.2.tgz",
                 buf, &siglen, privpath.c_str(), pubpath.c_str());
    if (ret != 0) {
        pr_info("sign failed\n");
        return;
    }

    pr_info("save sig to file");
    int fdin, fdout;
    fdin = open("/home/sora/gitbase/miscthings/conf/work/projects/dw_inst/localpkgs/one-1.2.tgz", O_RDONLY);
    if (fdin == -1) {
        pr_info("can not open input file for read\n");
        return;
    }
    fdout = open("/home/sora/gitbase/miscthings/conf/work/projects/dw_inst/localpkgs/sigfile", O_WRONLY|O_CREAT|O_TRUNC, 0600);

    // write original file
    while ((tmplen = read(fdin, tmpbuf, sizeof(tmpbuf))) > 0) {
        if (write(fdout, tmpbuf, tmplen) != tmplen) {
            pr_info("write tgz file failed\n");
            return;
        }
    }

    // write sig data
    if (write(fdout, buf, (unsigned int)siglen) != (unsigned int)siglen) {
        pr_info("write sig failed\n");
        return;
    }

    // write sig length as last byte
    char ch = siglen; // siglength is about (70,72)
    if (write(fdout, &ch, 1) != 1) {
        pr_info("can not append length into file\n");
        return;
    }
    pr_info("write in sig len is %d\n", siglen);
}

void MainWindow::handleverify() {
    pr_info("begin verify\n");

    bool ret = SigUtil::verify("/home/sora/gitbase/miscthings/conf/work/projects/dw_inst/localpkgs/sigfile",
                   pubpath.c_str());
    pr_info("verify result is %d", ret);
}

