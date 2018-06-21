/*************************************************************************
	> File Name: pkginfo.h
	> Author: davee-x
	> Mail: davee.naughty@gmail.com 
	> Created Time: 2018年06月10日 星期日 22时03分02秒
 ************************************************************************/

#ifndef PKGINFO_H
#define PKGINFO_H

#include <string>

//name, version, architecture, install date(from utc 0), size,
//signature type, build data, packager, vendor, summary, description

class PkgInfo {
private:
// private members
	std::string _name;
	std::string _version;
	std::string _architecture;
	unsigned long _installdate; // seconds start from utc 1970 0
	unsigned long _builddate;
	unsigned long _size;   // meatured in bytes
	std::string _sigtype; // ecdsa	
	std::string _packager; //author
	std::string _summary;  // summry of desc
	std::string _desc;  // detail desc

// private functions

public:
    PkgInfo(std::string name, std::string ver);
	~PkgInfo();

	// member get methods
	std::string getName();
	std::string getVersion();
	std::string getArchitecture();
	unsigned long getInstalldate();
	unsigned long getBuilddate();
	unsigned long getSize();
	std::string getSigtype();
	std::string getPackager();
	std::string getSummary();
	std::string getDesc();
	bool operator==(const PkgInfo &one);
	PkgInfo& operator=(const PkgInfo &one);

	// install && uninstall
	int install(std::string pkgpath, std::string prefix);
	int uninstall(std::string pkgpath, std::string prefix);
};

#endif
