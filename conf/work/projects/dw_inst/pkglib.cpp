/*************************************************************************
	> File Name: client.c
	> Author: davee-x
	> Mail: davee.naughty@gmail.com 
	> Created Time: 2018年06月10日 星期日 19时28分13秒
 ************************************************************************/

#include "pkgmgnt.h"
#include "pkghandle.h"
#include "json.h"
#include "ftplib.h"
#include "sigutil.h"

#include <unistd.h>
#include <algorithm>
#include <fstream>
#include <cstring>
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <vector>
#include <string>

using namespace std;


#ifdef _DEBUG
#define pr_info(f_, ...) printf((f_), ##__VA_ARGS__)
#else
#define pr_info(f_, ...)
#endif

using json = nlohmann::json;

// config params
static string localdir;
static string installdir;
static string pkgfilelocal;
static string pkgfileremotename;
static string pkgfilelocalname;
static string remoteaddr;
static string remoteuser;
static string remotepass;
static string remotepkgdir; // remote ftpdir
static bool daemon_flag = false;

static string defconf = "localpkgs/config.json"; //default place of config file
static ftplib *ftp = NULL;

static void compare_and_list_new(const vector<PkgInfo> vremote, vector<PkgInfo> &vnew);
static int install_and_updatelocal(PkgHandle &hdl);
static void do_copy_pkg(string pkgname);
static void do_copy_file(string from, string to);
static int download(string pkgfile);
static bool extract_and_install(string &pkgfile, string &pkgver);
static void uninstallpkg(string pkgfile);
static bool checksig(string fname, string fsig);
static void getpkglist(string filename, vector<PkgInfo> &vstr);

// demo main
int main(int ac, char *av[]) {

	load_config(ac, av);

	bool stop = false;

	while (!stop) {
		PkgHandle hdl;
		if (init_handle(hdl) == -1) {
			pr_info("fail init\n");
			return -1;
		}
		pr_info("begin check\n");
		if (get_and_check(hdl)) {
			pr_info("we have new pkgs\n");
			dumppkgs(hdl);
			updatepkgs(hdl);
		}
		uninit_handle(hdl);
		sleep(5);
	}
}

int init_handle(PkgHandle &hdl) {
	int ret = 0, res = 0;
	// if local package meta file not exist, create an empty one
	if (access(pkgfilelocal.c_str(), F_OK) != 0) {
		ofstream ofs(pkgfilelocal);
		json tmpobj = json({});
		ofs << tmpobj;
	}
	ftp = new ftplib();
	res = ftp->Connect(remoteaddr.c_str());
	if (res != 1) { 
		ret = -1;
		goto freeobj;
	}
	pr_info("after connet\n");

	res = ftp->Login(remoteuser.c_str(), remotepass.c_str());
	if (res != 1) {
		ret = -1;
		goto freeconnect;
	}
	pr_info("after login\n");

	res = ftp->Chdir(remotepkgdir.c_str());
	if (res != 1) {
		ret = -1;
		goto freeconnect;
	}
	pr_info("inited \n");

	return ret;

freeconnect:
	ftp->Quit();
freeobj:
	free(ftp);
	return ret;
}

void uninit_handle(PkgHandle &hdl) {
	ftp->Quit();
	free(ftp);
}

// get a list of new pkgs needed to be download
// currently list is just one line 
bool get_and_check(PkgHandle &hdl) {
	vector<PkgInfo> vremote, vnew;

	download(pkgfileremotename);
	getpkglist(localdir + pkgfileremotename, vremote);
	compare_and_list_new(vremote, vnew);
	if (!vnew.empty()) {
		hdl.set_pkglist(vnew);
		return true;
	} else {
		return false;
	}
}

void dumppkgs(PkgHandle &hdl) {
	for(auto pkg: hdl.get_pkglist()) {
		showInfo(pkg);
	}
}

void showInfo(PkgInfo& pkg) {
	cout << pkg.show() << endl;
}

void updatepkgs(PkgHandle &hdl) {
	pr_info("updating\n");
	install_and_updatelocal(hdl);
	pr_info("end update\n");
}

