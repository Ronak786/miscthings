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
#include "debug.h"

using json = nlohmann::json;

PkgHandle::PkgHandle() {
}

PkgHandle::~PkgHandle() {
}

int PkgHandle::init() {
	int ret = 0;
    return ret;
}

int PkgHandle::uninit() {
	return 0;
}

int PkgHandle::loadConfig(QString confpath) { // default "" means use default conf inside, initialize all vars remember
    if (confpath == "") {
		pr_info("config file emtpy, error");
		return -1; //can not find conf file
    }   

    std::ifstream ifs(confpath);
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
int PkgHandle::getLocalpkglist(QVector<PkgInfo> &infolist) {
    QString localmetapath = _localpkgdir + "/" + _localmetafile;
	return getPkglist(localmetapath, infolist);
}

int PkgHandle::getLocalpkginfo(QString pkgname, PkgInfo& info) {
    QVector<PkgInfo> locallist;
	getLocalpkglist(locallist);
    foreach (PkgInfo pkginfo, locallist) {
		if (pkginfo.getName() == pkgname) {
			info = pkginfo;
			return 0;
		}
	}
	return -1; // not exist
}

int PkgHandle::extractPkgs(QVector<PkgInfo>& pkglist) {
    char buf[4096];
	int ret = 0;
    foreach (PkgInfo pkg, pkglist) {
        QString pkgpath = _localpkgdir + pkg.getName() + "-" + pkg.getVersion() + ".tgz";
		snprintf(buf, sizeof(buf)-1, "tar xf %s -C %s", pkgpath.c_str(), _localpkgdir.c_str());
        pr_info("command is %s\n", buf);
		ret = system(buf);
		if (ret != 0) {
			return ret;
		}
	}
	return 0;
}

// delete tar pkgs
int PkgHandle::delPkgs(QVector<PkgInfo>& pkglist) {
	char buf[128];
	int ret = 0;
    foreach (PkgInfo pkg, pkglist) {
        QString pkgpath = _localpkgdir + pkg.getName() + "-" + pkg.getVersion() + ".tgz";
		snprintf(buf, sizeof(buf)-1, "rm -f %s", pkgpath.c_str());
//		pr_info(buf); // delete tar.gz file
//		pr_info("\n");
		ret = system(buf);
		if (ret != 0) return -1;
	}
	return ret;
}

int PkgHandle::delPkgsdir(QVector<PkgInfo>& pkglist, int justsrc) {
	char buf[128];
	int ret = 0;
    foreach (PkgInfo pkg, pkglist) {
        QString pkgpath;
        if (justsrc != 0) {
            pkgpath = _localpkgdir + pkg.getName() + "-" + pkg.getVersion() + "/src" ;
        } else {
            pkgpath = _localpkgdir + pkg.getName() + "-" + pkg.getVersion();
        }
		snprintf(buf, sizeof(buf)-1, "rm -rf %s", pkgpath.c_str());
		ret = system(buf);
//		pr_info(buf);
//		pr_info("\n");
	}
	return ret;
}

// TODO: return the error installed index ?
// or just return -1, then regenerate localpkglist??
int PkgHandle::installPkgs(QVector<PkgInfo>& pkglist) {
    foreach(PkgInfo pkg, pkglist) {
		pkg.install( _localpkgdir + pkg.getName() + "-" + pkg.getVersion(), _prefixdir);
	}
	return 0;
}

// TODO: similar to installPkgs
int PkgHandle::uninstallPkgs(QVector<PkgInfo>& pkglist) {
    foreach(PkgInfo pkg, pkglist) {
		pkg.uninstall(_localpkgdir + pkg.getName() + "-" + pkg.getVersion(), _prefixdir);
	}
	return 0;
}



//TODO: can not let pkginfo ctor detail be packaged into just pkginfo.cpp, but have to do here
//		do not know how to pass json objects
// add into pkginfo should be done in pkginfo.cpp
int PkgHandle::getPkglist(QString file, QVector<PkgInfo> &vstr) {
	std::ifstream ifs(file);
    if (!ifs || ifs.fail()) {
        pr_info("can not get meta file, just continue\n");
        return -1;
    }

    json content;
    ifs >> content;

    for (auto pkg: content) {
        /*
        vstr.push_back(PkgInfo(pkg["name"],pkg["version"],pkg["arch"],
					pkg["insdate"], pkg["builddate"], pkg["size"], pkg["sigtype"],
					pkg["packager"], pkg["summary"], pkg["desc"]));
        */
        vstr.push_back(PkgInfo(pkg["name"],pkg["version"]));
    }
	return 0;
}


int PkgHandle::updateLocalpkglist(QVector<PkgInfo> &vstr, int installflag) {
    QString localmetafile = _localpkgdir + _localmetafile;
	std::ifstream ifs(localmetafile);
    pr_info("localmeta: %s", localmetafile.c_str());
    if (!ifs) {
        pr_info("error occured when read local json in update\n");
        return -1;
    }
    json obj;
    ifs >> obj;

	if (installflag == 1) {
        foreach (PkgInfo item, vstr) {
            obj[item.getName().toStdString().c_str()]["name"] = item.getName().toStdString();
            obj[item.getName().toStdString().c_str()]["version"] = item.getVersion().toStdString();
            /*
			obj[item.getName().c_str()]["arch"] = item.getArchitecture();
			obj[item.getName().c_str()]["insdate"] = item.getInstalldate();
			obj[item.getName().c_str()]["builddate"] = item.getBuilddate();
			obj[item.getName().c_str()]["size"] = item.getSize();
			obj[item.getName().c_str()]["sigtype"] = item.getSigtype();
			obj[item.getName().c_str()]["packager"] = item.getPackager();
            obj[item.getName().c_str()]["summary"] = item.getSummary();

			obj[item.getName().c_str()]["desc"] = item.getDesc();
            */
		}
	} else if (installflag == 0) { 
        foreach(PkgInfo item, vstr) {
            obj.erase(item.getName().toStdString().c_str());
		}
	} else {
		pr_info("install flag error %d\n", installflag);
		return -1;
	}

    unlink(localmetafile.toStdString().c_str());
    std::ofstream ofs(localmetafile);
	if (!ofs || ofs.fail()) {
		pr_info("can not open meta file to update\n");
		return -1;
	}
    ofs << obj;
	return 0;
}

int PkgHandle::install(QVector<QString>& strlist) {
    QVector<PkgInfo> infolist;
    foreach(QString str, strlist) {
        auto const endofname = str.toStdString().find_last_of('-');
        auto const endofversion = str.toStdString().find_last_of('.');
        QString name = str.toStdString().substr(0, endofname);
        QString ver = str.toStdString().substr(endofname+1, endofversion - endofname -1);
        pr_info("name is %s, ver is %s", name.toStdString().c_str(), ver.toStdString().c_str());
        infolist.push_back(PkgInfo(name, ver));
    }
    if (extractPkgs(infolist) != 0) {
        pr_info("can not extract pkgs\n");
        return -1;
    }
    if (installPkgs(infolist) != 0) {
        pr_info("can not install pkgs\n");
        return -1;
    }
    if (updateLocalpkglist(infolist, 1) != 0) {
        pr_info("can not update locallist\n");
        return -1;
    }
    if (delPkgs(infolist) != 0) {
        pr_info("can not delete installed pkgs\n");
        return -1;
    }

    if (delPkgsdir(infolist, 1) != 0) {
        pr_info("can not delete just source\n");
        return -1;
    }
    return 0;
}

int PkgHandle::uninstall(QVector<QString> &strlist) {
    QVector<PkgInfo> vlocal;
    QVector<PkgInfo> vupdate;
    getLocalpkglist(vlocal);
    foreach (QString str, strlist) {
        foreach (PkgInfo pkg, vlocal) {
            if (pkg.getName() == str) {
                QString pkgpath = _localpkgdir + pkg.getName() + "-" + pkg.getVersion();
                pkg.uninstall(pkgpath, _prefixdir);
                vupdate.push_back(pkg);
            }
        }
    }
    if (delPkgsdir(vupdate, 0) != 0) {
        pr_info("can not delete pkgs dir\n");
        return -1;
    }
    if (updateLocalpkglist(vupdate, 0) != 0) {
        pr_info("can not update locallist for uninstall\n");
        return -1;
    }
    return 0;
}

