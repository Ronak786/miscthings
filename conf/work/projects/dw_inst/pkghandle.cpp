/*************************************************************************
	> File Name: pkghandle.cpp
	> Author: davee-x
	> Mail: davee.naughty@gmail.com 
	> Created Time: 2018年06月10日 星期日 20时07分40秒
 ************************************************************************/

#include <fstream>

#ifdef __cplusplus
extern "C" {
#endif

#include <unistd.h>

#ifdef __cplusplus
}
#endif

#include "json.h"
#include "pkghandle.h"
#include "sigutil.h"


#ifdef _DEBUG
#include <cstdio>
#define pr_info(f_, ...) printf((f_), ##__VA_ARGS__)
#else
#define pr_info(f_, ...)
#endif

using json = nlohmann::json;

PkgHandle::PkgHandle() {
	ftpclient = nullptr;
}

PkgHandle::~PkgHandle() {
	if (ftpclient) {
		free(ftpclient);
	}
	ftpclient = nullptr;
}

int PkgHandle::init() {
	int ret = 0;
    ftpclient = new ftplib();
    ret = ftpclient->Connect(_remoteaddr.c_str());
    if (ret != 1) { 
		ret = -1;
        goto freeobj;
    }   
    pr_info("after connet\n");

    ret = ftpclient->Login(_remoteuser.c_str(), _remotepass.c_str());
    if (ret != 1) {
        ret = -1; 
        goto freeconnect;
    }   
    pr_info("after login\n");

    ret = ftpclient->Chdir(_remotepkgdir.c_str());
    if (ret != 1) {
        ret = -1; 
		pr_info("can not chdir to %s\n", _remotepkgdir.c_str());
        goto freeconnect;
    }
	return 0;

freeconnect:
    ftpclient->Quit();
freeobj:
    free(ftpclient);
	ftpclient = nullptr;
    return ret;
}

int PkgHandle::uninit() {
	if (ftpclient != nullptr) {
		ftpclient->Quit();
		free(ftpclient);
		ftpclient = nullptr;
	}
	return 0;
}

int PkgHandle::loadConfig(std::string confpath) { // default "" means use default conf inside, initialize all vars remember
    if (confpath == "") {
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
				pr_info("unknown json object: %s\n", it.key().c_str());
				return -1;
            }
        }
    } else {
        pr_info("config file read error\n");
		return -1;
    }

    if (_daemon_flag) {
        daemon(1,0); // nochdir but close std streams
    }
    return 0;
}

// get list of remote pakages
int PkgHandle::getLocalpkglist(std::vector<PkgInfo> &infolist) {
	std::string localmetapath = _localpkgdir + "/" + _localmetafile;
	return getPkglist(localmetapath, infolist);
}

// get list of local installed packages
int PkgHandle::getRemotepkglist(std::vector<PkgInfo>& infolist) {
	download(_remotemetafile);
	std::string remotemetapath = _localpkgdir + "/" + _remotemetafile;
	return getPkglist(remotemetapath, infolist);
}

// TODO: should check return value of getlocal and get remote and return -1 if fail
int PkgHandle::getNewpkglist(std::vector<PkgInfo>& infolist) { //list of pkgs need upgrade/install(currently not distinguish
	std::vector<PkgInfo> remotelist, locallist;
	getLocalpkglist(locallist);
	getRemotepkglist(remotelist);
	for (auto remote: remotelist) {
		if (find(locallist.begin(), locallist.end(), remote) == locallist.end()) {
			infolist.push_back(remote);
		}
	}
	return 0;
}

int PkgHandle::getLocalpkginfo(std::string pkgname, PkgInfo& info) {
	std::vector<PkgInfo> locallist;
	getLocalpkglist(locallist);
	for (auto pkginfo: locallist) {
		if (pkginfo.getName() == pkgname) {
			info = pkginfo;
			return 0;
		}
	}
	return -1; // not exist
}

int PkgHandle::getRemotepkginfo(std::string pkgname, PkgInfo& info) {
	std::vector<PkgInfo> remotelist;
	getLocalpkglist(remotelist);
	for (auto pkginfo: remotelist) {
		if (pkginfo.getName() == pkgname) {
			info = pkginfo;
			return 0;
		}
	}
	return -1; // not exist
}

int PkgHandle::extractPkgs(std::vector<PkgInfo>& pkglist) {
	char buf[128];
	int ret = 0;
	for (auto pkg: pkglist) {
		std::string pkgpath = _localpkgdir + pkg.getName() + "-" + pkg.getVersion() + ".tar.gz";
		snprintf(buf, sizeof(buf)-1, "tar xf %s -C %s", pkgpath.c_str(), _localpkgdir.c_str());
		ret = system(buf);
		if (ret != 0) {
			return ret;
		}
	}
	return 0;
}

int PkgHandle::downloadPkgs(std::vector<PkgInfo>& pkglist) {
	for (auto pkg: pkglist) {
		if (download(pkg.getName() + "-" + pkg.getVersion() + ".tar.gz") != 0) {
			pr_info("can not get file %s\n", pkg.getName().c_str());
			return -1;
		}
		if (download(pkg.getName() + "-" + pkg.getVersion() + ".tar.gz.sig") != 0) {
			pr_info("can not get file %s signature\n", pkg.getName().c_str());
			return -1;
		}
	}
	return 0;
}

