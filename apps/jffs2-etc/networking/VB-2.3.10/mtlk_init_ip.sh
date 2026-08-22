#!/bin/sh
# This is the script that configures all the LAN interfaces.
# It places all the interfaces under a bridge named br0.

# Read the networking params from sys.conf:
if [ ! $IP_LAN ]
then
	IP_LAN=`awk -F "=" '/^ip_lan/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`
fi

if [ ! $IP_NETMASK ]
then
IP_NETMASK=`awk -F "=" '/^subnet_lan/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`
fi
if [ ! $MAC_LAN ]
then
	MAC_LAN=`awk -F "=" '/^mac_lan/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`
fi

if [ ! $MAC_USB ]
then
	MAC_USB=`awk -F "=" '/^mac_usb/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`
fi

# If the MAC address of the ETH interface is pre-defined in boot environment sector, use this value
if [ -e /bin/get_env_param ]
then
	/bin/get_env_param ethaddr > /dev/null
	if [ 0 = $? ]
	then
		MAC_LAN=`/bin/get_env_param ethaddr`
	fi
fi


# Make sure there is an assigned IP, if not, assign the default.
if [ ! $IP_LAN ] 
then 
	IP_LAN=192.168.1.1
	IP_NETMASK=255.255.255.0
fi


# Bring up the Gigabit ethernet interface, if it exists
# TODO: Test the PCI before doing the insmod?
modprobe e1000 2> /dev/null

# Create the bridge interface
brctl addbr br0
# Remove the 30 second delay of the bridge interface:
brctl setfd br0 0


# Don't allow aging on the bridge 
# TEMPORARY WORKAROUND: increase ageing to about 10 days
# TODO: Replace with good mechanism for handling unidirectional traffic on bridge
brctl setageing br0 1000000


### Add all the LAN interfaces to the bridge
ifconfig eth0 0.0.0.0

# Override the default MAC addresses with values from the sys.conf file
if [ $MAC_LAN ] 
then
	ifconfig eth0 down
	ifconfig eth0 hw ether $MAC_LAN up
fi
brctl addif br0 eth0


# Use smaller network queues if there is limited memory on this platform:
if [ $LOWER_MEM_USAGE = 1 ]
then
	ifconfig eth0 txqueuelen 256
	echo 500 > /proc/sys/net/core/netdev_max_backlog
fi

ETH1=`ifconfig -a | grep eth1 | wc -l`
if [ $ETH1 \> 0 ]
then 
	ifconfig eth1 0.0.0.0

	if [ $MAC_WAN ]
	then
		ifconfig eth1 down
		ifconfig eth1 hw ether $MAC_WAN up
	fi
	brctl addif br0 eth1
fi

ETH2=`ifconfig -a | grep eth2 | wc -l`
if [ $ETH2 \> 0 ]
then 
	ifconfig eth2 0.0.0.0
	brctl addif br0 eth2
fi

USB=`ifconfig -a | grep usb0 | wc -l`
if [ $USB \> 0 ]
then 
	ifconfig usb0 0.0.0.0

	if [ $MAC_USB ]
	then
		ifconfig usb0 down
		ifconfig usb0 hw ether $MAC_USB up
	fi
	brctl addif br0 usb0
fi

# Don't allow sending multicast packets up the bridge
## (Commented out, because currently this is dropped by using iptables)
## ifconfig br0 -multicast

# Configure the bridge IP
ifconfig br0 $IP_LAN netmask $IP_NETMASK


echo "br0 has the IP $IP_LAN" 

# Resolve cpu utilization under multicast traffic (by dropping multicast packets going up the stack)
if [ $MULTICAST_MANGLING = 1 ]
then
	iptables -t mangle -A PREROUTING -d 224.0.0.0/4 -j DROP
fi

# Disable source address learning in the internal switch (if there is such an option)
./mtlk_SA_learning_disable.sh

