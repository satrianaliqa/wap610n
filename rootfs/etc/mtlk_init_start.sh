#!/bin/sh

# 1. Start Telnet daemon with root shell FIRST (Port 23)
echo "--> [Early Boot] Starting Telnet daemon on Port 23..." > /dev/console
telnetd -l /bin/sh &

# 2. Start Dropbear SSH daemon (Port 22)
if [ -x /usr/sbin/dropbear ]; then
	mkdir -p /etc/dropbear
	chmod 700 /etc/dropbear 2>/dev/null || true
	echo "--> [Early Boot] Starting Dropbear SSH daemon on Port 22..." > /dev/console
	/usr/sbin/dropbear -p 22 -B &
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
