#!/bin/sh
# Monitor GoAhead Web Server memory footprint.
# /etc/inittab (::respawn:/root/mtlk/web/run_webs.sh) supervises and restarts webs.

while true
do
	if [ ! -f /var/old_web_pid ] && [ ! -f /var/firmware_upgrading ]; then
		# Reap any leaked forked webs child (legitimate init-managed webs has PPid 1)
		for wpid in `ps | awk '$5 ~ /(^|\/)webs$/ {print $1}'`; do
			wppid=`awk '{print $4}' /proc/$wpid/stat 2>/dev/null`
			if [ -n "$wppid" ] && [ "$wppid" != "1" ]; then
				echo "monitor_web.sh: reaping leaked webs child PID $wpid (PPid $wppid)"
				kill "$wpid" 2>/dev/null || true
			fi
		done
		memSize=`ps | awk '$5 ~ /(^|\/)webs$/ {if ($3+0 > max) max=$3+0} END {if (max > 0) print max}'`
		if [ -n "$memSize" ] && [ "$memSize" -gt 6000 ] 2>/dev/null; then
			echo "monitor_web.sh: webs VSZ ($memSize KB) exceeded threshold, triggering respawn via inittab"
			killall webs 2>/dev/null || true
		fi
	fi
	sleep 300
done