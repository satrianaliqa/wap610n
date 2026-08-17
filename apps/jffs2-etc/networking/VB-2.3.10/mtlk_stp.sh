#!/bin/sh

# This script implements a mechanism to handle topology changes due to blocking of the eth0
# port in the bridge (Metalink's workaround to replace STP).
# This mechanism is active, even when STP is disabled.


# Enable loop breaking in the bridge (STP-like feature), if it exists.
# Ignore errors, because this is only on STAR platforms, not dongle
echo 1 2> /dev/null > /sys/class/net/br0/bridge/loop_breaking 


while [ 1 ]
do
	# Send pings to all learned devices. This is needed in case we broke a loop in the bridge, 
	# and need to teach the new route to connected devices.
	# (to notify topology change to neighbors)
	for ip in `awk '/[0-9]+\.[0-9]+\./ {print $1}' /proc/net/arp`
	do
		# Try issuing ping with args (full busybox syntax)
		ping -c 1 $ip > /dev/null 2>/dev/null
		if [ ! $? = 0 ]
		then
			# Issue ping without args (light busybox syntax)
			ping $ip
		fi		
	done
	sleep 5
done





