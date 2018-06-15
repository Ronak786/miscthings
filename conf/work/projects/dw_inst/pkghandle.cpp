/*************************************************************************
	> File Name: pkghandle.cpp
	> Author: davee-x
	> Mail: davee.naughty@gmail.com 
	> Created Time: 2018年06月10日 星期日 20时07分40秒
 ************************************************************************/

#include "pkghandle.h"

using namespace std;

PkgHandle::PkgHandle() {}

PkgHandle::~PkgHandle() {}

void PkgHandle::set_pkglist(const vector<PkgInfo> &plist) {
	_vpkg = plist;
}

const vector<PkgInfo>& PkgHandle::get_pkglist() {
	return _vpkg;
}
