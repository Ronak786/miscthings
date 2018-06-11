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

using json = nlohmann::json;
string pkgfile = "localpkg.json";
string pkgfilelocal = "remotepkg.json";

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
