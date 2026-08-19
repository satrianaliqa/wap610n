#!/bin/sh
DHCPD_CONFIG_STRING=

DHCP_START_ADDRESS=`awk -F "=" '/^DHCPDStartAddress/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`
DHCP_END_ADDRESS=`awk -F "=" '/^DHCPDEndAddress/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`
DHCP_DNS1_ADDRESS=`awk -F "=" '/^DHCPDDNS1/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`
DHCP_DNS2_ADDRESS=`awk -F "=" '/^DHCPDDNS2/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`
DHCP_DNS3_ADDRESS=`awk -F "=" '/^DHCPDDNS3/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`
DHCP_SUBNET=`awk -F "=" '/^DHCPDSubnet/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`
DHCP_WINS=`awk -F "=" '/^DHCPDWINS/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`
DHCP_DOMAIN=`awk -F "=" '/^DHCPDDomain/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`
DHCP_LEASETIME=`awk -F "=" '/^DHCPDLeaseTime/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`
DHCP_GATEWAY=`awk -F "=" '/^ip_lan/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`

if [ $DHCP_START_ADDRESS ]
then
	DHCPD_CONFIG_STRING="$DHCPD_CONFIG_STRING start $DHCP_START_ADDRESS"
fi

if [ $DHCP_END_ADDRESS ]
then
	DHCPD_CONFIG_STRING="$DHCPD_CONFIG_STRING end $DHCP_END_ADDRESS"
fi

if [ $DHCP_DNS1_ADDRESS ]
then
	DHCPD_CONFIG_STRING="$DHCPD_CONFIG_STRING dns $DHCP_DNS1_ADDRESS"
fi

if [ $DHCP_DNS2_ADDRESS ]
then
	DHCPD_CONFIG_STRING="$DHCPD_CONFIG_STRING dns $DHCP_DNS2_ADDRESS"
fi

if [ $DHCP_DNS3_ADDRESS ]
then
	DHCPD_CONFIG_STRING="$DHCPD_CONFIG_STRING dns $DHCP_DNS3_ADDRESS"
fi

if [ $DHCP_SUBNET ]
then
	DHCPD_CONFIG_STRING="$DHCPD_CONFIG_STRING subnet $DHCP_SUBNET"
fi

if [ $DHCP_WINS ]
then
	DHCPD_CONFIG_STRING="$DHCPD_CONFIG_STRING wins $DHCP_WINS"
fi

if [ $DHCP_DOMAIN ]
then
	DHCPD_CONFIG_STRING="$DHCPD_CONFIG_STRING domain $DHCP_DOMAIN"
fi

if [ "$DHCP_LEASETIME" ]
then
	DHCPD_CONFIG_STRING="$DHCPD_CONFIG_STRING lease $DHCP_LEASETIME"
fi


if [ $DHCP_GATEWAY ]
then
	DHCPD_CONFIG_STRING="$DHCPD_CONFIG_STRING gw $DHCP_GATEWAY"
fi

./udhcpdconf.tcl ./udhcpd.conf $DHCPD_CONFIG_STRING

