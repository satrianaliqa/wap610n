#!/bin/sh

# This init script sets up the routing to support the Metalink demo scenarios.
# If running in bridging mode, this script ignores the routing directives and 
# adds wlan0 to br0.

	# TODO: If scanning is active, will there be a wlan0 interface to configure?
	# In any case, it probably can't be brought up until there is a link 
	# (i.e. both bridging and routing config will fail)
	# - so make sure this script is run again from the CLI after a scan

BRIDGE_MODE=`awk -F "=" '/^BridgeMode/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`

# If BRIDGE_MODE doesn't exist, use old parameter names for detecting mac cloning
if [ ! $BRIDGE_MODE ]
then
	BRIDGE_MODE=`awk -F "=" '/^wlan_bridging/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`
fi

# Give the wireless some time to bring the interface up.
# This is someting that is only needed in wls versions 2.2+, and must be investigated [sometime].
sleep 2

if expr $BRIDGE_MODE = 0
then
	echo Configuring routing
	# Enable routing in the kernel
	echo "1" > /proc/sys/net/ipv4/ip_forward
	
	# Run the routing lines from the sys.conf file (using the awk system command)
	awk -F "=" '/^route[0-9]+/ {str = $2; sub(/ /, "", str); sub(/\r/, "", str); system(str)}' /tmp/sys.conf

	# TODO: Replace this mechanism, so that the routing lines are stored here, 
	# instead of in wlan.conf.
	# In this case, add 2 placeholders for the menu-config file parser, e.g.:
	### ROUTE TABLE BEGIN 
	### ROUTE TABLE END 

else
	echo Configuring wireless bridging

	# If mac cloning is used, give ifconfig a known MAC address, to allow easier debugging.
	MAC_CLONING=`awk -F "=" '/^MacCloningEnabled/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`

	if [ $MAC_CLONING ]
	then
		# If the mac cloning param exists, then we are using the old parameter names for mac cloning,
		# so modify BRIDGE_MODE to new convention
		# TODO: 
		# THIS IS A TEMPORARY WORKAROUND, BECAUSE THE ENUM VALUE OF BRIDGE MODE CHANGED 
		# BETWEEN 2.3.0 AND 2.3.5 BRANCHES.
		# DELETE ALL THIS MAC CLONING CODE WHEN 2.3.1 BRANCH DIES - 
		# THIS IS DANGEROUS CODE
		if expr $MAC_CLONING = 1
		then
			BRIDGE_MODE=3
		else
			BRIDGE_MODE=2
		fi
	fi
	
	if expr $WLAN_NUM_BANDS = 1 && expr $BRIDGE_MODE = 3
	then
        ifconfig wlan0 down
        ifconfig wlan0 hw ether 00:11:fe:ed:be:ef
        ifconfig wlan0 up
	fi

	if expr $WLAN_NUM_BANDS = 2
	then
		# In dual band mode, we previously put wlan interfaces under br1.
		# The interfaces should be in the LAN bridge.
		# TODO: instead of creating and removing from br1, don't create br1 unless in routing
		brctl delif br1 wlan0
		brctl delif br1 wlan1
		ifconfig br1 down
		brctl delbr br1

		brctl addif br0 wlan0
		brctl addif br0 wlan1
	else
		# Add wlan0 to the bridge
		ifconfig wlan0 0.0.0.0 
		brctl addif br0 wlan0
	fi
	
	
fi

