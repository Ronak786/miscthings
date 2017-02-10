#!/bin/sh
# create: filename create targetname   size(in gb)
# delete: filename delete targetname  

#check requeired mods and files   
#errno: need to fill
#		1  cmdline param not enough
#		2  required bin and modules not found
#		3  action not correct
#		4  lun already exist

set -e
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

do_create_iscsi()
{
	targname="${TARGPFX}:$1"
	targname_path="${TARGPFX}\:$1"
	lunpath="${TARGPATH}/${targname}/luns/mgmt"
	
	if [ -e "${STOREPATH}/${1}_${1}" ]; then
		echo "already exist"
		exit 4
	fi
	truncate -s ${2}g ${STOREPATH}/${1}_${1}
	nohup $FILEIO  $1 $STOREPATH/${1}_${1} >> ${ISCSILOG}/${1} 2>&1 &
	echo "add_target $targname" > "${TARGPATH}/mgmt"
	usleep 100000
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
	
	if [ ! -e "${STOREPATH}/${targtail}_${lunname}" ]; then
		echo "not exist nothing to del"
		exit 4
	fi
	rm -f ${STOREPATH}/$1
	echo "del 0" > "$lunpath"
	usleep 100000
	echo "del_target $targname" > "${TARGPATH}/mgmt"
	pid=$(ps aux | grep $FILEIO | grep ${targtail}_${targtail}  | awk '{print $2}')
	kill $pid
	rm -f $STOREPATH/${lunname}_${lunname}
}

do_remove_entry()
{
	mv "$CFILE"  "$TFILE"
	for line in $( cat $TFILE )
	do
		if [ "$1" != "$(gawk '{print $1}' $line)" ]
		then
			echo "$line" >> $CFILE
		fi
	done
	rm -rf $TFILE 2>&1
}


do_read_config()
{
	cat $CFILE	2>&1
}

if [ $# -lt 1 ]; then
	echo "not enough param"
	exit 1
fi

#check if required modules and execs are installed
#check_exec

#should add some delete after fail create
#	and fail delete should remove force
ACTION="$1"
ISCSILOG="/var/log/iscsi_log"
mkdir -p "$ISCSILOG"
touch $CFILE
case $ACTION in 
"create" )
	if [ "$#" != "3" ]; then
		echo "not enough param  should 3"
		exit 1
	fi
	do_create_iscsi $2 $3 
	echo "$2 $3" >> "$CFILE"
	;;
"delete" )
	if [ "$#" != "2" ]; then
		echo "not enough param  should 2"
		exit 1
	fi
	do_delete_iscsi $2  
	do_remove_entry $2
	;;
"help" )
	echo "$0 create targetname size"
	echo "$0 delete targetname"
	echo "$0 rdconfig"
	;;
"rdconfig" )
	do_read_config
	;;
* )
echo "action not right"
	exit 3
esac
