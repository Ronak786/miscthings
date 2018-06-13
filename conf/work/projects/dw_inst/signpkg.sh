#########################################################################
# File Name: makepkg.sh
# Author: davee-x
# mail: davee.naughty@gmail.com
# Created Time: Mon 11 Jun 2018 02:01:25 PM CST
#########################################################################
#!/bin/bash

dirname=$1

if [ "x$dirname" == "x" ]; then
	echo "need dir name as parameter"
	exit 1
fi

LD_LIBRARY_PATH=.. ../createsig ${dirname}.tar.gz ${dirname}.tar.gz.sig
