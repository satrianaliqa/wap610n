#!/bin/sh

# This script implements a mechanism to handle topology changes due to blocking of the eth0
# port in the bridge (Metalink's workaround to replace STP).
# This mechanism is active, even when STP is disabled.


# Enable loop breaking in the bridge (STP-like feature)
echo 1 > /sys/class/net/br0/bridge/loop_breaking


while [ 1 ]
do
	# Send pings to all learned devices. This is needed in case we broke a loop in the bridge, 
	# and need to teach the new route to connected devices.
	# (to notify topology change to neighbors)
	for ip in `awk '/[0-9]+\.[0-9]+\./ {print $1}' /proc/net/arp`
	do
		ping -c 1 "$ip" > /dev/null 2>&1
	done
	sleep 5
done





