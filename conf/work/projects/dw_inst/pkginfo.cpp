/*************************************************************************
	> File Name: pkginfo.h
	> Author: davee-x
	> Mail: davee.naughty@gmail.com 
	> Created Time: 2018年06月10日 星期日 22时03分02秒
 ************************************************************************/

#include "pkginfo.h"

PkgInfo::PkgInfo(string nm, string ver, string siz): 
		name(nm), version(ver), size(siz) {}

~PkgInfo::PkgInfo() {}

string PkgInfo::getVersion() { return version;}
string PkgInfo::getName() { return name; }
string PkgInfo::getSize() { return size; }

bool PkgInfo::operator==(const PkgInfo &one) {
	return name == two.name &&
		   versio == two.version &&
		   size == two.size;
}

PkgInfo& PkgInfo::operator=(const PkgInfo &one) {
	if (this == &one) return *this;
	this->name = one.name;
	this->version = one.version;
	this->size = one.size;
	return *this;
}
