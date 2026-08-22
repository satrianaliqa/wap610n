#!/bin/sh

WPSHistory=/var/WPS_History

echo "" > $WPSHistory

n=1
while [ "$n" -lt $1 ]
do
	CurrentTime=`cat /proc/uptime | awk -F "." '{print $1}'`
	WPSCurrentStatus=`cat /tmp/wps_current_status | awk -F " = " '{printf $2}'`
	if [ "$WPSCurrentStatus" = "" ]
	then
		WPSCurrentStatus=-1
	fi

	echo "$CurrentTime,$WPSCurrentStatus" >> $WPSHistory

	sleep 1
    	n=$(( $n + 1 ))
done

echo "wpsCleanTempClock.sh: Delete all wps gui temp file by clock($1 seconds)." > /dev/console
echo wpsCleanTempClock.sh: delete /var/gui_wps_waiting > /dev/console
/root/mtlk/web/gui_wps_init.sh stop
rm -f /var/gui_wps_waiting
rm -f /var/success_get_ip
rm -f /var/assign_get_ip
