#!/bin/sh

# config_save.sh: flush and persist the configuration filesystem (/mnt/jffs2) to flash

echo "Saving configuration filesystem"

if ! /bin/config_umount.sh
then
	echo "Configuration filesystem was not saved"
	exit 1
fi

if ! /bin/config_mount.sh
then
	echo "Configuration filesystem was saved but could not be remounted"
	exit 1
fi

[ -f /mnt/jffs2/wlan0.conf ] && cp -f /mnt/jffs2/wlan0.conf /tmp/wlan0.conf 2>/dev/null || true
[ -f /mnt/jffs2/sys.conf ] && cp -f /mnt/jffs2/sys.conf /tmp/sys.conf 2>/dev/null || true
exit 0
