#########################################################################
# File Name: makejson.sh
# Author: davee-x
# mail: davee.naughty@gmail.com
# Created Time: Mon 11 Jun 2018 09:46:50 AM CST
#########################################################################
#!/bin/bash

param=$1
if [ "x$param" == "x" ]; then
	echo "param: local remote all"
	exit 1
fi

if [ "$param" == "local" ]; then
	./input localpkg.txt localpkgs/localpkg.json
elif [ "$param" == "remote" ]; then
	./input remotepkg.txt remotepkgs/remotepkg.json
elif [ "$param" == "all" ]; then
	./input localpkg.txt localpkgs/localpkg.json
	./input remotepkg.txt remotepkgs/remotepkg.json
else
	echo "param wrong"
fi
