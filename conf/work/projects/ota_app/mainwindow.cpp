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

/*
 * install flag: 1 for install, 0 for unisntall, 2 for sign;
 */
void MainWindow::getListFromFrame(QVector<QString>& installlist, int install) {
    QByteArray readarray = ui->textframe->toPlainText().toUtf8();
    QTextStream stream(&readarray);
    QRegExp reg("\\s+");
    while (!stream.atEnd()) {
        QString line = stream.readLine();
        QString name = line.section(reg, 0, 0);
        QString version = line.section(reg, 1, 1);
        if (install == 1) {
            // combine into one-1.2.tgz
            installlist.push_back(name + "-" + version + ".tgz");
        } else if (install == 0){
            installlist.push_back(name);
        } else if (install == 2) {
            installlist.push_back(name + "-" + version);
        }
    }
    pr_info("installllist of %d:", install);
    foreach(QString str, installlist) {
        pr_info("    %s", str.toStdString().c_str());
    }
}

void MainWindow::handleinstall() {   
    pr_info("begin handle install");
    QVector<QString> installlist;
    getListFromFrame(installlist, 1);
    handle->install(installlist, pubpath);
}

void MainWindow::handleuninstall() {
    pr_info("begin handle uninstall");
    QVector<QString> uninstalllist;
    getListFromFrame(uninstalllist, 0);
    handle->uninstall(uninstalllist);
}

void MainWindow::handlegenerate() {
    pr_info("begin generate self keys");
    SigUtil::generate_new_keypairs(privpath, pubpath);
}

void MainWindow::handlesign() {
    QString prefix = "/home/sora/gitbase/miscthings/conf/work/projects/dw_inst/localpkgs/";
    QVector<QString> signlist;
    getListFromFrame(signlist, 2);
    foreach(QString signstr, signlist) {
        signOnePkg(prefix + signstr + ".tar.gz", prefix + signstr + ".tgz");
    }
    pr_info("sign done");
}

void MainWindow::signOnePkg(QString from, QString to) {
    pr_info("begin sign\n");
    QByteArray sigarray;
    unsigned char tmpbuf[4096];
    unsigned int tmplen;
    int ret = SigUtil::sign(from, sigarray, privpath, pubpath);
    if (ret != 0) {
        pr_info("sign failed\n");
        return;
    }

    int fdin, fdout;
    fdin = open(from.toStdString().c_str(), O_RDONLY);
    if (fdin == -1) {
        pr_info("can not open input file for read\n");
        return;
    }
    fdout = open(to.toStdString().c_str(), O_WRONLY|O_CREAT|O_TRUNC, 0600);

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
}


