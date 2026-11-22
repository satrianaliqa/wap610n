#!/bin/sh
DIAG_LOG=/bin/diag_log.sh

# Apply low-memory sysctl tunings
if [ -f /etc/sysctl.conf ] && [ -x /sbin/sysctl ]; then
	/sbin/sysctl -p /etc/sysctl.conf >/dev/null 2>&1 || true
fi

# Helper to read persisted sys.conf boolean flags (defaults to 1 if unset)
get_sys_conf_val() {
	key="$1"
	def="$2"
	val=""
	for cf in /mnt/jffs2/sys.conf /root/mtlk/etc/sys.conf; do
		if [ -f "$cf" ]; then
			val=$(awk -F= -v k="$key" '{
				lhs=$1; gsub(/^[ \t]+|[ \t]+$/, "", lhs);
				if (lhs == k) {
					rhs=$2; gsub(/^[ \t]+|[ \t\r]+$/, "", rhs);
					print rhs;
				}
			}' "$cf" 2>/dev/null | tail -n 1)
			[ -n "$val" ] && break
		fi
	done
	if [ -n "$val" ]; then
		echo "$val"
	else
		echo "$def"
	fi
}

TELNET_EN=$(get_sys_conf_val Telnet_Enabled 1)
SSH_EN=$(get_sys_conf_val SSH_Enabled 1)

# 1. Start Telnet daemon with root shell FIRST (Port 23) if enabled
if [ "$TELNET_EN" != "0" ] && ! pidof telnetd >/dev/null 2>&1; then
	$DIAG_LOG "early boot: starting Telnet daemon on port 23" 2>/dev/null || true
	telnetd -l /bin/sh &
fi

# 2. Prepare Dropbear SSH keys and start daemon (Port 22) if enabled
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
			if [ "$SSH_EN" != "0" ] && ! pidof dropbear >/dev/null 2>&1; then
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
