#!/bin/sh

# This init script sets up the routing to support the Metalink demo scenarios.
# If running in bridging mode, this script ignores the routing directives and 
# adds wlan0 to br0.
# For the VB application, bridge mode will always be set.


# Use a passed argument to decide if this is the first or second call to this script.
# This is used in mac cloning, to do different operations before and after the supplicant is brought up
ROUTE_PASS_NUM=$1

if [ ! $ROUTE_PASS_NUM ]
then
	ROUTE_PASS_NUM=1
fi

if [ ! $BRIDGE_MODE ]
then
	BRIDGE_MODE=`awk -F "=" '/^BridgeMode/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`
fi

# If BRIDGE_MODE doesn't exist, use old parameter names for detecting mac cloning
if [ ! $BRIDGE_MODE ]
then
	BRIDGE_MODE=`awk -F "=" '/^wlan_bridging/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`
fi

# Give the wireless some time to bring the interface up.
# This is someting that is only needed in wls versions 2.2+, and must be investigated [sometime].
#sleep 2

if [ $BRIDGE_MODE == 0 ]
then
	echo Configuring routing
	# Enable routing in the kernel
	echo "1" > /proc/sys/net/ipv4/ip_forward
	
	# Run the routing lines from the sys.conf file (using the awk system command)
	awk -F "=" '/^route[0-9]+/ {str = $2; sub(/ /, "", str); sub(/\r/, "", str); system(str)}' /tmp/sys.conf

else
	echo Configuring wireless bridging

	if [ $ROUTE_PASS_NUM == 1 ]
	then
		# This is the first time the route script is run
		# Add wlan0 to the bridge
		ifconfig wlan0 0.0.0.0 
		brctl addif br0 wlan0
	else 
		# This is the second pass - set up MAC addr for mac cloning

		# If mac cloning is used, give ifconfig a known MAC address, to allow easier debugging.
		if [ ! $MAC_CLONING ]
		then
			MAC_CLONING=`awk -F "=" '/^MacCloningEnabled/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`
		fi
		
		if [ $MAC_CLONING ]
		then
			# If the mac cloning param exists, then we are using the old parameter names for mac cloning,
			# so modify BRIDGE_MODE to new convention
			# TODO: 
			# THIS IS A TEMPORARY WORKAROUND, BECAUSE THE ENUM VALUE OF BRIDGE MODE CHANGED 
			# BETWEEN 2.3.0 AND 2.3.5 BRANCHES.
			# DELETE ALL THIS MAC CLONING CODE WHEN 2.3.1 BRANCH DIES 
			if expr $MAC_CLONING = 1
			then
				BRIDGE_MODE=3
			else
				BRIDGE_MODE=2
			fi
		fi

		# In MAC cloning mode, give the interface a fake and well-know MAC address.
		# (The cloned MAC is used only by the wls driver)	
		if [ $WLAN_NUM_BANDS == 1 ] && [ $BRIDGE_MODE == 3 ]
		then
			ifconfig wlan0 down
			ifconfig wlan0 hw ether 00:11:fe:ed:be:ef
			ifconfig wlan0 up
		fi

	fi
fi

