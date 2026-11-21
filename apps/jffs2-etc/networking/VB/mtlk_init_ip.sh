#!/bin/sh
DIAG_LOG=/bin/diag_log.sh
# This is the script that configures all the LAN interfaces.
# It places all the interfaces under a bridge named br0.

# Read the networking params from sys.conf:
if [ -z "$IP_LAN" ]
then
	IP_LAN=`awk -F "=" '/^ip_lan/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`
fi

if [ -z "$IP_NETMASK" ]
then
	IP_NETMASK=`awk -F "=" '/^subnet_lan/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`
fi

if [ -z "$MAC_LAN" ]
then
	MAC_LAN=`awk -F "=" '/^mac_lan/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`
fi

IP_CONFIG_METHOD=`awk -F "=" '/^ip_config_method/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf 2>/dev/null`
DEFAULT_GW=`awk -F "=" '/^default_gw/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf 2>/dev/null`
DNS1=`awk -F "=" '/^DHCPDDNS1/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf 2>/dev/null`
DNS2=`awk -F "=" '/^DHCPDDNS2/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf 2>/dev/null`

# If the MAC address of the ETH interface is pre-defined in boot environment sector, use this value
if [ -e /bin/get_env_param ]
then
	/bin/get_env_param ethaddr > /dev/null
	if [ 0 = $? ]
	then
		MAC_LAN=`/bin/get_env_param ethaddr`
	fi
fi


# Make sure there is an assigned IP and netmask; if not, assign defaults.
if [ -z "$IP_LAN" ]
then
	IP_LAN=192.168.1.1
fi
if [ -z "$IP_NETMASK" ]
then
	IP_NETMASK=255.255.255.0
fi


# Create the bridge interface
if ! brctl addbr br0; then
	$DIAG_LOG "network init: failed to create br0" 2>/dev/null || true
	echo "network init: br0 creation failed" > /tmp/init_failed
	return 1
fi
# Remove the 30 second delay of the bridge interface:
brctl setfd br0 0


# Don't allow aging on the bridge 
# TEMPORARY WORKAROUND: increase ageing to about 10 days
# TODO: Replace with good mechanism for handling unidirectional traffic on bridge
brctl setageing br0 1000000


### Add all the LAN interfaces to the bridge
ifconfig eth0 0.0.0.0

# Override the default MAC addresses with values from the sys.conf file
if [ -n "$MAC_LAN" ]
then
	ifconfig eth0 down
	ifconfig eth0 hw ether "$MAC_LAN" up
fi
brctl addif br0 eth0


# Use smaller network queues if there is limited memory on this platform:
if [ "$LOWER_MEM_USAGE" = "1" ]
then
	ifconfig eth0 txqueuelen 256
	echo 500 > /proc/sys/net/core/netdev_max_backlog
fi

# Configure the bridge IP
if ! ifconfig br0 "$IP_LAN" netmask "$IP_NETMASK"; then
	$DIAG_LOG "network init: failed to assign $IP_LAN to br0" 2>/dev/null || true
	echo "network init: br0 address configuration failed" > /tmp/init_failed
	return 1
fi
echo "br0 has the IP $IP_LAN"

# Configure default gateway and DNS resolvers in static IP mode
if [ "$IP_CONFIG_METHOD" = "1" ]; then
	if [ -n "$DEFAULT_GW" ] && [ "$DEFAULT_GW" != "0.0.0.0" ] && [ "$DEFAULT_GW" != "$IP_LAN" ]; then
		route add default gw "$DEFAULT_GW" dev br0 2>/dev/null || true
	fi
	: > /tmp/resolv.conf
	if [ -n "$DNS1" ] && [ "$DNS1" != "0.0.0.0" ]; then
		echo "nameserver $DNS1" >> /tmp/resolv.conf
	fi
	if [ -n "$DNS2" ] && [ "$DNS2" != "0.0.0.0" ] && [ "$DNS2" != "$DNS1" ]; then
		echo "nameserver $DNS2" >> /tmp/resolv.conf
	fi
	if [ ! -s /tmp/resolv.conf ] && [ -n "$DEFAULT_GW" ] && [ "$DEFAULT_GW" != "0.0.0.0" ]; then
		echo "nameserver $DEFAULT_GW" >> /tmp/resolv.conf
	fi
fi

# Disable source address learning in the internal switch (if there is such an option)
./mtlk_SA_learning_disable.sh
