#!/bin/sh

# upnpd monitor - make sure upnpd doesn't cause mem leaks
# If upnpd mem utilization increases above a preset limit, daemon will be restarted.
# Copyright (C) 2009  Metalink Ltd.


# Change the following parameters to suit your requirements:
# 1. max mem limit (in KB)
# 2. sleep time between monitor operations (in seconds)
# 3. location of log file
UPNPD_MEM_LIMIT=2000
SLEEP_TIME=120
DEBUG_LOG=/dev/null
##DEBUG_LOG=/tmp/upnpd_monitor.txt


# Wait briefly for bridge initialization before polling
sleep 15

# DEBUG: Log the start of monitoring
echo Starting upnpd monitor > $DEBUG_LOG
cat /proc/uptime | awk '{print $1}' >> $DEBUG_LOG


while [ 1 ]
do
	UPNP_EN=""
	for conf in /mnt/jffs2/sys.conf /tmp/sys.conf; do
		if [ -f "$conf" ] && [ -z "$UPNP_EN" ]; then
			UPNP_EN=$(awk -F= '/^[ \t]*UPnP_Enabled[ \t]*=/ {v=$2; gsub(/^[ \t]+|[ \t\r]+$/, "", v); print v}' "$conf" 2>/dev/null | tail -n 1)
		fi
	done
	if [ "$UPNP_EN" != "1" ]; then
		exit 0
	fi
	if ! pidof upnpd >/dev/null 2>&1; then
		if [ -x /bin/upnpd ]; then
			nice -n 18 /bin/upnpd br0 &
		else
			nice -n 18 upnpd br0 &
		fi
		sleep 2
	fi
	UPNPD_MEM_USAGE=`ps | awk '$5 ~ /(^|\/)upnpd$/ {print $3; exit}'`
	case "$UPNPD_MEM_USAGE" in
		''|*[!0-9]*)
			sleep $SLEEP_TIME
			continue
			;;
	esac
	if [ "$UPNPD_MEM_USAGE" -gt "$UPNPD_MEM_LIMIT" ]
	then
		# DEBUG:
		##echo upnpd mem usage:  $UPNPD_MEM_USAGE >> $DEBUG_LOG

		# DEBUG: Log restart operation
		echo Restarting upnpd. mem usage:  $UPNPD_MEM_USAGE  >> $DEBUG_LOG
			cat /proc/uptime | awk '{print $1}'  >> $DEBUG_LOG

			# First try to kill upnpd nicely
			killall -SIGINT upnpd
			sleep 1

			# Now be rude if it isn't dead yet
			killall -9 upnpd  2> /dev/null
			sleep 1

			# Restart upnpd server
			if ! pidof upnpd >/dev/null 2>&1; then
				if [ -x /bin/upnpd ]; then
					nice -n 18 /bin/upnpd br0 &
				else
					nice -n 18 upnpd br0 &
				fi
			fi
	fi
	sleep $SLEEP_TIME
done


