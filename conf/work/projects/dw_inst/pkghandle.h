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
	// vars loaded from config file, eg installprefix, compress method(design later), meta file name,remote path...
	std::string _prefixdir;     // install dir, the prefix
	std::string _localmetafile; //local meta file name
	std::string _remoteaddr;
	std::string _remoteuser;
	std::string _remotepass;
	std::string _remotepkgdir; // remote ftpdir
	std::string _remotemetafile; // remote meta file name
	std::string _localpkgdir; // local download dir
	bool _daemon_flag; // should start this process as daemon or not
	std::string _confpath;
	ftplib *ftpclient;

// private methods
	int getPkglist(std::string file, std::vector<PkgInfo> &vstr);
	int download(std::fname);
public:
	PkgHandle();
	~PkgHandle();

	int loadConfig(std::string confpath); // default "" means use default conf inside, initialize all vars remember
	int init();
	int uninit();
	int getLocalpkglist(std::vector<PkgInfo>& resultlist);
	int getRemotepkglist(std::vector<PkgInfo>& reslist);
	int getNewpkglist(std::vector<PkgInfo>& reslist); //list of pkgs need upgrade/install(currently not distinguish
	int getLocalpkginfo(std::string pkgname, PkgInfo& );
	int getRemotepkginfo(std::string pkgname, PkgInfo&);
	bool verifyPkg(std::string pkgname_ver, std::string keybuf); //convert from char*, so should assert count before use it
	int installPkgs(std::vector<PkgInfo>& pkglist);
	int uninstallPkgs(std::vector<PkgInfo>& pkglist);
	int delpkgs(std::vector<PkgInfo>& pkglist); //delete xxx.tar.gz after install
	int delpkgsdir(std::vector<PkgInfo>& pkglist); // delete xxx/ after uninstall
};

#endif
