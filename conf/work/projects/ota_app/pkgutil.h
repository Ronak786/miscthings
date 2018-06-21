#ifndef PKGUTILS_H
#define PKGUTILS_H

#include <string>

int do_copy_file(std::string from, std::string to);
int do_copy_pkg(std::string pkgpath, std::string installdir);
int uninstallpkg(std::string pkgpath, std::string installdir);

#endif // PKGUTILS_H
