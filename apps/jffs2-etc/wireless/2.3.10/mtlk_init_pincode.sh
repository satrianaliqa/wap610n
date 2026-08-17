#!/bin/sh
echo "Start mtlk_init_pincode.sh" > /dev/console
existing=""
curr=""
newPinCode=""

# Get current Pin_Code
existing=`cat $CONFIGS_PATH/wlan0.conf | grep NonProc_WPS_DevicePIN`
curr=`echo $existing | awk -F "=" '{print $2}'`
# If it's not empty, abort
if [ "$curr" = "" ]
then	
	newPinCode=`get_env_param device_pin_code`
	# make sure the passprase was not deleted
	if [ ! $newPinCode ]
	then
		echo "make_device_pin:Could not retrieve DevicePIN from ENV, setting default 12345670."
		newPinCode=12345670
	fi
	
	grep -v NonProc_WPS_DevicePIN /tmp/wlan0.conf > /tmp/tmpwlan.conf
	echo NonProc_WPS_DevicePIN = $newPinCode >> /tmp/tmpwlan.conf
	mv /tmp/tmpwlan.conf $CONFIGS_PATH/wlan0.conf
	cp $CONFIGS_PATH/wlan0.conf /tmp/wlan0.conf
	
	# Files will be saved only once in init process, Flag will down by the mtlk_init.sh
	echo 1 > /tmp/NeedSave.txt		
fi

