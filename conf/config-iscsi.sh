#!/bin/sh
# create: filename create targetname   size(in gb) wb
# delete: filename delete targetname  
# recovery: filename recovery 
# unlink:   filename unlink
# load  :   filename load
# rdconfig:  filename rdconfig

#check requeired mods and files   
#errno: need to fill
#		1  cmdline param not enough
#		2  required bin and modules not found
#		3  action not correct
#		4  lun already exist

#set -e
#set -x
#set -o xtrace
FILEIO=fileio_tgt
TARGPATH="/sys/kernel/scst_tgt/targets/iscsi"
STOREPATH="/tmp/scst"
TARGPFX="iqn.2016-01.com.lwstore"
#should be implicit file starting with "."
CFILE="/tmp/tmpconf"
TFILE="/tmp/tmpconf.tmp"

check_exec()
{
	/sbin/modinfo -F filename iscsi_scst > /dev/null 2>&1  && \
	/sbin/modinfo -F filename scst_user  > /dev/null 2>&1 && \
	/sbin/modinfo -F filename scst  > /dev/null 2>&1
	if [ $? -ne 0 ]; then
		exit 2
	fi
	which iscsi-scstd > /dev/null 2>&1  && \
	which fileio_tgt  > /dev/null 2>&1  
	if [ $? -ne 0 ]; then
		exit 2
	fi
}

load_mod()
{
	modprobe scst_user >/dev/null 2>&1 
	modprobe iscsi-scst >/dev/null 2>&1 
	iscsi-scstd >/dev/null 2>&1 
}

get_block_file_pos()
{
	cat $CFILE | while read pos
	do
		local name="$(echo $pos | gawk '{print $1}')"
		if [ "$1" == "$name" ]; then
			echo "$(echo $pos | gawk '{print $4}')"
		fi
	done
}

do_create_iscsi()
{
	targname="${TARGPFX}:$1"
	targname_path="${TARGPFX}\:$1"
	lunpath="${TARGPATH}/${targname}/luns/mgmt"
	
	if [ -e "${3}/${1}_${1}" ]; then
		echo "already exist"
		return 4
	fi
	truncate -s ${2}g  ${3}/${1}_${1}
	nohup $FILEIO  $1 ${3}/${1}_${1} >> ${ISCSILOG}/${1} 2>&1 &
	echo "add_target $targname" > "${TARGPATH}/mgmt"
	usleep 1000000
	echo "add $1 0" > "$lunpath"
	echo 1 > "${TARGPATH}/${targname}/enabled"
	echo 1 > "${TARGPATH}/enabled"
}

do_recovery_iscsi()
{
	targname="${TARGPFX}:$1"
	targname_path="${TARGPFX}\:$1"
	lunpath="${TARGPATH}/${targname}/luns/mgmt"
	
	if [ ! -e "${2}/${1}_${1}" ]; then
		echo "not exist, can not restore $1"
		return 4
	fi
	nohup $FILEIO  $1 ${2}/${1}_${1} >> ${ISCSILOG}/${1} 2>&1 &
	echo "add_target $targname" > "${TARGPATH}/mgmt"
	usleep 1000000
	echo "add $1 0" > "$lunpath"
	echo 1 > "${TARGPATH}/${targname}/enabled"
	echo 1 > "${TARGPATH}/enabled"
}

do_delete_iscsi()
{
	targtail=$1
	targname="${TARGPFX}:${targtail}"
	lunname="$1"
	lunpath="${TARGPATH}/${targname}/luns/mgmt"
	lundev="${TARGPATH}/${targname}/luns/0"
	targdev="${TARGPATH}/${targname}"
	
	ls "$lundev" >/dev/null 2>&1  && echo "del 0" > "$lunpath" 
	usleep 1000000
	ls "$targdev" >/dev/null 2>&1 && echo "del_target $targname" > "${TARGPATH}/mgmt" 2>/dev/null
	pid=$(ps aux | grep $FILEIO | grep ${targtail}_${targtail}  | awk '{print $2}')
	if [ "n$pid" != "n" ]; then
		kill $pid > /dev/null 2>&1
	fi
}

do_unlink_iscsi()
{
	targtail=$1
	targname="${TARGPFX}:${targtail}"
	lunname="$1"
	lunpath="${TARGPATH}/${targname}/luns/mgmt"
	lundev="${TARGPATH}/${targname}/luns/0"
	targdev="${TARGPATH}/${targname}"
	
	ls "$lundev" >/dev/null 2>&1  && echo "del 0" > "$lunpath" 
	usleep 1000000
	ls "$targdev" >/dev/null 2>&1 && echo "del_target $targname" > "${TARGPATH}/mgmt" 2>/dev/null
	pid=$(ps aux | grep $FILEIO | grep ${targtail}_${targtail}  | awk '{print $2}')
	if [ "n$pid" != "n" ]; then
		kill $pid > /dev/null 2>&1
	fi
}


do_remove_entry()
{
	FILEDIR=$(get_block_file_pos ${1})
	rm -f $FILEDIR/${1}_${1} >/dev/null 2>&1
	cat $CFILE | while read line
	do
		if [ "$1" != "$(echo $line | gawk '{print $1}')" ]
		then
			echo "$line" >> $TFILE
		fi
	done 
	rm -rf $CFILE 2>&1
	cp $TFILE $CFILE
	rm -f $TFILE
	touch $TFILE
}

do_read_config()
{
	cat $CFILE	2>&1
}

if [ $# -lt 1 ]; then
	echo "not enough param"
	exit 1
fi

#should add some delete after fail create
#	and fail delete should remove force
ACTION="$1"
ISCSILOG="/var/log/iscsi_log"
mkdir -p "$ISCSILOG"
[ ! -e $CFILE ] && touch $CFILE 
[ ! -e $TFILE ] && touch $TFILE 
case $ACTION in 
"load" )
	load_mod
	;;
"create" )
	if [ "$#" != "5" ]; then
		echo "not enough param  should 3"
		exit 1
	fi
	do_create_iscsi $2 $3 $5
	echo "$2 wb $3 $5" >> "$CFILE"
	;;
"recovery" )
	cat $CFILE | while read recv_line
	do
		name=$(echo $recv_line | gawk '{print $1}')
		FILEDIR=$(get_block_file_pos $name)
		do_recovery_iscsi $name  $FILEDIR
	done
	;;
"delete" )
	if [ "$#" != "2" ]; then
		echo "not enough param  should 2"
		exit 1
	fi
	do_delete_iscsi $2  
	do_remove_entry $2
	;;
"unlink" )
	cat $CFILE | while read unlink_line
	do
		do_unlink_iscsi $unlink_line
	done
	;;
"help" )
	echo "$0 create targetname size wb absdir     create a new target"
	echo "$0 delete targetname		      delete an exist target"
	echo "$0 rdconfig			    get info about existing targets"
	echo "$0 recovery			    activate all target after bootup"
	echo "$0 unlink				    deactivate all target"
	echo "$0 load				    load modules and start iscsi service(do this after bootup"
	echo "$0 help				    show this help list"
	;;
"rdconfig" )
	do_read_config
	;;
* )
	echo "action not recognized, see  try \"$0 help\""
	exit 3
	;;
esac
