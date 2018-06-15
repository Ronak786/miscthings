/*************************************************************************
	> File Name: client.h
	> Author: davee-x
	> Mail: davee.naughty@gmail.com 
	> Created Time: 2018年06月10日 星期日 20时01分23秒
 ************************************************************************/


#ifndef PKGMGNT_H
#define PKGMGNT_H

#include "pkghandle.h"
#include "pkginfo.h"

int init_handle(PkgHandle &hdl);
void uninit_handle(PkgHandle &hdl);
int load_config(int ac, char *av[]);
bool get_and_check(PkgHandle &hdl);
const std::vector<PkgInfo>& getnewpkglist(PkgHandle &hdl);
int updatepkgs(PkgHandle &hdl);

#endif
