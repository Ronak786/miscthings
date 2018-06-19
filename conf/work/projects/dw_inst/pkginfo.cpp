/*************************************************************************
	> File Name: pkginfo.h
	> Author: davee-x
	> Mail: davee.naughty@gmail.com 
	> Created Time: 2018年06月10日 星期日 22时03分02秒
 ************************************************************************/

#include "pkginfo.h"


PkgInfo(std::string name, std::string ver, std::string arch,
            unsigned long insdate, unsigned long builddate, unsigned long size, std::string sigtype,
            std::string packager, std::string summary, std::string desc):
	_name = name, _version = ver, _architecture = arch,
	_installdate = insdate, _builddate = buildate, _size = size, _sigtype = sigtype,
	_packager = packager, _summary = summary, _desc = desc {}

PkgInfo::~PkgInfo() {
	std::cout << "in pkg " << _name << "'s dtor" << std::endl;
}

std::string PkgInfo::getName() {return _name; }
std::string PkgInfo::getVersion() {return _version;}
std::string PkgInfo::getArchitecture() {return _architecture;}
unsigned long PkgInfo::getInstalldate() {return _installdate;}
unsigned long PkgInfo::getBuilddate() {return _builddate;}
unsigned long PkgInfo::getSize() {return _size;}
std::string PkgInfo::getSigtype() {return _sigtype;}
std::string PkgInfo::getPackager() {return _packager;}
std::string PkgInfo::getSummary() {return _summary;}
std::string PkgInfo::getDesc() {return _desc;}


bool PkgInfo::operator==(const PkgInfo &another) { // if name.version == another's name.version, then same package
	return name == another.name &&
		   version == another.version;
}

PkgInfo& PkgInfo::operator=(const PkgInfo &one) {
	if (this == &one) return *this;
	this->_name = one._name;
	this->_version = one._version;
	this->_architecture = one._architecture;
	this->_installdate = one._installdate;
	this->_builddate = one._builddate;
	this->_size = one._size;
	this->_sigtype = one._sigtype;
	this->_packager = one._packager;
	this->_summary = one._summary;
	this->_desc = one._desc;
	return *this;
}

int PkgInfo::install(std::string pkgpath, std::string prefix) {
	do_copy_pkg(pkgpath, prefix);
}

int PkgInfo::uninstall(std::string pkgpath, std::string prefix) {
	uninstallpkg(pkgpath, prefix);
}
