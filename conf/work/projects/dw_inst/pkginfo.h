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
	string desc;
public:
	PkgInfo(string nm, string ver, string des);
	~PkgInfo();

	string getVersion();
	string getName();
	string getDesc();
	bool operator==(const PkgInfo &one);
	PkgInfo& operator=(const PkgInfo &one);
};

#endif
