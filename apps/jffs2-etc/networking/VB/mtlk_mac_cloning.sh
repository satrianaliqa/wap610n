#!/bin/sh

# This script sniffs an ethernet packet and uses it for mac cloning.
# It is run in the background, to allow the system to complete initialization, even before a first packet is retrieved


# There is no fixed address, so sniff the LAN ports to find the first MAC
# Get the Mac address of br0
DST_MAC=`get_env_param ethaddr | tr [:lower:] [:upper:]`
if [ ! $DST_MAC ]
then
	DST_MAC=`ifconfig br0 | awk 'NR<2 {print $5}'`
fi

# Get the Mac address of the connected device (the future cloned mac address)
# Make sure the sniffed packet is on the ethernet port 
# (ASSUMPTION: always port 1 in brctl)
SNIFFED_PORT=
while [ -z "$SNIFFED_PORT" ] || [ "$SNIFFED_PORT" = "2" ]
do
	if [ -e /bin/etherdump ]
	then
		# etherdump sniff for the first packet
		MAC_WLAN=`etherdump -i br0 -h | awk -f /root/mtlk/etc/etherdump_awk -v var=$DST_MAC`
	else
		# tcpdump sniff for the first packet
		MAC_WLAN=`tcpdump -i br0 -ec1 ether dst $DST_MAC or broadcast | awk '{print $2}'`
		# change the mac address to upper case
		MAC_WLAN=`echo $MAC_WLAN | tr "[a-z]" "[A-Z]"`
	fi
	
	# Find ount which port the packet was sniffed from
	SNIFFED_PORT=`brctl showmacs br0 | grep -i $MAC_WLAN | awk '{print $1}'`
done

# write the mac cloning address to a file for future use
echo $MAC_WLAN > /tmp/mac_cloning.addr
