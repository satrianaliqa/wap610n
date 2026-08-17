#!/bin/sh


# Run this script to reload the metalink driver, without rebooting.


#echo "Start Reload process"


# Get the single-band / dual-band mode, 
# according to the number of wls interfaces on the PCI bus.
# Currently, there is no dual-band STA, so only allow dual band in AP mode, 
# (even if the HW supports dual band).


#workaround to clean flag (mtlk_init_wls,sh)
if [ -e /tmp/init_done ]; then rm /tmp/init_done; fi


if [ ! $NETWORK_TYPE ]
then
	NETWORK_TYPE=`awk -F "=" '/^network_type/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/wlan0.conf`
fi

WLAN_NUM_BANDS=1
export WLAN_NUM_BANDS

echo kill drvhlpr
killall drvhlpr
A=`ps | grep drvhlpr | grep -v grep | awk '{print $1}'`
	for a in $A ; do kill -9 $a ; done
	
echo kill WPS_PBC
killall WPS_PBC.sh
A=`ps | grep WPS_PBC.sh | grep -v grep | awk '{print $1}'`
	for a in $A ; do kill -9 $a ; done
	
B=`awk -F "=" '/^WPS_PB/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/HW.ini`
A=`ps | grep "cat ${B}" | grep -v grep | awk '{print $1}'`
	for a in $A ; do kill -9 $a ; done

# wsccmd needs to be killed properly (no -9) in order to clean up properly.	
echo kill WPS
killall wsccmd
A=`ps | grep wsccmd | grep -v grep | awk '{print $1}'`
for a in $A ; do kill -9 $a ; done
#Delete fifo
A=`ls /tmp/WPS* | grep -v WPS_DEVICE_PIN`
rm $A

# hostapd needs to be killed properly (no -9) in order to clean up properly.	
echo kill hostapd
killall hostapd
A=`ps | grep hostapd | grep -v grep | awk '{print $1}'`
 for a in $A ; do kill -9 $a ; done

# supplicant needs to be killed properly (no -9) in order to clean up properly.	
echo kill supplicant
killall wpa_supplicant
A=`ps | grep wpa_supplicant | grep -v grep | awk '{print $1}'`
 for a in $A ; do kill -9 $a ; done

#Remove bridge interface
if [ ! $BRIDGE_MODE ]
then
	BRIDGE_MODE=`awk -F "=" '/^BridgeMode/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`
fi

if [ $BRIDGE_MODE != 0 ]
then
  echo bridge mode, delete interface
  ifconfig wlan0 down
  brctl delif br0 wlan0
fi


echo Removing mtlk

#delete init_failed if exist
if [ -f /tmp/init_failed ]; then rm /tmp/init_failed ; fi

#rmmod led
#sleep 5

#remove driver
DRIVER_UP=`lsmod | grep -c mtlk`
if [ $DRIVER_UP != 0 ] 
then
	count=0
	rmmod mtlk
	while [ $? = 1 ]
	do
		count=`expr $count + 1`
		if [ $count != 10 ]
		then
			sleep 1
			rmmod mtlk
		else
			echo failed to remove mtlk > /tmp/init_failed
			break
		fi	
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

if [ ! -e /tmp/init_failed ]; then (. ./mtlk_mac_cloning.sh); fi

#echo Dump errors
# Dump errors (seen if the user has a serial interface)
if [ -e /tmp/init_failed ]; then echo -e "\n\nInit failed, due to error:"; cat /tmp/init_failed; fi
echo "Reload process has done" 

