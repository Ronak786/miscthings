/*************************************************************************
	> File Name: pkghandle.cpp
	> Author: davee-x
	> Mail: davee.naughty@gmail.com 
	> Created Time: 2018年06月10日 星期日 20时07分40秒
 ************************************************************************/

#include "pkghandle.h"


PkgHandle::PkgHandle() {}

PkgHandle::~PkgHandle() {}

void PkgHandle::set_pkglist(vector<string> plist) {
	_vpkg = plist;
}

const vector<string>& PkgHandle::get_pkglist() {
	return _vpkg;
}
