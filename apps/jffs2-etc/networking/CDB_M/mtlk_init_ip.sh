#!/bin/sh
# This is the script that configures all the LAN interfaces.
# It places all the interfaces under a bridge named br0.

# This is how to read the IP without caring about whether there are spaces after the "=":
IP_LAN=`awk -F "=" '/^ip_lan/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`
IP_NETMASK=`awk -F "=" '/^subnet_lan/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`
MAC_LAN=`awk -F "=" '/^mac_lan/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`
MAC_WAN=`awk -F "=" '/^mac_wan/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`
MAC_USB=`awk -F "=" '/^mac_usb/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`

# Make sure that sys.conf has the network values. If not, get from the old wlan.conf
# This is important when first upgrading from an old CLI (all parameters in wlan.conf) to a new CLI (wlan.conf + sys.conf)
IP_WC=`echo $IP_LAN | wc -c`
if expr $IP_WC = 1 
then
	IP_LAN=`awk -F "=" '/^ip_lan/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/wlan.conf`
	IP_NETMASK=`awk -F "=" '/^subnet_lan/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/wlan.conf`
	MAC_LAN=`awk -F "=" '/^mac_lan/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/wlan.conf`
	MAC_WAN=`awk -F "=" '/^mac_wan/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/wlan.conf`
	MAC_USB=`awk -F "=" '/^mac_usb/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/wlan.conf`
fi

# If the MAC address of the ETH interface is pre-defined in boot environment sector, use this values
if [ -e /bin/get_env_param ]
then
	/bin/get_env_param ethaddr > /dev/null
	if [ 0 = $? ]
	then
		MAC_LAN=`/bin/get_env_param ethaddr`
	fi
fi


# Make sure there is a valid IP, if not, assign the default.
# TODO: A better check would be a regexp for a valid IP, instead of a word count
IP_WC=`echo $IP_LAN | wc -c`
if expr $IP_WC = 1 
then 
	IP_LAN=192.168.1.1
	IP_NETMASK=255.255.255.0
fi


# Bring up the Gigabit ethernet interface, if it exists
# TODO: Test the PCI before doing the insmod?
modprobe e1000 2> /dev/null

brctl addbr br0
# Remove the 30 second delay of the bridge interface:
brctl setfd br0 0

# Don't allow aging on the bridge 
# TEMPORARY WORKAROUND: increase ageing to about 10 days
# TODO: Replace with good mechanism for handling unidirectional traffic on bridge
brctl setageing br0 1000000


# Add all the LAN interfaces to the bridge


ifconfig eth0 0.0.0.0

# Use smaller network queues if there is limited memory on this platform:
if expr $LOWER_MEM_USAGE = 1
then
	ifconfig eth0 txqueuelen 256
	echo 500 > /proc/sys/net/core/netdev_max_backlog
fi

# Override the default MAC addresses with values from the sys.conf file
MAC_L_WC=`echo $MAC_LAN | wc -c`
if expr $MAC_L_WC != 1
then
	ifconfig eth0 down
	ifconfig eth0 hw ether $MAC_LAN up
fi
brctl addif br0 eth0

ETH1=`ifconfig -a | grep eth1 | wc -l`
if expr $ETH1 \> 0
then 
	ifconfig eth1 0.0.0.0

	MAC_W_WC=`echo $MAC_WAN | wc -c`
	if expr $MAC_W_WC != 1
	then
		ifconfig eth1 down
		ifconfig eth1 hw ether $MAC_WAN up
	fi
	brctl addif br0 eth1
fi

ETH2=`ifconfig -a | grep eth2 | wc -l`
if expr $ETH2 \> 0
then 
	ifconfig eth2 0.0.0.0
	brctl addif br0 eth2
fi

USB=`ifconfig -a | grep usb0 | wc -l`
if expr $USB \> 0
then 
	ifconfig usb0 0.0.0.0

	MAC_U_WC=`echo $MAC_USB | wc -c`
	if expr $MAC_U_WC != 1
	then
		ifconfig usb0 down
		ifconfig usb0 hw ether $MAC_USB up
	fi
	brctl addif br0 usb0
fi

# Don't allow sending multicast packets up the bridge
## (Commented out, because currently this is dropped by using iptables)
## ifconfig br0 -multicast
ifconfig br0 $IP_LAN netmask $IP_NETMASK


echo "br0 has the IP $IP_LAN" 

# Uncomment this to enable dhcpd 
# TODO: This requires more support, e.g. configure IP range from menu, config file...
#dhcpcd -t 0 br0 &


# Resolve cpu utilization under multicast traffic (by dropping multicast packets going up the stack)
if expr $MULTICAST_MANGLING = 1
then
	iptables -t mangle -A PREROUTING -d 224.0.0.0/4 -j DROP
fi

