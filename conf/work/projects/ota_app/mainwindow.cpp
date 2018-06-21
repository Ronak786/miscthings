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
    connect(ui->pushButton, SIGNAL(clicked(bool)), this, SLOT(handleinstall()));
    connect(ui->pushButton_2, SIGNAL(clicked(bool)), this, SLOT(handleuninstall()));
    connect(ui->pushButton_3, SIGNAL(clicked(bool)), this, SLOT(handlegenerate()));
    connect(ui->pushButton_4, SIGNAL(clicked(bool)), this, SLOT(handlesign()));
    connect(ui->pushButton_5, SIGNAL(clicked(bool)), this, SLOT(handleverify()));
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
    int ret = SigUtil::sign("/home/sora/gitbase/miscthings/conf/work/projects/dw_inst/localpkgs/one-1.2.tgz",
                 buf, &siglen, privpath.c_str(), pubpath.c_str());
    if (ret != 0) {
        pr_info("sign failed\n");
        return;
    }

    pr_info("save sig to file");
    int fd = open("/home/sora/gitbase/miscthings/conf/work/projects/dw_inst/localpkgs/sigfile", O_WRONLY|O_CREAT);
    if (write(fd, buf, (unsigned int)siglen) != (unsigned int)siglen) {
        pr_info("write sig failed\n");
        return;
    }
}

void MainWindow::handleverify() {
    pr_info("begin verify\n");
    unsigned char buf[128];
    int siglen;
    int fd = open("/home/sora/gitbase/miscthings/conf/work/projects/dw_inst/localpkgs/sigfile", O_RDONLY);
    if ((siglen = read(fd, buf, 128)) == -1) {
        pr_info("read sig failed\n");
        return;
    }
    bool ret = SigUtil::verify("/home/sora/gitbase/miscthings/conf/work/projects/dw_inst/localpkgs/one-1.2.tgz",
                   buf, (unsigned int)siglen, pubpath.c_str());
    pr_info("verify result is %d", ret);
}

