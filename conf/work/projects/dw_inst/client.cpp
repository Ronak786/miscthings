/*************************************************************************
	> File Name: client.c
	> Author: davee-x
	> Mail: davee.naughty@gmail.com 
	> Created Time: 2018年06月10日 星期日 19时28分13秒
 ************************************************************************/

#include "client.h"
#include "pkghandle.h"

#include <unistd.h>
#include <algorithm>

#include <cstring>

string pkgfile = "/home/sora/tmp/pkgfile.txt";
string pkgfilelocal = "/home/sora/tmp/pkgfilelocal.txt";

static void getpkglist(string filename, vector<string> &vstr);

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
	vector<string> vremote, vnew;
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

void showInfo(string pkg) {
	printf("name: %s\n", pkg.c_str());
}

void updatepkgs(PkgHandle hdl) {
	printf("updating\n");
}

void getpkglist(string file, vector<string> &vstr) {
	FILE *fp = fopen(file.c_str(), "ro");
	char tmpname[100];
	if (fp == NULL) {
		printf("can not get meta file, just continue\n");
		return;
	}
	while (fgets(tmpname, 100, fp)) {
		if (strlen(tmpname) > 0) {
			if (tmpname[strlen(tmpname)-1] == '\n') {
				tmpname[strlen(tmpname)-1] = '\0';
			}
			vstr.push_back(tmpname);
		}
	}
}

//vremote: contain downloaded list
//vnew: return list of pkgs needed to be downloade
void compare_and_list_new(const vector<string> vremote, vector<string> &vnew) {
	vector<string> vlocal;
	getpkglist(pkgfilelocal, vlocal);
	for (auto remote: vremote) {
		if (find(vlocal.begin(), vlocal.end(), remote) == vlocal.end()) {
			printf("added new pkg: %s\n", remote.c_str());
			vnew.push_back(remote);
		}
	}
}
