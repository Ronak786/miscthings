/*************************************************************************
	> File Name: pkginfo.h
	> Author: davee-x
	> Mail: davee.naughty@gmail.com 
	> Created Time: 2018年06月10日 星期日 22时03分02秒
 ************************************************************************/

#ifndef PKGINFO_H
#define PKGINFO_H

#include <QString>
#include "pkglib_global.h"

//name, version, architecture, install date(from utc 0), size,
//signature type, build data, packager, vendor, summary, description

class PKGLIBSHARED_EXPORT PkgInfo {
private:
// private members
    QString _name;
    QString _version;

// private functions

public:
    PkgInfo(): _name(QString("name")), _version(QString("version")) {}
    PkgInfo(const QString &name, const QString &ver);
	~PkgInfo();

	// member get methods
    QString getName();
    QString getVersion();
	bool operator==(const PkgInfo &one);
	PkgInfo& operator=(const PkgInfo &one);

	// install && uninstall
    int install(QString pkgpath, QString prefix);
    int uninstall(QString pkgpath, QString prefix);
};

#endif
