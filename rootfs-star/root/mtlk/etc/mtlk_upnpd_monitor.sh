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
DEBUG_LOG=/dev/console
##DEBUG_LOG=/tmp/upnpd_monitor.txt


# DEBUG: Log the start of monitoring
echo Starting upnpd monitor > $DEBUG_LOG
cat /proc/uptime | awk '{print $1}' >> $DEBUG_LOG


while [ 1 ]
do
	if ! pidof upnpd >/dev/null 2>&1; then
		nice -n 18 upnpd &
		sleep 2
	fi
	UPNPD_MEM_USAGE=`ps | grep " upnpd" | grep -v grep | awk 'NR==1 {print $3}'`
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
				nice -n 18 upnpd &
			fi
	fi
	sleep $SLEEP_TIME
done


