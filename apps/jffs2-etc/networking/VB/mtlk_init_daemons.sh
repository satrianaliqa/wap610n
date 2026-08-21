#!/bin/sh

if [ $WPS_PB == 1 ]
then
	# Start the GPIO driver for WPS PB and LEDs
	if [ $GPIO_LEDS ] 
	then
		insmod ./gpio.$MOD_EXT outputLeds=$GPIO_LEDS pushButtons=$GPIO_PUSHBUTTONS
	else
		insmod ./gpio.$MOD_EXT 
	fi
fi

if [ $LEDMAN == 1 ]
then
	# Start the LEDs and LED manager
	## TODO: Change the path of led.o if it is part of the rootfs
	insmod ./led.$MOD_EXT
	nice -n 15 ./mtlk_ledman.sh &
fi

# Start the upnp Daemon
nice -n 18 upnpd &

# Start Telnet and Dropbear SSH daemons for remote terminal access & diagnosis
if [ -x /usr/sbin/telnetd ]; then
	echo "Starting Telnet daemon (port 23)..." > /dev/console
	telnetd -l /bin/sh &
fi

if [ -x /usr/sbin/dropbear ]; then
	echo "Starting Dropbear SSH daemon (port 22)..." > /dev/console
	mkdir -p /etc/dropbear
	if [ ! -f /etc/dropbear/dropbear_rsa_host_key ]; then
		/usr/bin/dropbearkey -t rsa -f /etc/dropbear/dropbear_rsa_host_key 2>/dev/null || true
	fi
	if [ ! -f /etc/dropbear/dropbear_dss_host_key ]; then
		/usr/bin/dropbearkey -t dss -f /etc/dropbear/dropbear_dss_host_key 2>/dev/null || true
	fi
	/usr/sbin/dropbear -B &
fi

# Start the DHCP Client
nice -n 18 /etc/udhcpc/dhcp.tcl startup &

# Start the DHCP Server
DHCPD_ENABLED=`awk -F "=" '/^DHCPDEnabled/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`
if [ $DHCPD_ENABLED ]
then
	if [ $DHCPD_ENABLED == 1 ]
	then
		./mtlk_init_dhcpd.sh 
		nice -n 18 udhcpd /tmp/udhcpd.conf
	fi
fi

# Start the upnp Daemon
# Skip for pass TestDevice SSDP Discovery test
# Change to execute upnpd in wlan.tar.gz/etc/mtlk_init_wls_daemons.sh
# Ricky Cao on Feb. 24 2009
# nice -n 18 upnpd &

# Start the upnp monitor (prevents mem leaks from upnpd)
nice -n 18 ./mtlk_upnpd_monitor.sh &

# Start the SW reboot daemon (This will do nothing if board supports HW reboot)
./mtlk_pbc_reboot.sh &

# Turn on loop-breaking features in the STA. 
LOOP_BREAKING_ENABLED=`awk -F "=" '/^LoopBreakingEnabled/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`
if [ ! $LOOP_BREAKING_ENABLED ] 
then
	LOOP_BREAKING_ENABLED=0
fi
if [ $LOOP_BREAKING_ENABLED -eq 1 -a $NETWORK_TYPE -eq 0 ]
then
	echo "Turn on the STP-like loop breaking feature"
	# Turn on the STP-like loop breaking feature
	nice -n 18 ./mtlk_stp.sh &

#june.chen, 2011-01-06, add carrier detect support for WES610N, done it within loop detection
DEVICE_TYPE=`hostname`
	if [ $DEVICE_TYPE == "WES610N"  ]
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
