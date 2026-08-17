#!/bin/sh

killall drvhlpr
A=`ps | grep drvhlpr | grep -v grep | awk '{print $1}'`
	for a in $A ; do kill -9 $a ; done

killall WPS_PBC.sh
A=`ps | grep WPS_PBC.sh | grep -v grep | awk '{print $1}'`
	for a in $A ; do kill -9 $a ; done

#kill process waiting on gpio	
B=`awk -F "=" '/^WPS_PB/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/HW.ini`
A=`ps | grep "cat ${B}" | grep -v grep | awk '{print $1}'`
	for a in $A ; do kill -9 $a ; done
	
killall wsccmd
A=`ps | grep wsccmd | grep -v grep | awk '{print $1}'`
for a in $A ; do kill -9 $a ; done

#kill mtlk_wps_msgloop.tcl process
A=`ps | grep mtlk_wps_msgloop.tcl | grep -v grep | awk '{print $1}'`
	for a in $A ; do kill -9 $a ; done

#Delete fifo
A=`ls /tmp/WPS* | grep -v WPS_DEVICE_PIN`
rm $A

if [ -e /root/mtlk/etc ]
then
	apps_path="/root/mtlk/etc"
else 
	apps_path="/mnt/jffs2/etc"
fi

cd $apps_path

# Source platform-specific settings
. ./mtlk_init_platform.sh

#restart wsccmd
(. ./mtlk_init_wls_apps.sh);

#restart drvhlpr
(. ./mtlk_drvhlpr.sh &);

cd -


