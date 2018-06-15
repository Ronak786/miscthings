/*************************************************************************
	> File Name: pkginfo.h
	> Author: davee-x
	> Mail: davee.naughty@gmail.com 
	> Created Time: 2018年06月10日 星期日 22时03分02秒
 ************************************************************************/

#ifndef PKGINFO_H
#define PKGINFO_H

#include <string>

class PkgInfo {
private:
	std::string name;
	std::string version;
	std::string desc;
public:
	PkgInfo(std::string nm, std::string ver, std::string des);
	~PkgInfo();

	std::string getVersion();
	std::string getName();
	std::string getDesc();
	bool operator==(const PkgInfo &one);
	PkgInfo& operator=(const PkgInfo &one);
	std::string show();
};

#endif
