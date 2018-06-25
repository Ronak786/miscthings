#include "pkgutil.h"
#include "debug.h"
#include <QFile>
#include <QDir>


int do_copy_file(QString from, QString to) {
    QFile::remove(to);
    QFileInfo finfo(from);
    if (!finfo.isSymLink()) {
        pr_info("%s is not symlink\n", from.toStdString().c_str());
        if (QFile::copy(from, to)) {
            return 0;
        } else {
            return -1;
        }
    } else {

        QString target = finfo.symLinkTarget();
 //       pr_info("%s is symlink of %s\n", from.toStdString().c_str(), target.toStdString().c_str());
        char buf[1024];
        snprintf(buf, 1023, "cp -a %s %s", from.toStdString().c_str(), to.toStdString().c_str());
        system(buf);
        // qt cannot handle symlink well, use gnu version here
        /*
        if (QFile::link(target, to)) {
            return 0;
        } else {
            return -1;
        }
        */
    }
    return 0;
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
        QString fullfromline = pkgpath + "/src/" + pathline;
        QString fulltoline = installdir + pathline;
        QFileInfo finfo(fullfromline);
        if (finfo.isDir()) {
            pr_info("makedir: %s\n", fulltoline.toStdString().c_str());
            qdir.mkpath(fulltoline); // make dir, we use find(1) make sure dir created before copy file
        } else {
            do_copy_file(fullfromline, fulltoline);
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
        QString fullline = installdir + pathline;
        QFileInfo finfo(fullline);
        if (!finfo.isDir()) {
            QFile::remove(fullline);
            pr_info("unlink: %s\n", fullline.toStdString().c_str());
        } else {
            reverselines.push_back(fullline); // push all dirs in reverse order
        }
    }

    listfile.close();

    // remove dirs if empty
    QVector<QString>::reverse_iterator i;
    for(i = reverselines.rbegin(); i != reverselines.rend(); ++i) {
        QString tmpdir = *i;
        pr_info("remove dir %s\n", tmpdir.toStdString().c_str());
        qdir.rmdir(tmpdir);
    }
    return 0;
}
