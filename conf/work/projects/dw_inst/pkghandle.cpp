/*************************************************************************
	> File Name: pkghandle.cpp
	> Author: davee-x
	> Mail: davee.naughty@gmail.com 
	> Created Time: 2018年06月10日 星期日 20时07分40秒
 ************************************************************************/

#include "pkghandle.h"

#define pr_info(s) std::cout << s << std::endl

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
    res = ftpclient->Connect(_remoteaddr.c_str());
    if (res != 1) { 
        ret = -1; 
        goto freeobj;
    }   
    pr_info("after connet\n");

    res = ftpclient->Login(_remoteuser.c_str(), _remotepass.c_str());
    if (res != 1) {
        ret = -1; 
        goto freeconnect;
    }   
    pr_info("after login\n");

    res = ftpclient->Chdir(_remotepkgdir.c_str());
    if (res != 1) {
        ret = -1; 
        goto freeconnect;
    }
	return ret;

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
			pr_info("added new pkg");
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

// TODO: return the error installed index ?
// or just return -1, then regenerate localpkglist??
int PkgHandle::installPkgs(std::vector<PkgInfo>& pkglist) {
	for(auto pkg: pkglist) {
		pkg.install(_prefixdir);
	}
	return 0;
}

// TODO: similar to installPkgs
int uninstallPkgs(std::vector<PkgInfo>& pkglist) {
	for(auto pkg: pkglist) {
		pkg.uninstall(_prefixdir);
	}
	return 0;
}

// put checksig into sigutil, turn into a static function
bool verifyPkg(std::string pkgname_ver, std::string keybuf) { //convert from char*, so should assert count before use it
	std::string verfile = pkgname_ver + ".sig";
	return Sigutil.checksig(pkgname_ver.c_str(), verfile.c_str(), keybuf.c_str(), keybuf.size());
}

// need to be: request from remote ftp and save file to localdir
//      ftp initialize should do every check circle, not every file download here!!
int PkgHandle::download(string fname) {
    int res = 0;

    pr_info("download file");

	std::string destfilepath = _localpkgdir + "/" + fname;
    unlink(destfilepath.c_str());

    res = ftp->Get(destfilepath.c_str(), fname.c_str(), ftplib::image);
    if (res != 1) {
        return -1; // get fail
    }
    return 0;
}

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

