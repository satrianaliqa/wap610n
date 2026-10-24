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

# Obtain config lock to serialize all flash/loopback config operations
config_lock.sh $$
if [ $? -ne 0 ]
then
	echo " ($$) config_umount: Could not obtain config lock!"
	logger -t $$ "config_umount: Could not obtain config lock!"
	exit 1
fi

cleanup_and_unlock() {
	rm -f "$CONFIG_GZ" 2>/dev/null
	config_unlock.sh $$ >/dev/null 2>&1 || true
}
trap 'cleanup_and_unlock' 0 1 2 15

if [ ! -f "$CONFIG_FILE" ]
then
	echo " ($$) config_umount: $CONFIG_FILE does not exist!"
	exit 1
fi

# Flush dirty pages of the loopback mount into /tmp/fs.img without unmounting /mnt/jffs2
# so concurrent readers (/mnt/jffs2/wlan0.conf, /mnt/jffs2/sys.conf) never see an empty mountpoint.
sync
if grep -q " $CONFIG_MNT " /proc/mounts 2>/dev/null
then
	mount -o remount,ro "$CONFIG_MNT" 2>/dev/null || true
	sync
fi

rm -f "$CONFIG_GZ" 2>/dev/null
if ! gzip -c "$CONFIG_FILE" > "$CONFIG_GZ"
then
	if grep -q " $CONFIG_MNT " /proc/mounts 2>/dev/null; then
		mount -o remount,rw "$CONFIG_MNT" 2>/dev/null || true
	fi
	echo " ($$) Failed compressing configuration filesystem"
	logger -t $$ "Failed compressing configuration filesystem"
	exit 1
fi

if grep -q " $CONFIG_MNT " /proc/mounts 2>/dev/null
then
	mount -o remount,rw "$CONFIG_MNT" 2>/dev/null || true
fi

# Never start an erase/write cycle with an image larger than mtdblock4 (128 KB).
CONFIG_SIZE=`wc -c < "$CONFIG_GZ"`
if [ "$CONFIG_SIZE" -gt "$CONFIG_MAX_SIZE" ]
then
	echo " ($$) Configuration image is too large: $CONFIG_SIZE bytes (max $CONFIG_MAX_SIZE)"
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
