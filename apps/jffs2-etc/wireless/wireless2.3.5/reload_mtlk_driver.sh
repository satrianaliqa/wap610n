#!/bin/sh


# Run this script to reload the metalink driver, without rebooting.
echo reload > /tmp/reload


#echo "Start Reload process"
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





wlan_filename="wlan0.conf"


NETWORK_TYPE=`awk -F "=" '/^network_type/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/$wlan_filename`


WLAN_NUM_BANDS=1
export WLAN_NUM_BANDS

echo kill drvhlpr
A=`ps | grep drvhlpr | grep -v grep | awk '{print $1}'`
	for a in $A ; do kill -9 $a ; done
	
echo kill WPS_PBC
A=`ps | grep WPS_PBC.sh | grep -v grep | awk '{print $1}'`
	for a in $A ; do kill -9 $a ; done

echo kill WPS
A=`ps | grep wsccmd | grep -v grep | awk '{print $1}'`
for a in $A ; do kill -9 $a ; done

echo kill hostapd
A=`ps | grep hostapd | grep -v grep | awk '{print $1}'`
for a in $A ; do kill -9 $a ; done

echo kill supplicant
A=`ps | grep wpa_supplicant | grep -v grep | awk '{print $1}'`
for a in $A ; do kill -9 $a ; done

sleep 3


WLAN_BRIDGING=`awk -F "=" '/^BridgeMode/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`

if expr $WLAN_BRIDGING != 0
then
  echo bridge mode, delete interface
  ifconfig wlan0 down
  brctl delif br0 wlan0
fi


echo Removing mtlk

# Source platform-specific settings
#echo $MOD_EXT

#delete init_failed if exist
if [ -f /tmp/init_failed ]; then rm /tmp/init_failed ; fi

#sleep 3
#rmmod led
#sleep 5

#patch:
count=0
rmmod mtlk
while expr $? = 1
do
	count=`expr $count + 1`
	if expr $count != 10
	then
		sleep 1
		rmmod mtlk
	else
		echo failed to remove mtlk > /tmp/init_failed
	fi	
done
echo count=$count

#Clean tmp folder 
#echo Removing dlinks in tmp
rm /tmp/*.bin
rm /tmp/*.ko
rm /tmp/*.tcl
A=`ls /tmp/WPS* | grep -v WPS_DEVICE_PIN`
rm $A

sleep 5

# Source platform-specific settings
#echo $MOD_EXT
#delete init_failed if exist


#cd /mnt/jffs2/etc
. ./mtlk_init_platform.sh

echo Starting with mtlk_init_wls.sh
if [ ! -e /tmp/init_failed ]; then (. ./mtlk_init_wls.sh); fi
echo mow mtlk_init_route.sh
if [ ! -e /tmp/init_failed ]; then (. ./mtlk_init_route.sh); fi
echo now mtlk_init_wls_apps.sh
if [ ! -e /tmp/init_failed ]; then (. ./mtlk_init_wls_apps.sh); fi

#echo message log handling
# Save the head of the message log (useful for debugging when there were many msgs and the log was rotated)
#./mtlk_save_log.sh &

echo Dump errors
# Dump errors (seen if the user has a serial interface)
if [ -e /tmp/init_failed ]; then echo -e "\n\nInit failed, due to error:"; cat /tmp/init_failed; fi
#sleep 3
if [ ! -e /tmp/init_failed ]; then (. ./mtlk_init_route.sh); fi

echo "Reload process has done" 

