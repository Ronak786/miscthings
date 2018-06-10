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

using namespace std;

class PkgHandle {
private:
	vector<string> _vpkg;
public:
	PkgHandle();
	~PkgHandle();
	void set_pkglist(vector<string> plist);
	const vector<string>& get_pkglist();
};

#endif
