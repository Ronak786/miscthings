/*************************************************************************
	> File Name: pkghandle.h
	> Author: davee-x
	> Mail: davee.naughty@gmail.com 
	> Created Time: 2018年06月10日 星期日 20时05分21秒
 ************************************************************************/

#ifndef PKG_HANDLE
#define PKG_HANDLE

#include <string>
#include <vector>
#include "pkginfo.h"


class PkgHandle {
private:
	std::vector<PkgInfo> _vpkg;
public:
	PkgHandle();
	~PkgHandle();
	void set_pkglist(const std::vector<PkgInfo> &plist);
	const std::vector<PkgInfo>& get_pkglist();
};

#endif
