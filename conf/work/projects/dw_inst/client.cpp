/*************************************************************************
	> File Name: client.c
	> Author: davee-x
	> Mail: davee.naughty@gmail.com 
	> Created Time: 2018年06月10日 星期日 19时28分13秒
 ************************************************************************/

#include "client.h"
#include "pkghandle.h"
#include "json.h"

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

using json = nlohmann::json;

// install releated
string remotedir = "pkgs/";
string localdir = "localpkgs/";
string installdir = "destdir/";

string pkgfilelocal = localdir + "localpkg.json";
string pkgfilelocalbak = localdir + "localpkg.json.bak";

string pkgfileremotename = "remotepkg.json";
string pkgfileremote = localdir + pkgfileremotename;
string pkgfileremotebak = localdir + "remotepkg.json.bak";

static void getpkglist(string filename, vector<PkgInfo> &vstr);

int main(int ac, char *av[]) {
	bool stop = false;
	PkgHandle hdl;
	init_handle(hdl);
	while (!stop) {
		printf("begin check\n");
		if (get_and_check(hdl)) {
			printf("we have new pkgs\n");
			dumppkgs(hdl);
			updatepkgs(hdl);
		}
		sleep(3);
	}
}

void init_handle(PkgHandle &hdl) {
	if (access(pkgfilelocal.c_str(), F_OK) != 0) {
		ofstream ofs(pkgfilelocal);
		json tmpobj = json({});
		ofs << tmpobj;
	}
	printf("inited \n");
}

// get a list of new pkgs needed to be download
// currently list is just one line 
bool get_and_check(PkgHandle &hdl) {
	vector<PkgInfo> vremote, vnew;
//	download(pkgfileremotename);
	getpkglist(pkgfileremote, vremote);
	compare_and_list_new(vremote, vnew);
	if (!vnew.empty()) {
		hdl.set_pkglist(vnew);
		return true;
	} else {
		return false;
	}
}

void dumppkgs(PkgHandle hdl) {
	for(auto pkg: hdl.get_pkglist()) {
		showInfo(pkg);
	}
}

void showInfo(PkgInfo& pkg) {
	printf("name: %s, version %s, desc %s\n", pkg.getName().c_str(), pkg.getVersion().c_str(), pkg.getDesc().c_str());
}

void updatepkgs(PkgHandle &hdl) {
	printf("updating\n");
	install_and_updatelocal(hdl);
	printf("end update\n");
}

void install_and_updatelocal(PkgHandle &hdl) {
	vector<PkgInfo> vnew = hdl.get_pkglist();
	ifstream ifs(pkgfilelocal);
	if (!ifs) {
		printf("error occured when read local json\n");
		exit(1);
	}
	json obj;
	ifs >> obj;
	for(auto item: vnew) {
		// update packages
		string pkgfile = item.getName() + "-" + item.getVersion();
		download(pkgfile + ".tar.gz");
		if (!extract_and_install(pkgfile)) {
			printf("can not install pkg %s\n", item.getName().c_str());
			continue;
		}

		// update local meta info
		printf("process %s\n", item.getName().c_str());
		obj[item.getName().c_str()]["name"] = item.getName();
		obj[item.getName().c_str()]["version"] = item.getVersion();
		obj[item.getName().c_str()]["desc"] = item.getDesc();
		printf("install pkg %s success\n", item.getName().c_str());
	}
	unlink(pkgfilelocalbak.c_str());
	rename(pkgfilelocal.c_str(), pkgfilelocalbak.c_str());
	ofstream ofs(pkgfilelocal);
	ofs << obj;
}

// current: just copy from a remote dir
// need to be: request from remote ftp and save file to localdir
void download(string pkgfile) {
	printf("download( just copy here)\n");
	do_copy_file(pkgfile, remotedir, localdir);
}


void do_copy_file(string filename, string remotedir, string localdir) {
	ifstream ifs(remotedir + filename);
	ofstream ofs(localdir + filename);
	if (!ifs || !ofs) {
		printf("do_copy_file can't get dir of copy file ifs or ofs\n");
		exit(1);
	}
	ofs << ifs.rdbuf();	
}

void do_copy_file2(string from, string to) {
	unlink(to.c_str());
	ifstream ifs(from);
	ofstream ofs(to);
	if (!ifs || !ofs) {
		printf("do_copy_file2 can't get dir of copy file ifs or ofs\n");
		exit(1);
	}
	ofs << ifs.rdbuf();	
}

void do_copy_pkg(string pkgname) {
	ifstream ifslist(localdir + pkgname + "/FILELIST.lst");
	string pathline;
	if (!ifslist) {
		printf("can not copy package path %s\n", pkgname.c_str());
	}
	while (std::getline(ifslist, pathline)) {
		if (pathline[pathline.length()-1] == '/') {
			string tmpdir = installdir + pathline;
			cout << "makedir: " << installdir + pathline << endl;
			mkdir(tmpdir.c_str(), 0775); // make dir, we use find(1) make sure dir created before copy file
		} else {
			cout << "do copy file2: " << localdir + pkgname + "/src/" + pathline << " to: " << installdir + pathline << endl;
			do_copy_file2(localdir + pkgname + "/src/" + pathline, installdir + pathline);
		}
	}
	ifslist.close();
}

void uninstallpkg(string pkgname) {
	ifstream ifslist(localdir + pkgname + "/FILELIST.lst");
	if (!ifslist) {
		printf("can not open file for %s and uninstall, may be first install\n", pkgname.c_str());
	}

	string pathline;
	while (std::getline(ifslist, pathline)) {
		if (pathline[pathline.length()-1] != '/') {
			string tmpdir = installdir + pathline;
			unlink(tmpdir.c_str());
			cout << "unlink: " << tmpdir << endl;
		}
	}
}

bool extract_and_install(string pkgfile) {
	string localpath = localdir + pkgfile + ".tar.gz";
	string localpathdir = localdir + pkgfile;
	string localpathdirbak = localdir + pkgfile + "-bak";

	uninstallpkg(pkgfile);

	char localbuf[200];
	sprintf(localbuf, "rm -rf %s; /bin/mv  %s %s; tar xf %s -C %s\n", 
			localpathdirbak.c_str(), localpathdir.c_str(), localpathdirbak.c_str(), localpath.c_str(), localdir.c_str());
	system(localbuf);
	do_copy_pkg(pkgfile);
	unlink(localpath.c_str());
	return true;
}

void getpkglist(string file, vector<PkgInfo> &vstr) {
	ifstream ifs(file);
	if (!ifs) {
		printf("can not get meta file, just continue\n");
		return;
	}

	json content;
	ifs >> content;
	cout << "file " << file << endl;
	for (auto pkg: content) {
		cout << "get content: " <<  pkg["name"] <<  pkg["version"] << pkg["desc"] << endl;
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
			printf("added new pkg: %s\n", remote.getName().c_str());
			vnew.push_back(remote);
		}
	}
}