// delete tar pkgs
int PkgHandle::delPkgs(std::vector<PkgInfo>& pkglist) {
	char buf[128];
	int ret = 0;
	for (auto pkg: pkglist) {
		std::string pkgpath = _localpkgdir + pkg.getName() + "-" + pkg.getVersion() + ".tar.gz";
		snprintf(buf, sizeof(buf)-1, "rm -f %s", pkgpath.c_str());
//		pr_info(buf); // delete tar.gz file
//		pr_info("\n");
		ret = system(buf);
		if (ret != 0) return -1;

		pkgpath = _localpkgdir + pkg.getName() + "-" + pkg.getVersion() + ".tar.gz.sig";
		snprintf(buf, sizeof(buf)-1, "rm -f %s", pkgpath.c_str());
//		pr_info(buf); // delete sig file
//		pr_info("\n");
		ret = system(buf);
		if (ret != 0) return -1;
	}
	return ret;
}

int PkgHandle::delPkgsdir(std::vector<PkgInfo>& pkglist) {
	char buf[128];
	int ret = 0;
	for (auto pkg: pkglist) {
		std::string pkgpath = _localpkgdir + pkg.getName() + "-" + pkg.getVersion();
		snprintf(buf, sizeof(buf)-1, "rm -rf %s", pkgpath.c_str());
		ret = system(buf);
//		pr_info(buf);
//		pr_info("\n");
	}
	return ret;
}

// TODO: return the error installed index ?
// or just return -1, then regenerate localpkglist??
int PkgHandle::installPkgs(std::vector<PkgInfo>& pkglist) {
	for(auto pkg: pkglist) {
		pkg.install( _localpkgdir + pkg.getName() + "-" + pkg.getVersion(), _prefixdir);
	}
	return 0;
}

// TODO: similar to installPkgs
int PkgHandle::uninstallPkgs(std::vector<PkgInfo>& pkglist) {
	for(auto pkg: pkglist) {
		pkg.uninstall(_localpkgdir + pkg.getName() + "-" + pkg.getVersion(), _prefixdir);
	}
	return 0;
}

// put checksig into sigutil, turn into a static function
// pass in should be a full path
// TODO:
// checksig need a prefix??
bool PkgHandle::verifyPkgs(std::vector<PkgInfo>& pkglist) { //convert from char*, so should assert count before use it
	for (auto info: pkglist) {
		std::string pkgfile = _localpkgdir + info.getName() + "-" + info.getVersion() + ".tar.gz";
		std::string verfile = pkgfile + ".sig";
		std::string pubkeypath = _localpkgdir + "pubkey.pem";
		if (checksig(pkgfile.c_str(), verfile.c_str(), pubkeypath.c_str()) != true) {
			pr_info("verify %s failed\n", info.getName().c_str());
			return -1;
		}
	}
	return 0;
}

// need to be: request from remote ftp and save file to localdir
//      ftp initialize should do every check circle, not every file download here!!
int PkgHandle::download(std::string fname) {
    int res = 0;

    pr_info("download file %s\n", fname.c_str());

	std::string destfilepath = _localpkgdir + "/" + fname;
    unlink(destfilepath.c_str());

    res = ftpclient->Get(destfilepath.c_str(), fname.c_str(), ftplib::image);
    if (res != 1) {
        return -1; // get fail
    }
    return 0;
}

//TODO: can not let pkginfo ctor detail be packaged into just pkginfo.cpp, but have to do here
//		do not know how to pass json objects
// add into pkginfo should be done in pkginfo.cpp
int PkgHandle::getPkglist(std::string file, std::vector<PkgInfo> &vstr) {
	std::ifstream ifs(file);
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
	return 0;
}


int PkgHandle::updateLocalpkglist(std::vector<PkgInfo> &vstr, int installflag) {
	std::string localmetafile = _localpkgdir + _localmetafile;
	std::ifstream ifs(localmetafile);
    if (!ifs) {
        pr_info("error occured when read local json in update\n");
        return -1;
    }
    json obj;
    ifs >> obj;

	if (installflag == 1) {
		for (auto item: vstr) {
			obj[item.getName().c_str()]["name"] = item.getName();
			obj[item.getName().c_str()]["version"] = item.getVersion();
			obj[item.getName().c_str()]["arch"] = item.getArchitecture();
			obj[item.getName().c_str()]["insdate"] = item.getInstalldate();
			obj[item.getName().c_str()]["builddate"] = item.getBuilddate();
			obj[item.getName().c_str()]["size"] = item.getSize();
			obj[item.getName().c_str()]["sigtype"] = item.getSigtype();
			obj[item.getName().c_str()]["packager"] = item.getPackager();
			obj[item.getName().c_str()]["summary"] = item.getSummary();
			obj[item.getName().c_str()]["desc"] = item.getDesc();
		}
	} else if (installflag == 0) { 
		for(auto item: vstr) {
			obj.erase(item.getName().c_str());
		}
	} else {
		pr_info("install flag error %d\n", installflag);
		return -1;
	}

	unlink(localmetafile.c_str());
    ofstream ofs(localmetafile);
	if (!ofs || ofs.fail()) {
		pr_info("can not open meta file to update\n");
		return -1;
	}
    ofs << obj;
	return 0;
}
