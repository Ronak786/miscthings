#ifndef PKGUTILS_H
#define PKGUTILS_H

#include <QString>

int do_copy_file(QString from, QString to);
int do_copy_pkg(QString pkgpath, QString installdir);
int uninstallpkg(QString pkgpath, QString installdir);

#endif // PKGUTILS_H
