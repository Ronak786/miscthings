#!/bin/sh
# $0 objname <in >out
objname=${1:-objname}
od -A n -v -t x1 | sed -e '1iconst unsigned char '$objname'[] = { 
s/\([0-9a-f][0-9a-f]\) */0x\1,/g
$s/,$//
$a\
};
'

