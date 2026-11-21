#!/bin/sh

# umount_config.sh
#
# Flushes the configuration filesystem (/mnt/jffs2 backed by /tmp/fs.img)
# and saves it to /dev/mtdblock4 without unmounting /mnt/jffs2.

CONFIG_BLOCK=/dev/mtdblock4
CONFIG_FILE=/tmp/fs.img
CONFIG_GZ=${CONFIG_FILE}.gz
CONFIG_MNT=/mnt/jffs2
CONFIG_MAX_SIZE=131072
DIAG_LOG=/bin/diag_log.sh

# Obtain config lock to serialize all flash/loopback config operations
config_lock.sh $$
if [ $? -ne 0 ]
then
	echo " ($$) config_umount: Could not obtain config lock!"
	$DIAG_LOG "config_umount: could not obtain configuration lock" 2>/dev/null || true
	logger -t $$ "config_umount: Could not obtain config lock!"
	exit 1
fi

REMOUNTED_RO=0

cleanup_and_unlock() {
	if [ "$REMOUNTED_RO" -eq 1 ]; then
		mount -o remount,rw "$CONFIG_MNT" 2>/dev/null || true
		REMOUNTED_RO=0
	fi
	rm -f "$CONFIG_GZ" 2>/dev/null
	config_unlock.sh $$ >/dev/null 2>&1 || true
}
trap 'cleanup_and_unlock' 0
trap 'exit 130' 1 2 15

if [ ! -f "$CONFIG_FILE" ]
then
	echo " ($$) config_umount: $CONFIG_FILE does not exist!"
	$DIAG_LOG "config_umount: configuration image is missing" 2>/dev/null || true
	exit 1
fi

# Flush dirty pages of the loopback mount into /tmp/fs.img without unmounting /mnt/jffs2
# so concurrent readers (/mnt/jffs2/wlan0.conf, /mnt/jffs2/sys.conf) never see an empty mountpoint.
sync
if grep -q " $CONFIG_MNT " /proc/mounts 2>/dev/null
then
	if ! mount -o remount,ro "$CONFIG_MNT" 2>/dev/null; then
		sleep 1
		sync
		if ! mount -o remount,ro "$CONFIG_MNT" 2>/dev/null; then
			echo " ($$) Failed remounting $CONFIG_MNT read-only before flash save"
			$DIAG_LOG "config_umount: remount ro failed; aborting flash write" 2>/dev/null || true
			exit 1
		fi
	fi
	REMOUNTED_RO=1
	sync
fi

rm -f "$CONFIG_GZ" 2>/dev/null
if ! gzip -c "$CONFIG_FILE" > "$CONFIG_GZ"
then
	echo " ($$) Failed compressing configuration filesystem"
	$DIAG_LOG "config_umount: failed compressing configuration filesystem" 2>/dev/null || true
	logger -t $$ "Failed compressing configuration filesystem"
	exit 1
fi

if [ "$REMOUNTED_RO" -eq 1 ]
then
	mount -o remount,rw "$CONFIG_MNT" 2>/dev/null || true
	REMOUNTED_RO=0
fi

# Never start an erase/write cycle with an image larger than mtdblock4 (128 KB).
CONFIG_SIZE=`wc -c < "$CONFIG_GZ"`
if [ "$CONFIG_SIZE" -gt "$CONFIG_MAX_SIZE" ]
then
	echo " ($$) Configuration image is too large: $CONFIG_SIZE bytes (max $CONFIG_MAX_SIZE)"
	$DIAG_LOG "config_umount: configuration image exceeds flash limit" 2>/dev/null || true
	logger -t $$ "Configuration image is too large: $CONFIG_SIZE bytes"
	exit 1
fi

# Skip NOR flash erase/write if /dev/mtdblock4 already contains the exact compressed image
if simpdiff -s "$CONFIG_GZ" "$CONFIG_BLOCK" >/dev/null 2>&1
then
	echo " ($$) Configuration on flash is already up to date"
	exit 0
fi

echo " ($$) Saving configuration filesystem to flash"
if ! cp "$CONFIG_GZ" "$CONFIG_BLOCK"
then
	echo " ($$) Failed writing configuration filesystem to flash"
	$DIAG_LOG "config_umount: failed writing configuration filesystem to MTD4" 2>/dev/null || true
	logger -t $$ "Failed writing configuration filesystem to flash"
	exit 1
fi
sync

# Validate image integrity - binary diff of tmp gzip file and flash
CP_RETRY_COUNT=0
simpdiff -s "$CONFIG_GZ" "$CONFIG_BLOCK"
while [ $? -gt 0 ]
do
	CP_RETRY_COUNT=`expr $CP_RETRY_COUNT + 1`
	if [ $CP_RETRY_COUNT -gt 5 ]
	then
		echo " ($$) Failed copying configuration filesystem to flash!"
		$DIAG_LOG "config_umount: flash verification failed after retries" 2>/dev/null || true
		logger -t $$ "Failed copying configuration filesystem to flash!"
		exit 1
	fi

	if ! cp "$CONFIG_GZ" "$CONFIG_BLOCK"
	then
		echo " ($$) Retry failed writing configuration filesystem to flash"
		logger -t $$ "Retry failed writing configuration filesystem to flash"
		exit 1
	fi
	sync
	simpdiff -s "$CONFIG_GZ" "$CONFIG_BLOCK"
done

echo " ($$) Configuration saved (in $CP_RETRY_COUNT retry attempts)"
exit 0
