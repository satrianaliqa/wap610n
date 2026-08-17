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
	UPNPD_MEM_USAGE=`ps | grep " upnpd" | grep -v grep | awk '{print $3}'`
	if [ $UPNPD_MEM_USAGE ]	
	then
		# DEBUG:
		##echo upnpd mem usage:  $UPNPD_MEM_USAGE >> $DEBUG_LOG

		if [ $UPNPD_MEM_USAGE -gt $UPNPD_MEM_LIMIT ]
		then
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
			nice -n 18 upnpd &
		fi
	fi
	sleep $SLEEP_TIME
done


