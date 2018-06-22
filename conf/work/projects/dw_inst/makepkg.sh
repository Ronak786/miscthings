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

(cd $dirname && find .   |  grep -v FILELIST | xargs -n 1 /bin/ls -d | sed  -e '1,2d' -e 's#\./src##' > FILELIST.lst)
tar -zcf ${dirname}.tar.gz $dirname || rm -f ${dirname}.tar.gz
