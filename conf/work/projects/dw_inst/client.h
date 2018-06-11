/*************************************************************************
	> File Name: client.h
	> Author: davee-x
	> Mail: davee.naughty@gmail.com 
	> Created Time: 2018年06月10日 星期日 20时01分23秒
 ************************************************************************/


#ifndef CLIENT_H
#define CLIENT_H

#include "pkghandle.h"
#include "pkginfo.h"
#include <vector>
#include <string>

using namespace std;

void init_handle(PkgHandle &hdl);
bool get_and_check(PkgHandle &hdl);
void dumppkgs(PkgHandle hdl);
void showInfo(PkgInfo &pkg);
void updatepkgs(PkgHandle hdl);
void compare_and_list_new(const vector<PkgInfo> vremote, vector<PkgInfo> &vnew);
void install_and_updatelocal(PkgHandle hdl);

#endif
