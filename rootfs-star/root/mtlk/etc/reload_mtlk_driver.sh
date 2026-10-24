#!/bin/sh


# Run this script to reload the metalink driver, without rebooting.


#echo "Start Reload process"


# Get the single-band / dual-band mode, 
# according to the number of wls interfaces on the PCI bus.
# Currently, there is no dual-band STA, so only allow dual band in AP mode, 
# (even if the HW supports dual band).


#workaround to clean flag (mtlk_init_wls,sh)
if [ -e /tmp/init_done ]; then rm /tmp/init_done; fi


if [ -z "$NETWORK_TYPE" ]
then
	NETWORK_TYPE=`awk -F "=" '/^network_type/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/wlan0.conf`
fi

WLAN_NUM_BANDS=1
export WLAN_NUM_BANDS

echo kill drvhlpr
killall drvhlpr 2>/dev/null || true
usleep 500000 2>/dev/null || sleep 1
A=`ps | grep drvhlpr | grep -v grep | awk '{print $1}'`
	for a in $A ; do kill -9 $a 2>/dev/null || true ; done
	
echo kill WPS_PBC
killall WPS_PBC.sh 2>/dev/null || true
A=`ps | grep WPS_PBC.sh | grep -v grep | awk '{print $1}'`
	for a in $A ; do kill $a 2>/dev/null || true ; done
	usleep 500000 2>/dev/null || sleep 1
	A=`ps | grep WPS_PBC.sh | grep -v grep | awk '{print $1}'`
	for a in $A ; do kill -9 $a 2>/dev/null || true ; done
	rm -rf /tmp/WPS_PBC_monitor.lock 2>/dev/null || true
	
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

# wsccmd needs to be killed properly (no -9) in order to clean up properly.	
echo kill WPS
killall wsccmd 2>/dev/null || true
usleep 500000 2>/dev/null || sleep 1
A=`ps | grep wsccmd | grep -v grep | awk '{print $1}'`
for a in $A ; do kill -9 $a 2>/dev/null || true ; done
#Delete fifo
for A in /tmp/WPS*; do
	[ -e "$A" ] || continue
	[ "$A" = /tmp/WPS_DEVICE_PIN ] && continue
	rm -rf "$A"
done

#kill mtlk_wps_msgloop.tcl process
A=`ps | grep mtlk_wps_msgloop.tcl | grep -v grep | awk '{print $1}'`
	for a in $A ; do kill $a 2>/dev/null || true ; done
	usleep 500000 2>/dev/null || sleep 1
A=`ps | grep mtlk_wps_msgloop.tcl | grep -v grep | awk '{print $1}'`
	for a in $A ; do kill -9 $a 2>/dev/null || true ; done

# hostapd needs to be killed properly (no -9) in order to clean up properly.	
echo kill hostapd
killall hostapd 2>/dev/null || true
usleep 500000 2>/dev/null || sleep 1
A=`ps | grep hostapd | grep -v grep | awk '{print $1}'`
 for a in $A ; do kill -9 $a 2>/dev/null || true ; done

# supplicant needs to be killed properly (no -9) in order to clean up properly.	
echo kill supplicant
killall wpa_supplicant 2>/dev/null || true
usleep 500000 2>/dev/null || sleep 1
A=`ps | grep wpa_supplicant | grep -v grep | awk '{print $1}'`
 for a in $A ; do kill -9 $a 2>/dev/null || true ; done

# Always bring interface down before rmmod
ifconfig wlan0 down 2>/dev/null || true

#Remove bridge interface
if [ ! $BRIDGE_MODE ]
then
	BRIDGE_MODE=`awk -F "=" '/^BridgeMode/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf 2>/dev/null`
fi

if [ "$BRIDGE_MODE" != "0" ]
then
  echo bridge mode, delete interface
  brctl delif br0 wlan0 2>/dev/null || true
fi


echo Removing mtlk

#delete init_failed if exist
if [ -f /tmp/init_failed ]; then rm /tmp/init_failed ; fi

#remove driver
if lsmod | grep -q mtlk; then
	count=0
	while lsmod | grep -q mtlk; do
		count=`expr $count + 1`
		rmmod mtlk 2>/dev/null
		if ! lsmod | grep -q mtlk; then
			break
		fi
		if [ $count -ge 10 ]; then
			echo failed to remove mtlk > /tmp/init_failed
			break
		fi
		usleep 500000 2>/dev/null || sleep 1
	done
	echo count=$count
fi


# Source platform-specific settings
#echo $MOD_EXT
#delete init_failed if exist


#cd /mnt/jffs2/etc
. ./mtlk_init_platform.sh

echo Starting with mtlk_init_wls.sh
if [ ! -e /tmp/init_failed ]; then (. ./mtlk_init_wls.sh); fi

if [ ! -e /tmp/init_failed ]; then (. ./mtlk_init_route.sh 1); fi


if [ ! -e /tmp/init_failed ]; then (. ./mtlk_init_wls_apps.sh); fi

#echo message log handling
# Save the head of the message log (useful for debugging when there were many msgs and the log was rotated)
#./mtlk_save_log.sh &

# Run drvhlpr script
if [ ! -e /tmp/init_failed ]
then
	DRVHLPR_COUNT=`ps | grep drvhlpr | grep -c -v grep`
	if [ $DRVHLPR_COUNT = 0 ]
	then
		echo call drvhlpr
		(. ./mtlk_drvhlpr.sh &);
	fi
fi
if [ ! -e /tmp/init_failed ]; then (. ./mtlk_init_route.sh 2); fi

#echo Dump errors
# Dump errors (seen if the user has a serial interface)
if [ -e /tmp/init_failed ]; then echo -e "\n\nInit failed, due to error:"; cat /tmp/init_failed; fi

# raise the init-done flag (used by dhcp and wps for syncing)
echo 1 > /tmp/init_done

echo "Reload process has done" 

