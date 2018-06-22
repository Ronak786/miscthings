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

#include <QDir>
#include "pkghandle.h"
#include "sigutil.h"
#include "debug.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>


PkgHandle::PkgHandle(QString confpath): _confpath(confpath) {
    loadConfig();
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


//TODO: should use qt's json process lib here
int PkgHandle::loadConfig() { // default "" means use default conf inside, initialize all vars remember
    if (_confpath == QString("")) {
		pr_info("config file emtpy, error");
		return -1; //can not find conf file
    }   

    QByteArray val;
    QFile file(_confpath);
    if(file.open(QIODevice::ReadOnly | QIODevice::Text) == false) {
        pr_info("can not open conf file \n");
        return -1;
    }
    val = file.readAll();
    file.close();
    QJsonDocument doc = QJsonDocument::fromJson(val);
    QJsonObject docobj = doc.object();

    for (QJsonObject::iterator it = docobj.begin(); it != docobj.end(); ++it) {
        if (it.key() == "localpkgdir") {
            _localpkgdir = it.value().toString();
        } else if (it.key() == "prefixdir") {
            _prefixdir = it.value().toString();
        } else if (it.key() == "remoteaddr") {
            _remoteaddr = it.value().toString();
        } else if (it.key() == "remoteuser") {
            _remoteuser = it.value().toString();
        } else if (it.key() == "remotepass") {
            _remotepass = it.value().toString();
        } else if (it.key() == "remotepkgdir") {
            _remotepkgdir = it.value().toString();
        } else if (it.key() == "daemonize") {
            _daemon_flag = it.value().toString();
        } else if (it.key() == "remotemeta") {
            _remotemetafile = it.value().toString();
        } else if (it.key() == "localmeta") {
            _localmetafile = it.value().toString();
        } else {
            pr_info("unknown json object: %s\n", it.key().toStdString().c_str());
            return -1;
        }
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

    if (resizePkgs(pkglist) != 0) {
        pr_info("get data part of package error\n");
        return -1;
    }

    foreach (PkgInfo pkg, pkglist) {
        QString pkgpath = _localpkgdir + pkg.getName() + "-" + pkg.getVersion() + ".tgz";
        snprintf(buf, sizeof(buf)-1, "tar -zxf %s -C %s", pkgpath.c_str(), _localpkgdir.c_str());
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
    foreach (PkgInfo pkg, pkglist) {
        QString pkgpath = _localpkgdir + pkg.getName() + "-" + pkg.getVersion() + ".tgz";
        QFile::remove(pkgpath);
	}
    return 0;
}

int PkgHandle::delPkgsdir(QVector<PkgInfo>& pkglist, int justsrc) {
    foreach (PkgInfo pkg, pkglist) {
        QString pkgpath;
        if (justsrc != 0) {
            pkgpath = _localpkgdir + pkg.getName() + "-" + pkg.getVersion() + "/src" ;
        } else {
            pkgpath = _localpkgdir + pkg.getName() + "-" + pkg.getVersion();
        }
        QDir dir(pkgpath);
        dir.removeRecursively();
	}
    return 0;
}

// TODO: return the error installed index ?
// or just return -1, then regenerate localpkglist??
int PkgHandle::installPkgs(QVector<PkgInfo>& pkglist) {
    foreach(PkgInfo pkg, pkglist) {
        if (pkg.install( _localpkgdir + pkg.getName() + "-" + pkg.getVersion(), _prefixdir) != 0) {
            return -1;
        }
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

    QFile qfile(file);
    if(qfile.open(QIODevice::ReadOnly | QIODevice::Text) == false) {
        pr_info("can not open file get pkg list\n");
        return -1;
    }
    QByteArray val = qfile.readAll();
    qfile.close();
    QJsonDocument doc = QJsonDocument::fromJson(val);
    QJsonObject docobj = doc.object();

    for (QJsonObject::iterator it = docobj.begin(); it != docobj.end(); ++it) {
        QJsonArray pkg = (*it).toArray();
        vstr.push_back(PkgInfo(pkg["name"].toString(),pkg["version"]).toString());
    }
	return 0;
}


int PkgHandle::updateLocalpkglist(QVector<PkgInfo> &vstr, int installflag) {
    QString localmetafile = _localpkgdir + _localmetafile;
    QFile qfile(localmetafile);
    if(qfile.open(QIODevice::ReadOnly | QIODevice::Text) == false) {
        pr_info("can not open file for get update pkg list\n");
        return -1;
    }

    QJsonDocument doc = QJsonDocument::fromJson(qfile.readAll());
    qfile.close();
    QJsonObject docobj = doc.object();

	if (installflag == 1) {
        foreach (PkgInfo item, vstr) {
            QJsonObject tmpobj{{"name", item.getName()}, {"version", item.getVersion()}};
            docobj.insert(item.getName(), tmpobj);

		}
	} else if (installflag == 0) { 
        foreach(PkgInfo item, vstr) {
            docobj.remove(item.getName());
		}
	} else {
		pr_info("install flag error %d\n", installflag);
		return -1;
	}

    // write qjsondocument doc into file
    if(qfile.open(QIODevice::WriteOnly | QIODevice::Truncate) == false) {
        pr_info("can not open file for get update pkg list write\n");
        return -1;
    }
    qfile.write(doc.toJson());
    qfile.close();

	return 0;
}

int PkgHandle::install(QVector<QString>& strlist, QString& pubpath) {
    QVector<PkgInfo> infolist;
    foreach(QString str, strlist) {
        QRegExp sep("[-.]");
        QString name = str.section(sep, 0,0);
        QString ver =  str.section(sep, 1,1);
        pr_info("name is %s, ver is %s", name.toStdString().c_str(), ver.toStdString().c_str());
        infolist.push_back(PkgInfo(name, ver));
    }

    if(checkPkgs(infolist, pubpath) != 0) {
        pr_info("verify failed pkgs\n");
        return -1;
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

int PkgHandle::checkPkgs(QVector<PkgInfo>& infolist, QString& pubpath) {
    foreach(PkgInfo pkg, infolist) {
        if (!SigUtil::verify(_localpkgdir + pkg.getName() + "-" + pkg.getVersion() + ".tgz", pubpath)) {
            pr_info("verify %s failed\n", pkg.getName().toStdString().c_str());
            return -1;
        }
    }
    return 0;
}

int PkgHandle::resizePkgs(QVector<PkgInfo>& pkglist) {
    foreach(PkgInfo pkg, pkglist) {
        if (!SigUtil::resize(_localpkgdir + pkg.getName() + "-" + pkg.getVersion() + ".tgz")) {
            pr_info("resize %s failed\n", pkg.getName().toStdString().c_str());
            return -1;
        }
    }
}

