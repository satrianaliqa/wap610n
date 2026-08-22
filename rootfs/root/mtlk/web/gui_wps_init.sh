#!/bin/sh

ip_config_method=`awk -F "=" '/^ip_config_method/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`

if [ "$1" == "start" ]
then
	echo "gui_wps_init.sh: start." > /dev/console
	rm -f /var/success_get_ip
	rm -f /var/assign_get_ip
	rm -f /var/currentIP
	rm -f /tmp/wps_current_status
	rm -f /var/Get_IPInfo_From_DHCP
	rm -f /var/gui_get_ip_temp
	rm -f /var/wps_success_ssid
	rm -f /var/WPS_History_temp
	rm -f /var/WPS_History
	rm -f /var/tryToGetSSID
	rm -f /var/dhcp_timeout

	#kill `ps |grep 'wpsCleanTempClock.sh'|grep -v 'grep'|awk '{print $1}'`
	/root/mtlk/etc/umedia_kill_process.sh wpsCleanTempClock.sh
	echo 1 > /var/webCommit
	echo 1 > /var/gui_wps_waiting
	echo `cat /proc/uptime | awk -F \".\" '{print $1}'` > /var/wps_start_time
	echo `cat /proc/uptime | awk -F \".\" '{print $1}'` > /var/wps_current_time
	/root/mtlk/web/wpsCleanTempClock.sh 180 &
elif [ "$1" == "stop" ]; then
	echo "gui_wps_init.sh: stop.###########################################################" > /dev/console
	sleep 5
	#rm -f /tmp/wps_current_status
#	rm -f /var/Get_IPInfo_From_DHCP
	rm -f /var/gui_get_ip_temp
	rm -f /var/wps_success_ssid
	rm -f /var/wpsRunning
	rm -f /var/wps_type
	rm -f /var/gui_wps_waiting
	rm -f /var/tryToGetSSID

	/root/mtlk/etc/umedia_kill_process.sh wpsCleanTempClock.sh

	# Check static mode. ip_config_method = 1 is static mode.
	if [ $ip_config_method = 1 ]
	then
		rm -f /var/webCommit
	fi
fi
