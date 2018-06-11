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

using json = nlohmann::json;
string pkgfilelocal = "localpkg.json";
string pkgfilelocalbak = "localpkg.json.bak";
string pkgfile = "remotepkg.json";
string pkgfilebak = "remotepkg.json.bak";

// install releated
string remotedir = "pkgs/";
string localdir = "localpkgs/";
string installdir = "destdir/";

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
		sleep(5);
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
	getpkglist(pkgfile, vremote);
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
	printf("name: %s, version %s, size %s\n", pkg.getName().c_str(), pkg.getVersion().c_str(), pkg.getSize().c_str());
}

void updatepkgs(PkgHandle hdl) {
	printf("updating\n");
	install_and_updatelocal(hdl);
	printf("end update\n");
}

void install_and_updatelocal(PkgHandle hdl) {
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
			printf("can not install pkg\n", item.getName().c_str());
			continue;
		}

		// update local meta info
		printf("process %s\n", item.getName().c_str());
		obj[item.getName().c_str()]["name"] = item.getName();
		obj[item.getName().c_str()]["version"] = item.getVersion();
		obj[item.getName().c_str()]["size"] = item.getSize();
		printf("install pkg %s success\n", item.getName().c_str());
	}
	unlink(pkgfilelocalbak.c_str());
	rename(pkgfilelocal.c_str(), pkgfilelocalbak.c_str());
	ofstream ofs(pkgfilelocal);
	ofs << obj;
}

void download(string pkgfile) {
	printf("download( just copy here\n");
	do_copy_file(pkgfile, remotedir, localdir);
}

void do_copy_file(string filename, string remotedir, string localdir) {
	ifstream ifs(remotedir + filename);
	ofstream ofs(localdir + filename);
	if (!ifs || !ofs) {
		printf("get dir of copy file ifs or ofs\n");
		exit(1);
	}
	ofs << ifs.rdbuf();	
}

void do_copy_file2(string from, string to) {
	ifstream ifs(from);
	ofstream ofs(to);
	if (!ifs || !ofs) {
		printf("get dir of copy file ifs or ofs\n");
		exit(1);
	}
	ofs << ifs.rdbuf();	
}

void do_copy_pkg(string pkgname) {
	ifstream ifslist(localdir + pkgname + "/" + FILELIST.lst);
	string pathline;
	if (!ifslist) {
		printf("can not copy package path %s\n", pkgname);
	}
	while (std::getline(ifslist, pathline)) {
		if (pathline[pathline.length()-1] != "/") {
//			mkdir(destdir + pathline, 0775); // make dir, we use find(1) make sure dir created before copy file
			cout << "makedir: " << destdir + pathline << endl;
		} else {
//			do_copy_file2(localdir + pkgname + "/src/" + pathline, destdir + pathline);
			cout << "do copy file2:" << localdir + pkgname + "/src/" + pathline << "to: " << destdir + pathline << endl;
		}
	}
}

bool extract_and_install(string pkgfile) {
	string localpath = localdir + pkgfile;
	char localbuf[100];
	sprintf(localbuf, "tar xf %s -C %s\n", localpath.c_str(), localdir.c_str());
	system(localbuf);
	do_copy_pkg(pkgfile);
}

// should use ifstream?
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
		cout << "get content: " <<  pkg["name"] <<  pkg["version"] << pkg["size"] << endl;
		vstr.push_back(PkgInfo(pkg["name"],pkg["version"],pkg["size"]));
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
