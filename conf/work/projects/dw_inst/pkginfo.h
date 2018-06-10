/*************************************************************************
	> File Name: pkginfo.h
	> Author: davee-x
	> Mail: davee.naughty@gmail.com 
	> Created Time: 2018年06月10日 星期日 22时03分02秒
 ************************************************************************/

#ifndef PKGINFO_H
#define PKGINFO_H

#include <string>
using namespace std;

class PkgInfo {
private:
	string name;
	string version;
	string size;
public:
	PkgInfo(string nm, string ver, string siz);
	~PkgInfo();

	string getVersion();
	string getName();
	string getSize();
	bool operator==(const PkgInfo &one);
	PkgInfo& operator=(const PkgInfo &one);
};

#endif
