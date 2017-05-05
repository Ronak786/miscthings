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
TARGPFX="iqn.2016-01.com.lwstore"
CFILE="/etc/swgfs/iscsiconf"
TFILE="/etc/swgfs/.iscsiconf.in"
ALLOWFILE="/etc/initiators.allow"
TALLOWFILE="/etc/.initiators.allow.in"
DENYFILE="/etc/initiators.deny"
TDENYFILE="/etc/.initiators.deny.in"
DESCFILE="/etc/swgfs/descconf"
TDESCFILE="/etc/swgfs/.descconf"
export ERRFILE="/tmp/.error"

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
	exist=$(ps aux | grep iscsi-scstd | grep -v grep)
	if [ -z "$exist" ]; then 
		iscsi-scstd >/dev/null 2>&1 
	fi
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

check_ip ()
{
	res=$(echo $1 | egrep  '^([0-9]{1,3}\.){3}[0-9]{1,3}(,([0-9]{1,3}\.){3}[0-9]{1,3})*$')
	if [ "x$res" == "x" ]; then
		return 0
	else
		return 1
	fi
}

add_to_allow_file()
{
	echo "$2 $1" >> $ALLOWFILE
	echo "$2 ALL" >> $DENYFILE
}

del_from_allow_file()
{
	cat $ALLOWFILE | while read line
	do
		if [ "$1" != "$(echo $line | gawk '{print $1}')" ]
		then
			echo "$line" >> $TALLOWFILE
		fi
	done 
	cat $DENYFILE | while read line
	do
		if [ "$1" != "$(echo $line | gawk '{print $1}')" ]
		then
			echo "$line" >> $TDENYFILE
		fi
	done 
	rm -rf $ALLOWFILE 2>&1
	cp $TALLOWFILE $ALLOWFILE
	rm -f $TALLOWFILE
	touch $ALLOWFILE
	rm -rf $DENYFILE 2>&1
	cp $TDENYFILE $DENYFILE
	rm -f $TDENYFILE
	touch $DENYFILE
}

add_to_desc_file()
{
        echo "$1 $2" >> $DESCFILE
}

del_from_desc_file()
{
        cat $DESCFILE | while read line
        do
                if [ "$1" != "$(echo $line | gawk '{print $1}')" ]
                then
                        echo "$line" >> $TDESCFILE
                fi
        done
        rm -rf $DESCFILE 2>&1
        mv $TDESCFILE $DESCFILE
}

do_create_iscsi()
{
	targname="${TARGPFX}:$1"
	targname_path="${TARGPFX}\:$1"
	lunpath="${TARGPATH}/${targname}/luns/mgmt"
	
	if [ "$1" != "$(echo $1 | egrep '^[a-zA-Z0-9]*$')" ]; then
		echo "iscsi name only allow characters [a-zA-Z0-9]"
		exit
	fi
	res=$(echo "${2}" | egrep '^[1-9][0-9]*$')
	if [ "x$res" == "x" ]; then
		echo "size invalid"
		exit
	fi
	truncate -s ${2}g  ${3}/Iscsiblk_${1}_${1}
	nohup $FILEIO  $1 ${3}/Iscsiblk_${1}_${1} >> ${ISCSILOG}/${1} 2>&1 &
	echo "add_target $targname" > "${TARGPATH}/mgmt"
	usleep 1000000
	echo "add $1 0" > "$lunpath"
	echo 1 > "${TARGPATH}/${targname}/enabled"
	echo 1 > "${TARGPATH}/enabled"
	if [ ! "x$4" == "x" ] ; then
		ipaddr=$4
		check_ip $ipaddr
		res=$?
		if [ "$res" != "0" ]; then
			add_to_allow_file $ipaddr $targname
			if [ "x$5" != "x" ];then #has description
                                add_to_desc_file $1 "$5"
                        fi
                else
                        add_to_desc_file $1 "$4"
                fi
	fi
}

do_recovery_iscsi()
{
	targname="${TARGPFX}:$1"
	targname_path="${TARGPFX}\:$1"
	lunpath="${TARGPATH}/${targname}/luns/mgmt"
	
	if [ ! -e "${2}/Iscsiblk_${1}_${1}" ]; then
		echo "not exist, can not restore $1"
		return 4
	fi
	nohup $FILEIO  $1 ${2}/Iscsiblk_${1}_${1} >> ${ISCSILOG}/${1} 2>&1 &
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
	del_from_allow_file $targname
	del_from_desc_file $1
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
	rm -f $FILEDIR/Iscsiblk_${1}_${1} >/dev/null 2>&1
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
	cat $CFILE | while read line
        do
		ipaddr=""	
		tgname=$(echo $line | gawk '{print $1}')
		listdir="${TARGPATH}/${TARGPFX}:${tgname}/sessions"
		if [  -e $listdir ]; then
			for tmpaddr in $(ls -R1  $listdir | egrep '^([0-9]{1,3}\.){3}[0-9]{1,3}$' | sort | uniq)
			do
				if [ "x$ipaddr" == "x" ]; then
					ipaddr=${tmpaddr}
				else
					ipaddr="${tmpaddr},${ipaddr}"
				fi
			done
		fi

		name=${TARGPFX}:${tgname}
		restrict=$(cat $ALLOWFILE | egrep "^$name " | cut -d ' ' -f 2)
		desc=$(cat $DESCFILE | egrep "^$tgname " | cut -d ' ' -f 2- )
		if [ "x$restrict" == "x" ]; then
			restrict="default"
		fi
		if [ "x$ipaddr" == "x" ]; then
			ipaddr="default"
		fi
		if [ "x$desc" == "x" ]; then
			desc="default"
		fi
		echo $line $restrict $ipaddr $desc
        done
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
[ ! -e $ALLOWFILE ] && touch $ALLOWFILE 
[ ! -e $TALLOWFILE ] && touch $TALLOWFILE 
[ ! -e $DENYFILE ] && touch $DENYFILE 
[ ! -e $TDENYFILE ] && touch $TDENYFILE 
[ ! -e $DESCFILE ] && touch $DESCFILE 
[ ! -e $TDESCFILE ] && touch $TDESCFILE 
touch $ERRFILE
case $ACTION in 
"load" )
	load_mod
	;;
"create" )
	if [ "$#" != "5" -a "$#" != "6" -a "$#" != 7 ]; then
		echo "not enough param  should be 5 or 6 or 7 params"
		exit 1
	fi
	echo "right" > $ERRFILE
	cat $CFILE | while read ex_line
	do
		line_name=$(echo $ex_line | gawk '{print $1}')
		if [ "$2" == "${line_name}" ]; then
			echo "${line_name} alreadly exist, exiting"
			echo "exist" > $ERRFILE
			exit 
		fi
	done 
	error=$(cat $ERRFILE)
	if [ "$error" == "exist" ];then
		exit
	fi
	do_create_iscsi $2 $3 $5 "$6" "$7"
	echo "$2 wb $3 $5" >> "$CFILE"
	;;
"recovery" )
	cat $CFILE | while read recv_line
	do
		name=$(echo $recv_line | gawk '{print $1}')
		FILEDIR=$(get_block_file_pos $name)
		exist=$(ps aux | grep fileio_tgt | grep ${name}_${name} | grep -v grep)
		if [ -z "$exist" ]; then
			do_recovery_iscsi $name  $FILEDIR
		fi
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
