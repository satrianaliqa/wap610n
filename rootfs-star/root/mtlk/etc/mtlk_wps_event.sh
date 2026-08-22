#!/bin/sh

if [ -d /root/mtlk ]
then 
	COMBINED_VER_PATH=/root/mtlk
else
	COMBINED_VER_PATH=/mnt/jffs2
fi

WPS_LAST_CODE_FILE=/tmp/wps_last_code
WPS_STARTUP_TIME_FILE=/tmp/wps_startup_time
WPS_CMD_SCRIPT=$COMBINED_VER_PATH/etc/mtlk_wps_cmd.tcl
WPS_CURRENT_STATUS_FILE=/tmp/wps_current_status
ACTION=$1

WPS_INTERNAL_STAT_SCANNING='1'
WPS_INTERNAL_STAT_CONNECTING='2'
WPS_INTERNAL_STAT_OVERLAP='3'
WPS_INTERNAL_STAT_TIMEOUT='4'
WPS_INTERNAL_STAT_MANUAL_AP_SELECT='5'
WPS_INTERNAL_STAT_REGISTERING='6'
WPS_INTERNAL_STAT_PINERROR='7'
WPS_INTERNAL_STAT_CONNECTED='8'

NETWORK_TYPE=`awk -F "=" '/^network_type/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/wlan0.conf`
ip_config_method=`awk -F "=" '/^ip_config_method/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`

echo wps_event.sh:$1 >/dev/console

# WPS In progress
if [ $ACTION = $WPS_INTERNAL_STAT_SCANNING ]
then
	if [ $NETWORK_TYPE = 0 ]
	then
		echo "wps_event.sh:Event 1 - SEARCHING" >/dev/console
	else
		echo "wps_event.sh:Event 1 - LISTENING" >/dev/console
	fi
	
	#Jacky.Yang 27-Nov-2008, for check GUI is live, if GUI doesn' kill is file that mean GUI doesn't living.
	echo "1" > /var/gui_death

	echo "WPS_Status = 1" > $WPS_CURRENT_STATUS_FILE
fi

if [ $ACTION = $WPS_INTERNAL_STAT_CONNECTING ]
then
	echo "wps_event.sh:Event 2 - Trying to Connect" >/dev/console
	echo "WPS_Status = 3" > $WPS_CURRENT_STATUS_FILE
	$WPS_CMD_SCRIPT save_settings
fi

if [ $ACTION = $WPS_INTERNAL_STAT_OVERLAP ]
then
	echo "wps_event.sh:Event 3 - ERROR_SESSION_OVERLAP" >/dev/console
	echo "WPS_Status = 12" > $WPS_CURRENT_STATUS_FILE
	#Jacky.Yang 25-Nov-2008, delete temp file.
	echo "wps_event.sh: WPS_INTERNAL_STAT_OVERLAP" >/dev/console
fi

if [ $ACTION = $WPS_INTERNAL_STAT_TIMEOUT ]
then
	
	# Check previous status
	LAST_STATUS=`awk -F "=" '/^WPS_Status/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' $WPS_CURRENT_STATUS_FILE`
	if [ $LAST_STATUS = '2' ] 
	then
		echo "WPS_Status = 11" > $WPS_CURRENT_STATUS_FILE
		echo "wps_event.sh:Event 4 - ERROR_SESSION_TIMEOUT " >/dev/console
	else
		echo "WPS_Status = 10" > $WPS_CURRENT_STATUS_FILE
		echo "wps_event.sh:Event 4 - ERROR_WALK_TIMEOUT " >/dev/console
	fi

	#Jacky.Yang 12-Feb-2009, for fix wpe WET-shared will crash DUT issue.
	ifconfig wlan0 down
	ifconfig wlan0 up
fi

if [ $ACTION = $WPS_INTERNAL_STAT_MANUAL_AP_SELECT ]
then
	echo "wps_event.sh:Event 5 - Manual AP selection." >/dev/console
	$WPS_CMD_SCRIPT manual_ap_selection
fi

if [ $ACTION = $WPS_INTERNAL_STAT_REGISTERING ]
then
	echo "wps_event.sh:Event 6 - REGISTERING" >/dev/console
	echo "WPS_Status = 2" > $WPS_CURRENT_STATUS_FILE
fi

if [ $ACTION = $WPS_INTERNAL_STAT_PINERROR ]
then
	echo "wps_event.sh:Event 7 - PIN Error" >/dev/console
	echo "WPS_Status = 13" > $WPS_CURRENT_STATUS_FILE
fi

if [ $ACTION = $WPS_INTERNAL_STAT_CONNECTED ]
then
	echo "wps_event.sh:Event 8 - CONNECTED" >/dev/console
	echo "WPS_Status = 4" > $WPS_CURRENT_STATUS_FILE
fi

echo "WPS_LastErrorCode = $1" >$WPS_LAST_CODE_FILE	

# Success/ Failure 
if [ $ACTION = $WPS_INTERNAL_STAT_CONNECTING ] || [ $ACTION = $WPS_INTERNAL_STAT_TIMEOUT ] || [ $ACTION = $WPS_INTERNAL_STAT_OVERLAP ] || [ $ACTION = $WPS_INTERNAL_STAT_PINERROR ]
then
	rm $WPS_STARTUP_TIME_FILE
	echo "wps_event.sh:Erasing startup point" >/dev/console
	
	#restore previous wlidcard_essid value to the driver 
	$WPS_CMD_SCRIPT set_wildcard
	
	if [ $ACTION != $WPS_INTERNAL_STAT_CONNECTING ] && [ $NETWORK_TYPE = 0 ]
	then
		echo "wps_event.sh: Trying to reconnect to previous AP (restart wsccmd and supplicant)"
		
		$WPS_CMD_SCRIPT stop
		$WPS_CMD_SCRIPT start
		
		cd $COMBINED_VER_PATH/web
		./init_security.tcl reactivate >/dev/null
		cd -
	fi
	
	echo "wps_event.sh: WPS Session Done" >/dev/console

	# Check static mode. ip_config_method = 1 is static mode.
	if [ $ip_config_method = 1 ]
	then
		echo "wps_event.sh: Static mode" > /dev/console
		if [ ! -e /var/tryToGetSSID ]
		then
			echo "wps_event.sh: /root/mtlk/web/gui_wps_init.sh stop" > /dev/console
			/root/mtlk/web/gui_wps_init.sh stop
		fi
	fi

	#Jacky.Yang 25-Nov-2008, delete temp file, if network is static mode we need delete here, if dhcp mode that dhcp.tcl will kill it.
	#network_type = 0 is STA mode.
	if [ $NETWORK_TYPE != 0 ]
	then
		echo "wps_event.sh: Success/Failure, delete webCommit" >/dev/console
		#rm -f /var/webCommit
		#rm -f /var/gui_wps_waiting
		#Jacky.Yang 29-Nov-2008, add for GUI WPS
		#echo "mtlk_wps_event.sh:DHCP renew" >/dev/console
		#killall -USR1 udhcpc
		
		#Jacky.Yang 6-Dec-2008, try to write back wlan0.conf for prevent inconsistent issue.
		#$WPS_CMD_SCRIPT save_settings
	fi
fi

exit


