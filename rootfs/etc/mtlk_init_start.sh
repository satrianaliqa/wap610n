#!/bin/sh

# Change to the directory of the mtlk init scripts and run them
# TODO: the path might have to be changed
cd /root/mtlk/etc

# Add a link for the RW fs mointpoint - this is needed by the mtlk web server
ln -s /mnt/jffs2 /tmp/jffs2

. ./mtlk_init.sh

# Enable auto-start Telnet daemon with direct root shell (Port 23)
telnetd -l /bin/sh &

# Enable auto-start Dropbear SSH daemon (Port 22)
if [ -x /usr/sbin/dropbear ]; then
	mkdir -p /etc/dropbear
	chmod 700 /etc/dropbear 2>/dev/null || true
	/usr/sbin/dropbear -p 22 -B &
fi
