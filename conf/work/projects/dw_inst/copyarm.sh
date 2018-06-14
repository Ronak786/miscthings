#########################################################################
# File Name: copyarm.sh
# Author: davee-x
# mail: davee.naughty@gmail.com
# Created Time: Thu 14 Jun 2018 10:49:00 AM CST
#########################################################################
#!/bin/bash

rm -f arm/*
/bin/cp libsigutil.so arm/
/bin/cp libpkglib.so arm/
/bin/cp pkghandle.h arm/
/bin/cp pkginfo.h arm/
/bin/cp pkgmgnt.h arm/
/bin/cp pkgmgnt  arm/
/bin/cp localpkgs/pubkey.pem arm/
/bin/cp config.json  arm/
