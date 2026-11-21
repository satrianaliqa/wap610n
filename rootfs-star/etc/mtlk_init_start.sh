#!/bin/sh
DIAG_LOG=/bin/diag_log.sh

# Apply low-memory sysctl tunings
if [ -f /etc/sysctl.conf ] && [ -x /sbin/sysctl ]; then
	/sbin/sysctl -p /etc/sysctl.conf >/dev/null 2>&1 || true
fi

# 1. Start Telnet daemon with root shell FIRST (Port 23)
if ! pidof telnetd >/dev/null 2>&1; then
	$DIAG_LOG "early boot: starting Telnet daemon on port 23" 2>/dev/null || true
	telnetd -l /bin/sh &
fi

# 2. Start Dropbear SSH daemon (Port 22) in background so keygen never blocks boot
if [ -x /usr/sbin/dropbear ] && [ -d /tmp ]; then
	(
		LOCKDIR=/tmp/dropbear_init.lock
		if mkdir "$LOCKDIR" 2>/dev/null; then
			trap 'rmdir "$LOCKDIR" 2>/dev/null || true' 0
			mkdir -p /etc/dropbear /mnt/jffs2/dropbear 2>/dev/null || true
			chmod 700 /etc/dropbear /mnt/jffs2/dropbear 2>/dev/null || true
			for keyname in dropbear_rsa_host_key dropbear_dss_host_key; do
				keytype=rsa
				[ "$keyname" = "dropbear_dss_host_key" ] && keytype=dss
				if [ -f "/mnt/jffs2/dropbear/$keyname" ]; then
					cp -f "/mnt/jffs2/dropbear/$keyname" "/etc/dropbear/$keyname" 2>/dev/null || true
				elif [ ! -f "/etc/dropbear/$keyname" ] && [ -x /usr/bin/dropbearkey ]; then
					/usr/bin/dropbearkey -t "$keytype" -s 1024 -f "/etc/dropbear/$keyname" >/dev/null 2>&1 || true
					if [ -f "/etc/dropbear/$keyname" ] && [ -d /mnt/jffs2/dropbear ]; then
						cp -f "/etc/dropbear/$keyname" "/mnt/jffs2/dropbear/$keyname" 2>/dev/null || true
						chmod 600 "/mnt/jffs2/dropbear/$keyname" 2>/dev/null || true
					fi
				fi
				[ -f "/etc/dropbear/$keyname" ] && chmod 600 "/etc/dropbear/$keyname" 2>/dev/null || true
			done
			if [ -f /mnt/jffs2/dropbear/authorized_keys ]; then
				mkdir -p /root/.ssh 2>/dev/null || true
				chmod 700 /root /root/.ssh 2>/dev/null || true
				cp -f /mnt/jffs2/dropbear/authorized_keys /root/.ssh/authorized_keys 2>/dev/null || true
				chmod 600 /root/.ssh/authorized_keys 2>/dev/null || true
			fi
			if ! pidof dropbear >/dev/null 2>&1; then
				$DIAG_LOG "early boot: starting Dropbear SSH daemon on port 22" 2>/dev/null || true
				/usr/sbin/dropbear -p 22 >/dev/null 2>&1 &
			fi
		fi
	) &
fi

# 3. Add a link for the RW fs mountpoint
ln -s /mnt/jffs2 /tmp/jffs2 2>/dev/null || true

# 4. Change to mtlk etc and run init scripts
cd /root/mtlk/etc
. ./mtlk_init.sh
