#include <cstdio>
#include <cstring>
#include <fstream>
#include <vector>

#ifdef __cplusplus
extern "C" {
#endif
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#ifdef __cplusplus
}
#endif

#include "pkgutil.h"
#include "debug.h"


int do_copy_file(std::string from, std::string to) {
    unlink(to.c_str());
    std::ifstream ifs(from);
    std::ofstream ofs(to);
    if (!ifs || !ofs) {
        pr_info("do_copy_file can't get dir of copy file ifs or ofs\n");
        return -1;
    }
    pr_info("copy file from %s to %s\n", from.c_str(), to.c_str());
    ofs << ifs.rdbuf();
    return 0;
}

int do_copy_pkg(std::string pkgpath, std::string installdir) {
    std::ifstream ifslist(pkgpath + "/FILELIST.lst");
    std::string pathline;
    if (!ifslist) {
        pr_info("can not copy package path %s\n", pkgpath.c_str());
        return -1;
    }
    while (std::getline(ifslist, pathline)) {
        if (pathline[pathline.length()-1] == '/') {
            std::string tmpdir = installdir + pathline;
            pr_info("makedir: %s%s\n", installdir.c_str(), pathline.c_str());
            mkdir(tmpdir.c_str(), 0775); // make dir, we use find(1) make sure dir created before copy file
        } else {
            do_copy_file(pkgpath + "/src/" + pathline, installdir + pathline);
        }
    }
    ifslist.close();
    return 0;
}

int uninstallpkg(std::string pkgpath, std::string installdir) {
    std::ifstream ifslist(pkgpath + "/FILELIST.lst");
    if (!ifslist) {
        pr_info("can not open file and uninstall, may be first install\n");
        return 0;
    }

    std::vector<std::string> reverselines;
    std::string pathline;
    while (std::getline(ifslist, pathline)) {
        if (pathline[pathline.length()-1] != '/') {
            std::string tmpdir = installdir + pathline;
            unlink(tmpdir.c_str());
            pr_info("unlink: %s\n", tmpdir.c_str());
        } else {
            reverselines.push_back(pathline); // push all dirs in reverse order
        }
    }

    // remove dirs if empty
    for (auto i = reverselines.rbegin(); i != reverselines.rend(); ++i) {
        std::string tmpdir = installdir + *i;
        pr_info("remove dir");
        rmdir(tmpdir.c_str());
    }
    return 0;
}
