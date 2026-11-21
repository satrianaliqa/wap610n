#!/bin/sh

existing=""
curr=""
newPinCode=""

# Get current Pin_Code
curr=`awk -F "=" '/^NonProc_WPS_DevicePIN/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str; exit}' "$CONFIGS_PATH/wlan0.conf" 2>/dev/null`
# If it's not empty, abort
if [ -z "$curr" ]
then	
	newPinCode=`get_env_param device_pin_code`
	# make sure the passprase was not deleted
	if [ -z "$newPinCode" ]
	then
		echo "make_device_pin:Could not retrieve DevicePIN from ENV, setting default 12345670."
		newPinCode=12345670
	fi
	
	grep -v "^NonProc_WPS_DevicePIN" /tmp/wlan0.conf > /tmp/tmpwlan.conf
	echo "NonProc_WPS_DevicePIN = $newPinCode" >> /tmp/tmpwlan.conf
	mv /tmp/tmpwlan.conf "$CONFIGS_PATH/wlan0.conf"
	cp "$CONFIGS_PATH/wlan0.conf" /tmp/wlan0.conf
	
	# Files will be saved only once in init process, Flag will down by the mtlk_init.sh
	echo 1 > /tmp/NeedSave.txt		
fi

