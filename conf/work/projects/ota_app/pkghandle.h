/*************************************************************************
	> File Name: pkghandle.h
	> Author: davee-x
	> Mail: davee.naughty@gmail.com 
	> Created Time: 2018年06月10日 星期日 20时05分21秒
 ************************************************************************/

#ifndef PKG_HANDLE
#define PKG_HANDLE

#include "pkginfo.h"
#include <QVector>

class PkgHandle {
private:
	// vars loaded from config file, eg installprefix, compress method(design later), meta file name,remote path...
    QString _confpath;
    QString _prefixdir;     // install dir, the prefix
    QString _localmetafile; //local meta file name
    QString _remoteaddr;
    QString _remoteuser;
    QString _remotepass;
    QString _remotepkgdir; // remote ftpdir
    QString _remotemetafile; // remote meta file name
    QString _localpkgdir; // local download dir
	bool _daemon_flag; // should start this process as daemon or not

// private methods
    int getPkglist(QString file, QVector<PkgInfo> &vstr);
public:
    PkgHandle(QString confpath = QString(""));
	~PkgHandle();

    int loadConfig(); // default "" means use default conf inside, initialize all vars remember
	int init();
	int uninit();
    int getLocalpkglist(QVector<PkgInfo>& resultlist);
    int updateLocalpkglist(QVector<PkgInfo> &pkglist, int installflag);
    int getLocalpkginfo(QString pkgname, PkgInfo& );
    int installPkgs(QVector<PkgInfo>& pkglist);
    int uninstallPkgs(QVector<PkgInfo>& pkglist);
    int delPkgs(QVector<PkgInfo>& pkglist); //delete xxx.tar.gz after install
    int delPkgsdir(QVector<PkgInfo>& pkglist, int justsrc); // delete xxx/ after uninstall
    int extractPkgs(QVector<PkgInfo>& pkglist);
    int install(QVector<QString> &strlist); //tar.gz filenames to be installed
    int uninstall(QVector<QString> &strlist); // filename to be uninstalled
    int checkPkgs(QVector<PkgInfo>& infolist, QString& pubpath);
    int resizePkgs(QVector<PkgInfo>& pkglist);
};

#endif
