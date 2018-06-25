/*************************************************************************
	> File Name: pkginfo.h
	> Author: davee-x
	> Mail: davee.naughty@gmail.com 
	> Created Time: 2018年06月10日 星期日 22时03分02秒
 ************************************************************************/

#include "pkginfo.h"
#include "sigutil.h"
#include "pkgutil.h"
#include "debug.h"

PkgInfo::PkgInfo(const QString &name, const QString &ver):
    _name(name), _version(ver) {}

PkgInfo::~PkgInfo() {
}

QString PkgInfo::getName() {return _name; }
QString PkgInfo::getVersion() {return _version;}


bool PkgInfo::operator==(const PkgInfo &another) { // if name.version == another's name.version, then same package
	return _name == another._name &&
		   _version == another._version;
}

PkgInfo& PkgInfo::operator=(const PkgInfo &one) {
	if (this == &one) return *this;
	this->_name = one._name;
	this->_version = one._version;
	return *this;
}

int PkgInfo::install(QString pkgpath, QString prefix) {
    return do_copy_pkg(pkgpath, prefix);
}

int PkgInfo::uninstall(QString pkgpath, QString prefix) {
    return uninstallpkg(pkgpath, prefix);
}
