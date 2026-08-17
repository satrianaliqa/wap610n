#!/bin/sh
#set -x
echo "Start apps init"

if [ ! $NETWORK_TYPE ]
then
	NETWORK_TYPE=`awk -F "=" '/^network_type/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/wlan0.conf`
fi
# Activates the Supplicant if in STA mode.
if [ $NETWORK_TYPE = 0 ]
then
	# if the STA was never connected to an AP (after restore default) dont activate Supplicant
	NeverConnected=`awk -F "=" '/^NeverConnected/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/wlan0.conf`	
	if [ "$NeverConnected" != "1" ]
	then
		if [ ! $BRIDGE_MODE ]
		then
			BRIDGE_MODE=`awk -F "=" '/^BridgeMode/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`
		fi
		
		WPA_SUPPLICANT_COUNT=`ps | grep wpa_supplicant | grep -c -v grep`
		if [ $WPA_SUPPLICANT_COUNT = 0 ]
		then
			# if the bridge mode is mac_clon
			if [ $BRIDGE_MODE = 3 ] && [ -e /tmp/mac_cloning.addr ]
			then
				macl=`cat /tmp/mac_cloning.addr`
				$WPA_APPS_PATH/wpa_supplicant -Dwext -bbr0 -iwlan0 -c $WPA_CONFIGS_PATH/wpa_supplicant0.conf -p maclone=$macl > /dev/null 2>/dev/null &
			else
				$WPA_APPS_PATH/wpa_supplicant -Dwext -bbr0 -iwlan0 -c $WPA_CONFIGS_PATH/wpa_supplicant0.conf > /dev/null 2>/dev/null &
			fi
		else
			echo "WPA_SUPPLICANT_COUNT > 0"
		fi
	else
		echo "Not activating supplicant - NeverConnected = 1"
	fi		
fi


if [ ! $WPS_ON ]
then
	WPS_ON=`awk -F "=" '/^NonProc_WPS_ActivateWPS/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/wlan0.conf`
fi

# Bring up WPS or standalone hostapd/wpa_supplicant 
if [ $WPS_ON = 1 ]
then 
	ln -s  $WPA_APPS_PATH/wsccmd /tmp/wsccmd
	ln -s $ETC_PATH/mtlk_init_wps.sh /tmp/mtlk_init_wps.sh
	if [ $WPS_PB = 1 ]
	then
		ln -s $ETC_PATH/WPS_PBC.sh /tmp/WPS_PBC.sh
	fi
	if [ $NETWORK_TYPE = 2 ]
	then
		ln -s $WPA_APPS_PATH/hostapd /tmp/hostapd
		ln -s $WPA_APPS_PATH/hostapd.eap_user /tmp/hostapd.eap_user
		ln -s $WPA_APPS_PATH/hostapd_cli /tmp/hostapd_cli
		ln -s $WPA_APPS_PATH/template.conf /tmp/template.conf

	fi
	if [ $NETWORK_TYPE = 0 ]
	then
		ln -s $WPA_APPS_PATH/wpa_cli /tmp/wpa_cli
		ln -s $WPA_APPS_PATH/wpa_passphrase /tmp/wpa_passphrase
		ln -s $WPA_APPS_PATH/wpa_supplicant /tmp/wpa_supplicant
	
	fi
	chmod 777 /tmp/*
else
	# Activates the Hostapd 
	if [ $NETWORK_TYPE = 2 ]
	then
		HOSTAPD_COUNT=`ps | grep hostapd | grep -c -v grep`
		if [ $HOSTAPD_COUNT = 0 ]
		then
			# TODO: check which script should configure that unless error in dmesege
			#ifconfig wlan0 $IP_WLAN netmask $SUBNET_WLAN
			# TODO: Replace the following sleep with an indication from the driver that the wls is ready
			#sleep 3
			if  [ ! -e $WPA_CONFIGS_PATH/hostapd0.conf ]
			then
				cp $WPA_CONFIGS_PATH/hostapd.conf  $WPA_CONFIGS_PATH/hostapd0.conf
			fi
			$WPA_APPS_PATH/hostapd -d $WPA_CONFIGS_PATH/hostapd0.conf > /dev/null 2>/dev/null &
		else
			echo "HOSTAPD_COUNT > 0"
		fi
	fi
fi



# Run wps application 
if [ $WPS_ON = 1 ]
then
	WPS_COUNT=`ps | grep wsccmd | grep -c -v grep`
	if [ $WPS_COUNT = 0 ]
	then
		cd /tmp
		(. ./mtlk_init_wps.sh);
		./wsccmd 1 2 3 4 > /dev/null 2>/dev/null &
		#sleep 5
		if [ $WPS_PB = 1 ]
		then
			./WPS_PBC.sh $WPS_ON $NETWORK_TYPE &
		fi
		cd -
	else
		echo "WPS_COUNT > 0"
	fi
fi


echo "Finished apps init"
