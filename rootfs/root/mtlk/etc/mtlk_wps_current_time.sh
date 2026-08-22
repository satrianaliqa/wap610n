#!/bin/sh

CURRENT_TIME=`cat /proc/uptime | awk -F "." '{print $1}'`
echo "WPS_CurrentTime = $CURRENT_TIME" > /tmp/wps_current_time