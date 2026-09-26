#!/bin/sh

# Apply low-memory sysctl tunings
if [ -f /etc/sysctl.conf ] && [ -x /sbin/sysctl ]; then
	/sbin/sysctl -p /etc/sysctl.conf >/dev/null 2>&1 || true
fi

# 1. Start Telnet daemon with root shell FIRST (Port 23)
if ! pidof telnetd >/dev/null 2>&1; then
	echo "--> [Early Boot] Starting Telnet daemon on Port 23..." > /dev/console
	telnetd -l /bin/sh &
fi

# 2. Start Dropbear SSH daemon (Port 22)
if [ -x /usr/sbin/dropbear ]; then
	mkdir -p /etc/dropbear
	chmod 700 /etc/dropbear 2>/dev/null || true
	if [ ! -f /etc/dropbear/dropbear_rsa_host_key ] && [ -x /usr/bin/dropbearkey ]; then
		/usr/bin/dropbearkey -t rsa -s 1024 -f /etc/dropbear/dropbear_rsa_host_key 2>/dev/null || true
	fi
	if [ ! -f /etc/dropbear/dropbear_dss_host_key ] && [ -x /usr/bin/dropbearkey ]; then
		/usr/bin/dropbearkey -t dss -s 1024 -f /etc/dropbear/dropbear_dss_host_key 2>/dev/null || true
	fi
	if ! pidof dropbear >/dev/null 2>&1; then
		echo "--> [Early Boot] Starting Dropbear SSH daemon on Port 22..." > /dev/console
		/usr/sbin/dropbear -p 22 -B &
	fi
fi

# 3. Start Hardware WPS Safe-Shutdown Monitor daemon
if [ -f /root/mtlk/etc/WPS_PBC.sh ]; then
	/root/mtlk/etc/WPS_PBC.sh &
fi

# 4. Add a link for the RW fs mountpoint
ln -s /mnt/jffs2 /tmp/jffs2 2>/dev/null || true

# 5. Change to mtlk etc and run init scripts
cd /root/mtlk/etc
. ./mtlk_init.sh
