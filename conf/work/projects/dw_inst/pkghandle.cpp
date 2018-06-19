/*************************************************************************
	> File Name: pkghandle.cpp
	> Author: davee-x
	> Mail: davee.naughty@gmail.com 
	> Created Time: 2018年06月10日 星期日 20时07分40秒
 ************************************************************************/

#include "pkghandle.h"

#define pr_info(s) std::cout << s << std::endl

PkgHandle::PkgHandle() {}

PkgHandle::~PkgHandle() {}

int PkgHandle::loadConfig(std::string confpath) { // default "" means use default conf inside, initialize all vars remember

    if (confpath.equal("")) {
		pr_info("config file emtpy, error");
		return -1; //can not find conf file
    }   

    ifstream ifs(confpath);
    if (ifs && !ifs.fail()) { //fstream work properly and file exists
		_confpath = confpath;

        json jconf;
        ifs >> jconf; // may have exception here ??

        for (json::iterator it = jconf.begin(); it != jconf.end(); ++it) {
            if (it.key() == "localpkgdir") {
                _localpkgdir = it.value();
            } else if (it.key() == "prefixdir") {
                _prefixdir = it.value();
            } else if (it.key() == "remoteaddr") {
                _remoteaddr = it.value();
            } else if (it.key() == "remoteuser") {
                _remoteuser = it.value();
            } else if (it.key() == "remotepass") {
                _remotepass = it.value();
            } else if (it.key() == "remotepkgdir") {
                _remotepkgdir = it.value();
            } else if (it.key() == "daemonize") {
                _daemon_flag = it.value();
            } else if (it.key() == "remotemeta") {
                _remotemetafile = it.value();
            } else if (it.key() == "localmeta") {
                _localmetafile = it.value();
            } else {
                cout << "unknown json object: " << it.key() << endl;
            }
        }
    } else {
        pr_info("config file read error\n");
		return -1;
    }
    pkgfilelocal = localdir + pkgfilelocalname;

    if (_daemon_flag) {
        daemon(1,0); // nochdir but close std streams
    }
    return 0;
}

int getLocalpkglist(std::vector<PkgInfo> &resultlist) {
	std::string localmetapath = _localpkgdir + "/" + _localmetafile;
	std::vector<PkgInfo> localpkglist;
	return getPkglist(localmetapath, resultlist);
}
std::vector<PkgInfo> getRemotepkglist();
std::vector<PkgInfo> getNewpkglist(); //list of pkgs need upgrade/install(currently not distinguish
PkgInfo getLocalpkginfo(std::string pkgname);
PkgInfo getRemotepkginfo(std::string pkgname);
int installPkgs(std::vector<PkgInfo> pkglist);
int uninstallPkgs(std::vector<PkgInfo> pkglist);
bool verifyPkg(std::string pkgname_ver, std::string keybuf); //convert from char*, so should assert count before use it

//TODO: can not let pkginfo ctor detail be packaged into just pkginfo.cpp, but have to do here
//		do not know how to pass json objects
// add into pkginfo should be done in pkginfo.cpp
int PkgHandle::getPkglist(string file, std::vector<PkgInfo> &vstr) {
    ifstream ifs(file);
    if (!ifs || ifs.fail()) {
        pr_info("can not get meta file, just continue\n");
        return -1;
    }

    json content;
    ifs >> content;

    for (auto pkg: content) {
        vstr.push_back(PkgInfo(pkg["name"],pkg["version"],pkg["arch"], 
					pkg["insdate"], pkg["builddate"], pkg["size"], pkg["sigtype"],
					pkg["packager"], pkg["summary"], pkg["desc"]));
    }
}

