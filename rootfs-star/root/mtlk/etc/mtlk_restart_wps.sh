#!/bin/sh

#workaround to clean flag (mtlk_init_wls,sh)
if [ -e /tmp/init_done ]; then rm /tmp/init_done; fi

killall drvhlpr
A=`ps | grep drvhlpr | grep -v grep | awk '{print $1}'`
	for a in $A ; do kill -9 $a ; done

killall WPS_PBC.sh 2>/dev/null || true
A=`ps | grep WPS_PBC.sh | grep -v grep | awk '{print $1}'`
	for a in $A ; do kill $a 2>/dev/null || true ; done
	sleep 1
	A=`ps | grep WPS_PBC.sh | grep -v grep | awk '{print $1}'`
	for a in $A ; do kill -9 $a 2>/dev/null || true ; done
	rm -rf /tmp/WPS_PBC_monitor.lock 2>/dev/null || true

#kill process waiting on gpio	
B=`awk -F "=" '/^WPS_PB/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/HW.ini`
case "$B" in
	''|null) B=/dev/gpio13 ;;
	*[!0-9]*) : ;;
	*) B=/dev/gpio$B ;;
esac
if [ -n "$B" ]; then
	A=`ps | grep "cat ${B}" | grep -v grep | awk '{print $1}'`
	for a in $A ; do kill -9 $a 2>/dev/null || true ; done
fi
	
killall wsccmd
A=`ps | grep wsccmd | grep -v grep | awk '{print $1}'`
for a in $A ; do kill -9 $a ; done

#kill mtlk_wps_msgloop.tcl process
A=`ps | grep mtlk_wps_msgloop.tcl | grep -v grep | awk '{print $1}'`
	for a in $A ; do kill -9 $a ; done

#Delete fifo
for A in /tmp/WPS*; do
	[ -e "$A" ] || continue
	[ "$A" = /tmp/WPS_DEVICE_PIN ] && continue
	rm -rf "$A"
done

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

# raise the init-done flag (used by dhcp and wps for syncing)
echo 1 > /tmp/init_done

