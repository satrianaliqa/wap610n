#!/bin/sh
DIAG_LOG=/bin/diag_log.sh

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

case "$ACTION" in
	1|2|3|4|5|6|7|8) ;;
	*)
		$DIAG_LOG "WPS: unknown event '$ACTION'" 2>/dev/null || true
		exit 1
		;;
esac

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

$DIAG_LOG "WPS: event $1" 2>/dev/null || true

# WPS In progress
if [ "$ACTION" = "$WPS_INTERNAL_STAT_SCANNING" ]
then
	echo 2 > /dev/gpio2 2>/dev/null
	if [ "$NETWORK_TYPE" = "0" ]
	then
		$DIAG_LOG "WPS: event 1 - SEARCHING" 2>/dev/null || true
	else
		$DIAG_LOG "WPS: event 1 - LISTENING" 2>/dev/null || true
	fi

	#Jacky.Yang 27-Nov-2008, for check GUI is live, if GUI doesn' kill is file that mean GUI doesn't living.
	echo "1" > /var/gui_death

	echo "WPS_Status = 1" > $WPS_CURRENT_STATUS_FILE
fi

if [ "$ACTION" = "$WPS_INTERNAL_STAT_CONNECTING" ]
then
	echo 2 > /dev/gpio2 2>/dev/null
	$DIAG_LOG "WPS: event 2 - trying to connect" 2>/dev/null || true
	echo "WPS_Status = 3" > $WPS_CURRENT_STATUS_FILE
	$WPS_CMD_SCRIPT save_settings
fi

if [ "$ACTION" = "$WPS_INTERNAL_STAT_OVERLAP" ]
then
	echo 0 > /dev/gpio2 2>/dev/null
	$DIAG_LOG "WPS: event 3 - session overlap" 2>/dev/null || true
	echo "WPS_Status = 12" > $WPS_CURRENT_STATUS_FILE
	#Jacky.Yang 25-Nov-2008, delete temp file.
	$DIAG_LOG "WPS: session overlap" 2>/dev/null || true
fi

if [ "$ACTION" = "$WPS_INTERNAL_STAT_TIMEOUT" ]
then
	echo 0 > /dev/gpio2 2>/dev/null

	# Check previous status
	LAST_STATUS=`awk -F "=" '/^WPS_Status/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' $WPS_CURRENT_STATUS_FILE 2>/dev/null`
	if [ "$LAST_STATUS" = "2" ]
	then
		echo "WPS_Status = 11" > $WPS_CURRENT_STATUS_FILE
		$DIAG_LOG "WPS: event 4 - session timeout" 2>/dev/null || true
	else
		echo "WPS_Status = 10" > $WPS_CURRENT_STATUS_FILE
		$DIAG_LOG "WPS: event 4 - walk timeout" 2>/dev/null || true
	fi

	#Jacky.Yang 12-Feb-2009, for fix wpe WET-shared will crash DUT issue.
	ifconfig wlan0 down
	ifconfig wlan0 up
fi

if [ "$ACTION" = "$WPS_INTERNAL_STAT_MANUAL_AP_SELECT" ]
then
	$DIAG_LOG "WPS: event 5 - manual AP selection" 2>/dev/null || true
	$WPS_CMD_SCRIPT manual_ap_selection
fi

if [ "$ACTION" = "$WPS_INTERNAL_STAT_REGISTERING" ]
then
	$DIAG_LOG "WPS: event 6 - registering" 2>/dev/null || true
	echo "WPS_Status = 2" > $WPS_CURRENT_STATUS_FILE
fi

if [ "$ACTION" = "$WPS_INTERNAL_STAT_PINERROR" ]
then
	echo 0 > /dev/gpio2 2>/dev/null
	$DIAG_LOG "WPS: event 7 - PIN error" 2>/dev/null || true
	echo "WPS_Status = 13" > $WPS_CURRENT_STATUS_FILE
fi

if [ "$ACTION" = "$WPS_INTERNAL_STAT_CONNECTED" ]
then
	echo 1 > /dev/gpio2 2>/dev/null
	$DIAG_LOG "WPS: event 8 - connected" 2>/dev/null || true
	echo "WPS_Status = 4" > $WPS_CURRENT_STATUS_FILE
fi

echo "WPS_LastErrorCode = $1" >$WPS_LAST_CODE_FILE

# Success/ Failure
if [ "$ACTION" = "$WPS_INTERNAL_STAT_CONNECTING" ] || [ "$ACTION" = "$WPS_INTERNAL_STAT_TIMEOUT" ] || [ "$ACTION" = "$WPS_INTERNAL_STAT_OVERLAP" ] || [ "$ACTION" = "$WPS_INTERNAL_STAT_PINERROR" ]
then
	rm -f "$WPS_STARTUP_TIME_FILE"
	$DIAG_LOG "WPS: erasing startup point" 2>/dev/null || true

	#restore previous wlidcard_essid value to the driver
	$WPS_CMD_SCRIPT set_wildcard

	if [ "$ACTION" != "$WPS_INTERNAL_STAT_CONNECTING" ] && [ "$NETWORK_TYPE" = "0" ]
	then
		echo "wps_event.sh: Trying to reconnect to previous AP (restart wsccmd and supplicant)"

		$WPS_CMD_SCRIPT stop
		$WPS_CMD_SCRIPT start

		cd $COMBINED_VER_PATH/web
		./init_security.sh reactivate >/dev/null
		cd -
	fi

	$DIAG_LOG "WPS: session done" 2>/dev/null || true

	# Check static mode. ip_config_method = 1 is static mode.
	if [ "$ip_config_method" = "1" ]
	then
		$DIAG_LOG "WPS: static mode" 2>/dev/null || true
		if [ ! -e /var/tryToGetSSID ]
		then
			$DIAG_LOG "WPS: stopping GUI WPS state" 2>/dev/null || true
			/root/mtlk/web/gui_wps_init.sh stop
		fi
	fi

	#Jacky.Yang 25-Nov-2008, delete temp file, if network is static mode we need delete here, if dhcp mode that dhcp.tcl will kill it.
	#network_type = 0 is STA mode.
	if [ "$NETWORK_TYPE" != "0" ]
	then
		$DIAG_LOG "WPS: session done; clearing web commit state" 2>/dev/null || true
		#rm -f /var/webCommit
		#rm -f /var/gui_wps_waiting
		#Jacky.Yang 29-Nov-2008, add for GUI WPS
		#echo "mtlk_wps_event.sh:DHCP renew" >/dev/console
		#killall -USR1 udhcpc

		#Jacky.Yang 6-Dec-2008, try to write back wlan0.conf for prevent inconsistent issue.
		#$WPS_CMD_SCRIPT save_settings
	fi
fi

exit 0