int install_and_updatelocal(PkgHandle &hdl) {
	vector<PkgInfo> vnew = hdl.get_pkglist();
	ifstream ifs(pkgfilelocal);
	if (!ifs) {
		pr_info("error occured when read local json\n");
		return -1;
	}
	json obj;
	ifs >> obj;
	for(auto item: vnew) {
		// update packages
		string pkgfile = item.getName();
		string pkgver = item.getVersion();
		download(pkgfile + "-" + pkgver + ".tar.gz");
		download(pkgfile + "-" + pkgver + ".tar.gz.sig");
		
		//check signature
		if (!checksig(pkgfile + "-" + pkgver + ".tar.gz", pkgfile + "-" + pkgver +".tar.gz.sig")) {
			pr_info("check sig fail, can not install %s\n", pkgfile.c_str());
			continue;
		}
		if (!extract_and_install(pkgfile, pkgver)) {
			pr_info("can not install pkg %s\n", item.getName().c_str());
			continue;
		}

		// update local meta info 
		// TODO: obj assign should be better do in pkginfo file
		pr_info("process %s\n", item.getName().c_str());
		obj[item.getName().c_str()]["name"] = item.getName();
		obj[item.getName().c_str()]["version"] = item.getVersion();
		obj[item.getName().c_str()]["desc"] = item.getDesc();
		pr_info("install pkg %s success\n", item.getName().c_str());
	}
	// remove old pkgconfig
	unlink(pkgfilelocal.c_str());
	ofstream ofs(pkgfilelocal);
	ofs << obj;
	return 0;
}

// need to be: request from remote ftp and save file to localdir
//		ftp initialize should do every check circle, not every file download here!!
int download(string pkgfile) {
	int res = 0;

	pr_info("download file %s\n", pkgfile.c_str());

	string localfilepath = localdir + pkgfile;
	unlink(localfilepath.c_str());

	res = ftp->Get(localfilepath.c_str(), pkgfile.c_str(), ftplib::image);
	if (res != 1) {
		return -1;
	}
	return 0;
}

void do_copy_file(string from, string to) {
	unlink(to.c_str());
	ifstream ifs(from);
	ofstream ofs(to);
	if (!ifs || !ofs) {
		pr_info("do_copy_file can't get dir of copy file ifs or ofs\n");
		return;
	}
	ofs << ifs.rdbuf();	
}

void do_copy_pkg(string pkgname) {
	ifstream ifslist(localdir + pkgname + "/FILELIST.lst");
	string pathline;
	if (!ifslist) {
		pr_info("can not copy package path %s\n", pkgname.c_str());
		return;
	}
	while (std::getline(ifslist, pathline)) {
		if (pathline[pathline.length()-1] == '/') {
			string tmpdir = installdir + pathline;
			pr_info("makedir: %s%s\n", installdir.c_str(), pathline.c_str());
			mkdir(tmpdir.c_str(), 0775); // make dir, we use find(1) make sure dir created before copy file
		} else {
			pr_info("copy: %s/src/%s to: %s%s\n", localdir.c_str(), pkgname.c_str(),
					installdir.c_str(), pathline.c_str());
			do_copy_file(localdir + pkgname + "/src/" + pathline, installdir + pathline);
		}
	}
	ifslist.close();
}

void uninstallpkg(string pkgname) {
	ifstream ifslist(localdir + pkgname + "/FILELIST.lst");
	if (!ifslist) {
		pr_info("can not open file for %s and uninstall, may be first install\n", pkgname.c_str());
		return;
	}

	vector<string> reverselines;
	string pathline;
	while (std::getline(ifslist, pathline)) {
		if (pathline[pathline.length()-1] != '/') {
			string tmpdir = installdir + pathline;
			unlink(tmpdir.c_str());
			cout << "unlink: " << tmpdir << endl;
		} else {
			reverselines.push_back(pathline); // push all dirs in reverse order
		}
	}
	// remove dirs if empty
	for (auto i = reverselines.rbegin(); i != reverselines.rend(); ++i) {
		string tmpdir = installdir + *i;
		cout << "remove dir " << tmpdir << endl;
		rmdir(tmpdir.c_str());
	}
}

bool extract_and_install(string &pkgfile, string &pkgver) {
	string localpath = localdir + pkgfile + "-" + pkgver + ".tar.gz";
	string localpathdir = localdir + pkgfile + "-" + pkgver ;

	// uninstall local pre installed version of tobe installed file
	vector<PkgInfo> vlocal;
	getpkglist(pkgfilelocal, vlocal);
	for(auto item: vlocal) {
		if (item.getName() == pkgfile) {
			uninstallpkg(pkgfile + "-" + item.getVersion());
		}
	}

	char localbuf[200];
	snprintf(localbuf, sizeof(localbuf)-1 , "rm -rf %s; tar xf %s -C %s", 
		localpathdir.c_str(), localpath.c_str(), localdir.c_str());
	int ret = system(localbuf);
	if (WEXITSTATUS(ret) != 0) {
		pr_info("can not extract tar ball\n");
		return false;
	}
	do_copy_pkg(pkgfile + "-" + pkgver);
	unlink(localpath.c_str());
	return true;
}

// add into pkginfo should be done in pkginfo.cpp
void getpkglist(string file, vector<PkgInfo> &vstr) {
	ifstream ifs(file);
	if (!ifs) {
		pr_info("can not get meta file, just continue\n");
		return;
	}

	json content;
	ifs >> content;

	pr_info("file %s\n", file.c_str());
	for (auto pkg: content) {
		vstr.push_back(PkgInfo(pkg["name"],pkg["version"],pkg["desc"]));
	}
}

