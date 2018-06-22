#include "pkgutil.h"
#include "debug.h"
#include <QFile>
#include <QDir>


int do_copy_file(QString from, QString to) {
    QFile::remove(to);
    if (QFile::copy(from, to)) {
        return 0;
    } else {
        return -1;
    }
}

int do_copy_pkg(QString pkgpath, QString installdir) {
    QFile listfile(pkgpath + "/FILELIST.lst");
    if (!listfile.open(QIODevice::ReadOnly)) {
        pr_info("can not copy package path %s\n", pkgpath.toStdString().c_str());
        return -1;
    }
    QTextStream liststream(&listfile);
    QString pathline;
    QDir qdir;

    while (!liststream.atEnd()) {
        pathline = liststream.readLine();
        if (pathline[pathline.length()-1] == QChar('/')) {
            QString tmpdir = installdir + pathline;
            pr_info("makedir: %s%s\n", installdir.toStdString().c_str(), pathline.toStdString().c_str());
            qdir.mkpath(tmpdir); // make dir, we use find(1) make sure dir created before copy file
        } else {
            do_copy_file(pkgpath + "/src/" + pathline, installdir + pathline);
        }
    }
    listfile.close();
    return 0;
}

int uninstallpkg(QString pkgpath, QString installdir) {
    QFile listfile(pkgpath + "/FILELIST.lst");
    if (!listfile.open(QIODevice::ReadOnly)) {
        pr_info("can not open file and uninstall, may be first install\n");
        return -1;
    }

    QVector<QString> reverselines;
    QTextStream liststream(&listfile);
    QString pathline;
    QDir qdir;

    while (!liststream.atEnd()) {
        pathline = liststream.readLine();
        if (pathline[pathline.length()-1] != QChar('/')) {
            QString tmpdir = installdir + pathline;
            qdir.rmdir(tmpdir);
            pr_info("unlink: %s\n", tmpdir.toStdString().c_str());
        } else {
            reverselines.push_back(pathline); // push all dirs in reverse order
        }
    }

    listfile.close();

    // remove dirs if empty
    QVector<QString>::iterator i;
    for(i = reverselines.rbegin(); i != reverselines.rend(); ++i) {
        QString tmpdir = installdir + *i;
        pr_info("remove dir");
        qdir.rmdir(tmpdir);
    }
    return 0;
}
