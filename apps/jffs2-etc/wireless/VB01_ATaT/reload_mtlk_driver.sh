#!/bin/sh

# Run this script to reload the metalink driver, without rebooting.

#Clean tmp folder 
echo Removing dlinks in tmp
rm /tmp/*.bin
rm /tmp/*.ko
rm /tmp/*.tcl

echo "Start Reload process"
KERNEL_VER=`uname -r | awk -F '.' '{print $1"."$2}'`
if expr $KERNEL_VER = "2.4"
then
	MOD_EXT=o
else
	MOD_EXT=ko
fi

# Get the single-band / dual-band mode, 
# according to the number of wls interfaces on the PCI bus.
# Currently, there is no dual-band STA, so only allow dual band in AP mode, 
# (even if the HW supports dual band).

export NETWORK_TYPE
export WLAN_NUM_BANDS
export WPS_ON

if [ -e /tmp/wlan0.conf ] 
then 
	wlan_filename="wlan0.conf"
else
	wlan_filename="wlan.conf"
fi

NETWORK_TYPE=`awk -F "=" '/^network_type/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/$wlan_filename`

if expr $NETWORK_TYPE = 2
then
	WLAN_NUM_BANDS=`lspci | grep Wireless | wc -l`
else
	WLAN_NUM_BANDS=1
fi


killall drvhlpr

killall wsccmd

	
if expr $NETWORK_TYPE = 2
then
	echo kill hostapd
	killall hostapd
else
	echo kill supplicant
	killall wpa_supplicant
fi


WLAN_BRIDGING=`awk -F "=" '/^BridgeMode/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`

if expr $WLAN_BRIDGING != 0
then
  echo bridge mode, delete interface
  ifconfig wlan0 down
  brctl delif br0 wlan0
fi


echo Removing mtlk
sleep 3
#rmmod led
#sleep 5

#patch:
count=0
rmmod mtlk
while expr $? = 1
do
	echo failed to remove mtlk
	sleep 1
	count=`expr $count + 1`
	if expr $count != 10
		then
			rmmod mtlk
	else
		echo mtlk removed
	fi
done
echo count=$count

echo Waiting 5 sec...
sleep 5


# Source platform-specific settings
#echo $MOD_EXT
#delete init_failed if exist
if [ -f /tmp/init_failed ]; then rm /tmp/init_failed ; fi

#cd /mnt/jffs2/etc
. ./mtlk_init_platform.sh

echo Starting with mtlk_init_wls.sh
if [ ! -e /tmp/init_failed ]; then (. ./mtlk_init_wls.sh); fi
echo mow mtlk_init_route.sh
if [ ! -e /tmp/init_failed ]; then (. ./mtlk_init_route.sh); fi
echo now mtlk_init_wls_apps.sh
if [ ! -e /tmp/init_failed ]; then (. ./mtlk_init_wls_apps.sh); fi

echo message log handling
# Save the head of the message log (useful for debugging when there were many msgs and the log was rotated)
./mtlk_save_log.sh &

echo Dump errors
# Dump errors (seen if the user has a serial interface)
if [ -e /tmp/init_failed ]; then echo -e "\n\nInit failed, due to error:"; cat /tmp/init_failed; fi
sleep 3
if [ ! -e /tmp/init_failed ]; then (. ./mtlk_init_route.sh); fi

echo "Reload process has done" 

