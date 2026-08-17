#!/bin/sh

echo "Start apps init"


# Activates the Supplicant if in STA mode.
if expr $NETWORK_TYPE = 0
then
	BRIDGE_MODE=`awk -F "=" '/^BridgeMode/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`

	if expr $BRIDGE_MODE \> 0 
	then
		# if the bridge mode is mac_clon
		if expr $BRIDGE_MODE = 3
		then
			macl=`cat /tmp/mac_cloning.addr`
			$WPA_APPS_PATH/wpa_supplicant -Dwext -bbr0 -iwlan0 -c $WPA_CONFIGS_PATH/wpa_supplicant0.conf -p maclone=$macl > /dev/null 2>/dev/null &
		else
			$WPA_APPS_PATH/wpa_supplicant -Dwext -bbr0 -iwlan0 -c $WPA_CONFIGS_PATH/wpa_supplicant0.conf > /dev/null 2>/dev/null &
		fi
	else
		$WPA_APPS_PATH/wpa_supplicant -Dwext -iwlan0 -c $WPA_CONFIGS_PATH/wpa_supplicant0.conf > /dev/null 2>/dev/null &

	fi

fi

WPS_ON=`awk -F "=" '/^NonProc_WPS_ActivateWPS/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/wlan0.conf`
# Bring up WPS or standalone hostapd/wpa_supplicant 
if expr $WPS_ON = 1
then 
	ln -s  $WPA_APPS_PATH/wsccmd /tmp/wsccmd
	ln -s $ETC_PATH/mtlk_init_wps.sh /tmp/mtlk_init_wps.sh
	if expr $WPS_PB = 1
	then
		ln -s $ETC_PATH/WPS_PBC.sh /tmp/WPS_PBC.sh
	fi
	if expr $NETWORK_TYPE = 2
	then
		ln -s $WPA_APPS_PATH/hostapd /tmp/hostapd
		ln -s $WPA_APPS_PATH/hostapd.eap_user /tmp/hostapd.eap_user
		ln -s $WPA_APPS_PATH/hostapd_cli /tmp/hostapd_cli
		ln -s $WPA_APPS_PATH/template.conf /tmp/template.conf

	fi
	if expr $NETWORK_TYPE = 0
	then
		ln -s $WPA_APPS_PATH/wpa_cli /tmp/wpa_cli
		ln -s $WPA_APPS_PATH/wpa_passphrase /tmp/wpa_passphrase
		ln -s $WPA_APPS_PATH/wpa_supplicant /tmp/wpa_supplicant
	
	fi
	chmod 777 /tmp/*
	#running the wps application is now located at the end of all init processes
	
	
	# update the pin,(TODO if random PIN is supported,can be done after wsccmd is running)
	#DEVICE_PIN=`awk -F "=" '/^NonProc_WPS_DevicePIN/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/wlan0.conf`

	#DP_WC=`echo $DEVICE_PIN | wc -c`
	#if expr $DP_WC = 1 
	#    then 
	#    sleep 1
	#    echo -n NonProc_WPS_DevicePIN= >> /tmp/wlan0.conf 
	#    cat /tmp/WPS_DEVICE_PIN >> /tmp/wlan0.conf
	#    echo -n NonProc_WPS_DevicePIN= >> /mnt/jffs2/wlan0.conf 
	#    cat /tmp/WPS_DEVICE_PIN >> /mnt/jffs2/wlan0.conf
	#fi
else
	# Activates the Hostapd 
	if expr $NETWORK_TYPE = 2 
	then
		if expr $WLAN_NUM_BANDS = 2
		then
			# With dual band, create the second bridge before starting hostapd - otherwise it fails
			brctl addbr br1
			brctl setfd br1 0
		fi
		# TODO: check which script should configure that unless error in dmesege
		ifconfig wlan0 $IP_WLAN netmask $SUBNET_WLAN
		DEBUG_LEVEL=`awk -F "=" '/^UpDebugLevel/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/wlan0.conf`
		# TODO: Replace the following sleep with an indication from the driver that the wls is ready
		if expr $DEBUG_LEVEL = 1
		then
			sleep 3
		fi
		$WPA_APPS_PATH/hostapd -d $WPA_CONFIGS_PATH/hostapd0.conf > /dev/null 2>/dev/null &
		
		if expr $WLAN_NUM_BANDS = 2
		then
			ifconfig wlan1 0.0.0.0 
			# TODO: Replace the following sleep with an indication from the driver that the wls is ready
			if expr $DEBUG_LEVEL = 1
			then
				sleep 3
			fi
			$WPA_APPS_PATH/hostapd -d $WPA_CONFIGS_PATH/hostapd1.conf > /dev/null 2>/dev/null &
		fi
	fi
fi



# Run wps application 
if expr $WPS_ON = 1
then 
	if expr $NETWORK_TYPE = 0 || expr $NETWORK_TYPE = 1
	then
		#enable STA scan
		iwlist wlan0 scanning > /dev/null 2>/dev/null &
		#sleep 4
	fi
	cd /tmp
	./mtlk_init_wps.sh
	dblevel=`awk -F "=" '/^UpDebugLevel/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/wlan0.conf`
	#WpsHostIP and WpsHostPort shown in the web only if UpDebugLevel = 1
	WpsHostIP=`awk -F "=" '/^WpsHostIP/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/wlan0.conf`
	WpsHostPort=`awk -F "=" '/^WpsHostPort/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/wlan0.conf`
	wpsDebug=0
	if [ $WpsHostIP ]
	then
		if expr $dblevel = 1 
		then
			if [ -e /usr/bin/nc ]
			then 
				echo "Run debug WPS via NetCat"
				./wsccmd 1 2 3 4 2>&1 | nc $WpsHostIP $WpsHostPort & 
				ps | grep nc | grep -v grep
				ps | grep wsccmd | grep -v grep
				wpsDebug=1
			fi
		fi
	fi
	if expr $wpsDebug = 0
	then
		./wsccmd 1 2 3 4 > /dev/null 2>/dev/null &
	fi 

	#sleep 5
	if expr $WPS_PB = 1
	then
		./WPS_PBC.sh &
	fi
	cd -
fi

# Run WatchDog script
echo call drvhlpr
./mtlk_drvhlpr.sh > /tmp/drvhlpr_log &

echo "Finished apps init"