//vremote: contain downloaded list
//vnew: return list of pkgs needed to be downloade
void compare_and_list_new(const vector<PkgInfo> vremote, vector<PkgInfo> &vnew) {
	vector<PkgInfo> vlocal;
	getpkglist(pkgfilelocal, vlocal);
	for (auto remote: vremote) {
		if (find(vlocal.begin(), vlocal.end(), remote) == vlocal.end()) {
			pr_info("added new pkg: %s\n", remote.getName().c_str());
			vnew.push_back(remote);
		}
	}
}

bool checksig(string fnamestr, string fsigstr) {
	string filepath = localdir + fnamestr;
	string sigpath = localdir + fsigstr;
	string pubkeypath = localdir + "pubkey.pem";
	const char *fname = filepath.c_str();
	const char *fsig = sigpath.c_str();
	unsigned char content[SHA256_DIGEST_LENGTH];
	EC_KEY * pubeckey;
	int siglen;
	char *sig = NULL; //buffer to store signature
	int res = 0;
	bool ret = true;

	if (get_sha256(fname, content) == -1) {
		pr_info("can not get sha\n");
		ret = false;
		goto freenon;
	}

	if (readpubeckey(&pubeckey, pubkeypath.c_str()) == -1) {
		pr_info("can not get pubeckey\n");
		ret = false;
		goto freenon;
	}

	if ((siglen = readkeyfromfile(fsig, &sig)) == -1) {
		pr_info("can not read sig from file %s\n", fname);
		ret = false;
		goto freekey;
	}
	
	res = ecdsa_verify(content, SHA256_DIGEST_LENGTH, (unsigned char*)sig, (unsigned int)siglen, pubkeypath.c_str());
	switch(res) {
	case -1:
	  pr_info("verify error occureed\n");
	  ret = false;
	  break;
	case 0:
	  pr_info("verify fail\n");
	  ret = false;
	  break;
	default:
	  pr_info("verify success file %s, can install\n", fname);
	  ret = true;
	  break;
	}

	free(sig);

freekey:
	EC_KEY_free(pubeckey);
freenon:
	return ret;
}

/*
 * recognized option:
 * daemon: (bool)true/false
 * remoteaddr: (string)ip:port
 * remoteuser: (string)username
 * remotepass: (string)password
 * remotepkgdir:  (string)ftp's dir
 * localpkgdir: (string)pkg download dir
 * installdir: (string)pkg install rootdir, on board is persist
 */
int load_config(int ac, char *av[]) {
	// set cmdline specified conf file 
	// set default vars first
	localdir = "dumpdir/";
	installdir = "dumpdir/";

	defconf = "localpkgs/config.json";
	pkgfileremotename = "readme";
	pkgfilelocalname = "readme";
	remoteaddr = "192.168.0.1:21";
	remoteuser = "dumb";
	remotepass = "123456";
	remotepkgdir = "/persist/ftpdir/"; // remote ftpdir
	daemon_flag = false;

	if (ac >= 2) {
		defconf = av[1];
	}

	ifstream ifs(defconf);
	if (ifs && !ifs.fail()) { //fstream work properly and file exists
		json jconf;
		ifs >> jconf;

		for (json::iterator it = jconf.begin(); it != jconf.end(); ++it) {
			if (it.key() == "localpkgdir") {
				localdir = it.value();
			} else if (it.key() == "installdir") {
				installdir = it.value();
			} else if (it.key() == "remoteaddr") {
				remoteaddr = it.value();
			} else if (it.key() == "remoteuser") {
				remoteuser = it.value();
			} else if (it.key() == "remotepass") {
				remotepass = it.value();
			} else if (it.key() == "remotepkgdir") {
				remotepkgdir = it.value();
			} else if (it.key() == "daemonize") {
				daemon_flag = it.value();
			} else if (it.key() == "remotemeta") {
				pkgfileremotename = it.value();
			} else if (it.key() == "localmeta") {
				pkgfilelocalname = it.value();
			} else {
				cout << "unknown json object: " << it.key() << endl;
			}
		}
	} else {
		pr_info("use all default config\n");
	}
	pkgfilelocal = localdir + pkgfilelocalname;

	if (daemon_flag) {
		daemon(1,0);
	}
	// dump configs
	/*
	pr_info("dump configs: defconf:%s, pkgfileremotename:%s, pkgfilelocalname:%s,"
			"remoteaddr:%s,remoteuser:%s,remotepass:%s,remotepkgdir:%s,daemon_flag:%d\n",
			defconf.c_str(), pkgfileremotename.c_str(), pkgfilelocalname.c_str(),
			remoteaddr.c_str(), remoteuser.c_str(), remotepass.c_str(), remotepkgdir.c_str(),
			daemon_flag);
	*/
	return 0;
}
