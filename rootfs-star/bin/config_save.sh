#!/bin/sh

# config_save.sh: flush and persist the configuration filesystem (/mnt/jffs2) to flash
DIAG_LOG=/bin/diag_log.sh

echo "Saving configuration filesystem"

if [ -f /mnt/jffs2/sys.conf ]; then
	for key in SSH_Enabled Telnet_Enabled WirelessMgmtEnabled UPnP_Enabled SSHJ_Enabled SSHJ_User SSHJ_Host; do
		if ! grep -q "^[ \t]*${key}[ \t]*=" /mnt/jffs2/sys.conf 2>/dev/null; then
			val=""
			found=0
			if [ -f /tmp/sys.conf ]; then
				val=$(awk -v k="$key" '
					$0 ~ "^[ \t]*" k "[ \t]*=" {
						idx = index($0, "=")
						v = substr($0, idx + 1)
						sub(/^[ \t]+/, "", v)
						sub(/[ \t\r]+$/, "", v)
						print v
						f = 1
					}
					END { if (!f) exit 1 }
				' /tmp/sys.conf 2>/dev/null | tail -n 1) && found=1
			fi
			if [ "$found" = "0" ]; then
				case "$key" in
					UPnP_Enabled|SSHJ_Enabled) val="0" ;;
					SSHJ_User) val="" ;;
					SSHJ_Host) val="wap610n" ;;
					*) val="1" ;;
				esac
			fi
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
