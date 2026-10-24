#!/bin/sh

# Start the BCL Server
# For debugging redirect to: /tmp/bcl.log, instead of /dev/null
echo "Starting BCL Server..."
if [ -e /bin/nice ]
then nice -n 18 $BCL_PATH/BclSockServer > /dev/null &
else
$BCL_PATH/BclSockServer > /dev/null &
fi

if [ $WLAN_NUM_BANDS ] && [ $WLAN_NUM_BANDS == 2 ]
then
	# Start the second BCL server
	if [ -e /bin/nice ]
	then nice -n 18 $BCL_PATH/bcl2 > /dev/null &
	else
	$BCL_PATH/bcl2 > /dev/null &
	fi
fi

# Note: GoAhead Web Server (webs) on Port 80 is supervised by /etc/inittab
# (::respawn:/root/mtlk/web/run_webs.sh). Signal SIGHUP so webs reloads
# wireless/system state once wireless daemons are initialized.
killall -HUP webs 2>/dev/null || true

# Start single instance of web memory monitor
if ! ps | grep -v grep | grep -q 'monitor_web.sh'; then
	/root/mtlk/web/monitor_web.sh &
fi
