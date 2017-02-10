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
#echo "to truncate -s ${3}g ${STOREPATH}/${1}_${2}"
	nohup $FILEIO  $1 $STOREPATH/${1}_${1} >> ${ISCSILOG}/${1} 2>&1 &
	echo "add_target $targname" > "${TARGPATH}/mgmt"
#	cd "${TARGPATH}/${targname}"
	usleep 100000
	echo "add $1 0" > "$lunpath"
	echo 1 > "${TARGPATH}/${targname}/enabled"
	echo 1 > "${TARGPATH}/enabled"
#echo "targname :$targname:"
#echo "lunname :$lunname:"
#echo "lunpath :$lunpath:"
#echo "mgmt path :${TARGPATH}/mgmt:"
#echo "enable path :${TARGPATH}/${targname}/enabled:"
#echo "enable2 path :${TARGPATH}/enabled:"
}

do_delete_iscsi()
{
	#need add '\'
#	tmptargname=$1
#	targtail=${tmptargname#*:}
	targtail=$1
	targname="${TARGPFX}:${targtail}"
#	targname_path="${TARGPFX}\:${targtail}"
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
#echo "to delete ${STOREPATH}/${targtail}_${lunname}"
#echo "del lunpath :$lunpath:"
#echo "del_target :$targname:  :${TARGPATH}/mgmt:"
}

if [ $# -lt 1 ]; then
	echo "not enough param"
	exit 1
fi

#check if required modules and execs are installed
#check_exec

ACTION="$1"
ISCSILOG="/var/log/iscsi_log"
mkdir -p "$ISCSILOG"
case $ACTION in 
"create" )
	if [ "$#" != "3" ]; then
		echo "not enough param  should 3"
		exit 1
	fi
#echo "about to create"
	do_create_iscsi $2 $3 
	;;
"delete" )
	if [ "$#" != "2" ]; then
		echo "not enough param  should 2"
		exit 1
	fi
#echo "about to delete"
	do_delete_iscsi $2  
	;;
"help" )
	echo "$0 create|delele targetname size(only for create)"
	;;
"rdconfig" )
	do_read_config
	;;
* )
echo "action not right"
	exit 3
esac
