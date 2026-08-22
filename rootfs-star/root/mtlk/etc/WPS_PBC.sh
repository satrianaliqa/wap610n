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
			cat $WPS_PBC_GPIO > /dev/null
			#Jacky.Yang 1-Dec-2008, Remove set unconfigured flag to mtlk_wps_cmd.tcl when we success get correct wps status.
			/root/mtlk/etc/mtpriv wlan0 Wildcard_ESSID ''
			# Jacky.Yang 30-Nov-2008, for GUI
			kill `ps |grep 'wpsCleanTempClock.sh'|grep -v 'grep'|awk '{print $1}'`
			/root/mtlk/web/gui_wps_init.sh start
			#/root/mtlk/etc/wps_config_flash.sh
			echo pbc > /var/wps_type

			#Add for active WPS monitor of HNAP - Ricky Cao on Nov. 24 2008
			if [ -e /tmp/hnap_wps_status ]; then
				HNAP_WPS_STATUS='cat /tmp/hnap_wps_status'
				if [ $HNAP_WPS_STATUS = '0' ]; then
					echo "Abort previous WPS progress.."
					/root/mtlk/etc/mtlk_wps_cmd.tcl abort
				fi
			fi
			killall -SIGUSR1 hnap_wps_status_monitor
			if [ -e /tmp/wps_current_status ]; then
				echo "Delete /tmp/wps_current_status for start hnap_wps_monitor.."
				rm /tmp/wps_current_status
			fi
			echo 0 > /tmp/hnap_wps_status
			/root/mtlk/etc/hnap_wps_status_monitor &
			#Ricky Cao on Nov. 24 2008
			./mtlk_wps_cmd.tcl $action_type
		done
	fi
fi


echo 'WPS pushbutton: not active'
