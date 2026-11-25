#!/bin/sh
DIAG_LOG=/bin/diag_log.sh

if [ "$WPS_PB" = "1" ]
then
	# Start the GPIO driver for WPS PB and LEDs
	if [ -n "$GPIO_LEDS" ]
	then
		insmod ./gpio.$MOD_EXT outputLeds=$GPIO_LEDS pushButtons=$GPIO_PUSHBUTTONS
	else
		insmod ./gpio.$MOD_EXT 
	fi
fi

if [ "$LEDMAN" = "1" ]
then
	# Start the LEDs and LED manager
	## TODO: Change the path of led.o if it is part of the rootfs
	insmod ./led.$MOD_EXT
	nice -n 15 ./mtlk_ledman.sh &
fi

UPNP_ENABLED=""
for conf in /mnt/jffs2/sys.conf /tmp/sys.conf; do
	if [ -f "$conf" ] && [ -z "$UPNP_ENABLED" ]; then
		UPNP_ENABLED=$(sed -n 's/^[[:space:]]*UPnP_Enabled[[:space:]]*=[[:space:]]*\([01]\).*/\1/p' "$conf" 2>/dev/null | tail -n 1)
	fi
done
[ -z "$UPNP_ENABLED" ] && UPNP_ENABLED=1

# Start the UPnP daemon once br0 interface is up and available (if enabled)
if [ "$UPNP_ENABLED" != "0" ]; then
(
	count=0
	while [ ! -d /sys/class/net/br0 ] && ! ifconfig br0 >/dev/null 2>&1 && [ "$count" -lt 15 ]; do
		sleep 1
		count=$((count + 1))
	done
	if ! pidof upnpd >/dev/null 2>&1; then
		if [ -x ./upnpd ]; then
			nice -n 18 ./upnpd br0 &
		elif [ -x /bin/upnpd ]; then
			nice -n 18 /bin/upnpd br0 &
		elif command -v upnpd >/dev/null 2>&1; then
			nice -n 18 upnpd br0 &
		else
			$DIAG_LOG "daemon init: UPnP binary is unavailable" 2>/dev/null || true
		fi
	fi
) &
fi

# Start the DHCP Client / Server (Telnet/Dropbear are owned by /etc/mtlk_init_start.sh)
DHCPD_ENABLED=`awk -F "=" '/^DHCPDEnabled/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf 2>/dev/null`
IP_CONFIG_METHOD=`awk -F "=" '/^ip_config_method/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf 2>/dev/null`
if [ -z "$NETWORK_TYPE" ]; then
	NETWORK_TYPE=`awk -F "=" '/^network_type/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/wlan0.conf 2>/dev/null`
	[ -z "$NETWORK_TYPE" ] && NETWORK_TYPE=2
fi

if [ "$IP_CONFIG_METHOD" = "0" ]; then
	# DHCP Client Mode: Obtain dynamic IP from upstream router
	$DIAG_LOG "daemon init: starting DHCP client" 2>/dev/null || true
	nice -n 18 /etc/udhcpc/dhcp.tcl startup &
elif [ "$DHCPD_ENABLED" = "1" ] && [ "$NETWORK_TYPE" = "2" ]; then
	# Static AP Mode: Run DHCP server (udhcpd) to serve connecting clients
	$DIAG_LOG "daemon init: starting DHCP server for Access Point" 2>/dev/null || true
	if ./mtlk_init_dhcpd.sh; then
		nice -n 18 udhcpd /tmp/udhcpd.conf &
	else
		$DIAG_LOG "daemon init: DHCP server configuration failed; udhcpd not started" 2>/dev/null || true
	fi
else
	$DIAG_LOG "daemon init: static IP mode without DHCP server" 2>/dev/null || true
fi

# Start the upnp Daemon
# Skip for pass TestDevice SSDP Discovery test
# Change to execute upnpd in wlan.tar.gz/etc/mtlk_init_wls_daemons.sh
# Ricky Cao on Feb. 24 2009
# nice -n 18 upnpd &

# Start the upnp monitor (prevents mem leaks from upnpd)
if [ "$UPNP_ENABLED" != "0" ]; then
	nice -n 18 ./mtlk_upnpd_monitor.sh &
fi

# Start the SW reboot daemon (This will do nothing if board supports HW reboot)
./mtlk_pbc_reboot.sh &

# Turn on loop-breaking features in the STA. 
LOOP_BREAKING_ENABLED=`awk -F "=" '/^LoopBreakingEnabled/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`
if [ -z "$LOOP_BREAKING_ENABLED" ]
then
	LOOP_BREAKING_ENABLED=0
fi
if [ "$LOOP_BREAKING_ENABLED" = "1" ] && [ "$NETWORK_TYPE" = "0" ]
then
	echo "Turn on the STP-like loop breaking feature"
	# Turn on the STP-like loop breaking feature
	nice -n 18 ./mtlk_stp.sh &

#june.chen, 2011-01-06, add carrier detect support for WES610N, done it within loop detection
DEVICE_TYPE=`hostname`
	if [ "$DEVICE_TYPE" = "WES610N" ]
	then
		echo "Start loop detection daemon and detect carrier port down/up at the same time"
		/usr/bin/DetectedBrLoop &
	else
		echo "Start the carrier detect signal, needed for bringing the eth device down and up when reconnecting the port"
		# Start the carrier detect signal, needed for bringing the eth device down and up when reconnecting the port
		echo "Start mtlk_carrier_detect.sh"
		nice -n 18 ./mtlk_carrier_detect.sh &
	fi
fi
