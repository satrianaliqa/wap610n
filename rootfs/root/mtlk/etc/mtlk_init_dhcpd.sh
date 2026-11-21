#!/bin/sh
DHCPD_CONFIG_STRING=

DHCP_START_ADDRESS=`awk -F "=" '/^DHCPDStartAddress/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`
DHCP_END_ADDRESS=`awk -F "=" '/^DHCPDEndAddress/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`
DHCP_DNS1_ADDRESS=`awk -F "=" '/^DHCPDDNS1/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`
DHCP_DNS2_ADDRESS=`awk -F "=" '/^DHCPDDNS2/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`
DHCP_DNS3_ADDRESS=`awk -F "=" '/^DHCPDDNS3/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`
DHCP_IP_LAN=`awk -F "=" '/^ip_lan/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`
[ -z "$DHCP_IP_LAN" ] && DHCP_IP_LAN="192.168.1.1"
DHCP_SUBNET=`awk -F "=" '/^DHCPDSubnet/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`
DHCP_WINS=`awk -F "=" '/^DHCPDWINS/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`
DHCP_DOMAIN=`awk -F "=" '/^DHCPDDomain/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`
DHCP_LEASETIME=`awk -F "=" '/^DHCPDLeaseTime/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`
DHCP_GATEWAY=`awk -F "=" '/^default_gw/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`
if [ -z "$DHCP_GATEWAY" ] || [ "$DHCP_GATEWAY" = "0.0.0.0" ]; then
	DHCP_GATEWAY=$DHCP_IP_LAN
fi

if [ $DHCP_START_ADDRESS ]
then
	DHCPD_CONFIG_STRING="$DHCPD_CONFIG_STRING start $DHCP_START_ADDRESS"
fi

if [ $DHCP_END_ADDRESS ]
then
	DHCPD_CONFIG_STRING="$DHCPD_CONFIG_STRING end $DHCP_END_ADDRESS"
fi

DHCP_DNS_COUNT=0
for dns_addr in "$DHCP_DNS1_ADDRESS" "$DHCP_DNS2_ADDRESS" "$DHCP_DNS3_ADDRESS"; do
	if [ -n "$dns_addr" ] && [ "$dns_addr" != "0.0.0.0" ] && [ "$dns_addr" != "$DHCP_IP_LAN" ]; then
		DHCPD_CONFIG_STRING="$DHCPD_CONFIG_STRING dns $dns_addr"
		DHCP_DNS_COUNT=1
	fi
done
if [ "$DHCP_DNS_COUNT" = "0" ]; then
	DHCPD_CONFIG_STRING="$DHCPD_CONFIG_STRING dns 1.1.1.1 dns 8.8.8.8"
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


if [ -n "$DHCP_GATEWAY" ]
then
	DHCPD_CONFIG_STRING="$DHCPD_CONFIG_STRING gw $DHCP_GATEWAY"
fi

/etc/udhcpd/udhcpdconf.tcl /etc/udhcpd/udhcpd.conf $DHCPD_CONFIG_STRING

