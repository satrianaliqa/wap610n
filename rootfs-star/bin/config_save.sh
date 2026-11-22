#!/bin/sh

# config_save.sh: flush and persist the configuration filesystem (/mnt/jffs2) to flash
DIAG_LOG=/bin/diag_log.sh

echo "Saving configuration filesystem"

if [ -f /mnt/jffs2/sys.conf ]; then
	for key in SSH_Enabled Telnet_Enabled WirelessMgmtEnabled; do
		if ! grep -q "^[[:space:]]*${key}[[:space:]]*=" /mnt/jffs2/sys.conf 2>/dev/null; then
			val=""
			if [ -f /tmp/sys.conf ]; then
				val=$(awk -F= -v k="$key" '{
					lhs=$1; gsub(/^[ \t]+|[ \t]+$/, "", lhs);
					if (lhs == k) {
						rhs=$2; gsub(/^[ \t]+|[ \t\r]+$/, "", rhs);
						print rhs;
					}
				}' /tmp/sys.conf 2>/dev/null | tail -n 1)
			fi
			[ -z "$val" ] && val=1
			echo "${key} = ${val}" >> /mnt/jffs2/sys.conf
		fi
	done
fi

if ! /bin/config_umount.sh
then
	echo "Configuration filesystem was not saved"
	$DIAG_LOG "config_save: configuration was not written to flash" 2>/dev/null || true
	exit 1
fi

if ! /bin/config_mount.sh
then
	echo "Configuration filesystem was saved but could not be remounted"
	$DIAG_LOG "config_save: flash write succeeded but remount failed" 2>/dev/null || true
	exit 1
fi

[ -f /mnt/jffs2/wlan0.conf ] && cp -f /mnt/jffs2/wlan0.conf /tmp/wlan0.conf 2>/dev/null || true
[ -f /mnt/jffs2/sys.conf ] && cp -f /mnt/jffs2/sys.conf /tmp/sys.conf 2>/dev/null || true
exit 0
