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

	printf("begin load config\n");
	load_config(ac, av);

	bool stop = false;

	printf("begin loop\n");
	while (!stop) {
		PkgHandle hdl;
		printf("begin init\n");
		if (init_handle(hdl) == -1) {
			printf("fail init\n");
			return -1;
		}
		printf("begin check\n");
		if (get_and_check(hdl)) {
			printf("we have new pkgs\n");
			updatepkgs(hdl);
		}
		uninit_handle(hdl);
		sleep(5);
	}
	return 0;
}
