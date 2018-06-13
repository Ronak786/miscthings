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

int init_handle(PkgHandle &hdl);
void uninit_handle(PkgHandle &hdl);
bool get_and_check(PkgHandle &hdl);
void dumppkgs(PkgHandle hdl);
void showInfo(PkgInfo &pkg);
void updatepkgs(PkgHandle &hdl);
void compare_and_list_new(const vector<PkgInfo> vremote, vector<PkgInfo> &vnew);
int install_and_updatelocal(PkgHandle &hdl);
void do_copy_pkg(string pkgname);
void do_copy_file(string from, string to);
int download(string pkgfile);
bool extract_and_install(string pkgfile);
void uninstallpkg(string pkgfile);
bool checksig(string fname, string fsig);
void getpkglist(string filename, vector<PkgInfo> &vstr);
int load_config(int ac, char *av[]);

#endif
