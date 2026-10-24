#!/bin/sh
# Fallback safety timer for WebUI configuration apply.
# Normally childFinishAndExit() in webs removes /var/old_web_pid and sends SIGHUP
# to the parent webs process immediately upon completion.

sleep 15
if [ -f /var/old_web_pid ]; then
	OLD_PID=`cat /var/old_web_pid 2>/dev/null`
	rm -f /var/old_web_pid
	if [ -n "$OLD_PID" ]; then
		kill -HUP "$OLD_PID" 2>/dev/null || true
	fi
fi