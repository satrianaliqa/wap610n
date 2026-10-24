#!/bin/sh
# Monitor GoAhead Web Server memory footprint.
# /etc/inittab (::respawn:/root/mtlk/web/run_webs.sh) supervises and restarts webs.

while true
do
	if [ ! -f /var/old_web_pid ] && [ ! -f /var/firmware_upgrading ]; then
		memSize=`ps | grep '[w]ebs' | grep -v 'run_webs' | head -n 1 | awk '{print $3}'`
		if [ -n "$memSize" ] && [ "$memSize" -gt 6000 ] 2>/dev/null; then
			echo "monitor_web.sh: webs VSZ ($memSize KB) exceeded threshold, triggering respawn via inittab"
			killall webs 2>/dev/null || true
		fi
	fi
	sleep 300
done