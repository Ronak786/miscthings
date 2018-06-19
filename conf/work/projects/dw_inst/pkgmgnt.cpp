/*************************************************************************
	> File Name: pkgmgnt.cpp
	> Author: davee-x
	> Mail: davee.naughty@gmail.com 
	> Created Time: Thu 14 Jun 2018 10:57:17 AM CST
 ************************************************************************/

#include "pkgmgnt.h"
#include <unistd.h>
#include <cstdio>

int main(int ac, char *av[]) {

	if (ac != 2) {
		printf("need to give a conf path\n");
		return 0;
	}

	PkgHandle handle;
	printf("begin load config\n");
	if (handle.loadConfig(av[1]) != 0) {
		printf("can not load config\n");
		return -1;
	}

	bool stop = false;

	printf("begin loop\n");
	while (!stop) {
		std::vector<PkgInfo> locallist, remotelist, newlist;
		printf("begin init\n");
		if (handle.init() != 0) {
			printf("init failed\n");
			break;
		}

		if (handle.getLocalpkglist(locallist) != 0) {
			printf("get local failed\n");
			break;
		}
		printf("\nlocalpkgs:\n");
		for (auto i: locallist) {
			printf("name: %s, version: %s\n", i.getName().c_str(), i.getVersion().c_str());
		}

		if (handle.getRemotepkglist(remotelist) != 0) {
			printf("get remote failed\n");
			break;
		}
		printf("\nremotepkgs:\n");
		for (auto i: remotelist) {
			printf("name: %s, version: %s\n", i.getName().c_str(), i.getVersion().c_str());
		}

		if (handle.getNewpkglist(newlist) != 0) {
			printf("get new failed\n");
			break;
		}
		printf("\nnewpkgs:\n");
		for (auto i: newlist) {
			printf("name: %s, version: %s\n", i.getName().c_str(), i.getVersion().c_str());
		}

		handle.uninit();
		sleep(5);
	}
	return 0;
}
