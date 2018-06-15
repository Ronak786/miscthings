/*************************************************************************
	> File Name: pkginfo.h
	> Author: davee-x
	> Mail: davee.naughty@gmail.com 
	> Created Time: 2018年06月10日 星期日 22时03分02秒
 ************************************************************************/

#include "pkginfo.h"

using namespace std;

PkgInfo::PkgInfo(string nm, string ver, string des): 
		name(nm), version(ver), desc(des) {}

PkgInfo::~PkgInfo() {}

string PkgInfo::getVersion() { return version;}
string PkgInfo::getName() { return name; }
string PkgInfo::getDesc() { return desc; }

bool PkgInfo::operator==(const PkgInfo &one) {
	return name == one.name &&
		   version == one.version;
}

PkgInfo& PkgInfo::operator=(const PkgInfo &one) {
	if (this == &one) return *this;
	this->name = one.name;
	this->version = one.version;
	this->desc = one.desc;
	return *this;
}

string PkgInfo::show() {
	return "name: " + name + " ,version: " + version + " ,desc: " + desc;
}
