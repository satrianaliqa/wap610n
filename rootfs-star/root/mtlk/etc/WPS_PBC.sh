#!/bin/sh
# A pushbutton daemon for dorango

#Jacky.Yang 14-Oct-2008, If wireless is unconfigured, we doesn't want connect to default ssid.
unconfigured=`awk -F "=" '/^unconfigured/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/wlan0.conf`

if [ $1 ]
then
	WPS_ON=$1
else
	WPS_ON=0
fi

if [ $2 ]
then
	NETWORK_TYPE=$2
else
	NETWORK_TYPE=
fi

if [ $WPS_ON == "1" ] || [ $NETWORK_TYPE > "2" ]
then

	if [ -d /root/mtlk ]
	then 
		COMBINED_VER_PATH=/root/mtlk
	else
		COMBINED_VER_PATH=/mnt/jffs2
	fi
	cd $COMBINED_VER_PATH/etc

	action_type='get_conf_via_pbc'
	if [ $NETWORK_TYPE == "2" ]
	then
		action_type='conf_via_pbc'
	fi

	# Get WPS gpio device from HW.ini
	WPS_PBC_GPIO=`awk -F "=" '/^WPS_PB/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/HW.ini`

	if [ "$WPS_PBC_GPIO" != "null" ]
	then
		while [ 1 ]
		do
			# Wait for physical WPS button press
			cat $WPS_PBC_GPIO > /dev/null
			echo "[HARDWARE EVENT] WPS Button Pressed! Executing Safe System Shutdown..." > /dev/console
			
			# Turn off activity LEDs
			echo 0 > /dev/led0 2>/dev/null || true
			echo 0 > /dev/led1 2>/dev/null || true
			
			# Sync storage buffers and safely halt CPU
			sync
			poweroff
		done
	fi
fi


echo 'WPS pushbutton: not active'
