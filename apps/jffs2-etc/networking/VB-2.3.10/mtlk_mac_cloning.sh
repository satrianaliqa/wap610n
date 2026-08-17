#!/bin/sh

#Set up MAC addr for mac cloning
#if ver is 2.3.10 and up do nothing	
if [ -e /proc/sys/dev/mtlk/wlan0/Version ]
then
	if [ ! $BRIDGE_MODE ]
	then
		BRIDGE_MODE=`awk -F "=" '/^BridgeMode/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`
	fi
	# In MAC cloning mode, give the interface a fake and well-know MAC address.
	# (The cloned MAC is used only by the wls driver)	
	if [ $BRIDGE_MODE = 3 ]
	then
		ifconfig wlan0 down
		ifconfig wlan0 hw ether 00:11:fe:ed:be:ef
		ifconfig wlan0 up
	fi

fi
