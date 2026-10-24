#!/bin/sh

# mount_config.sh
#
# This script mounts the config filesystem, after loading it from flash

echo "U-Media, mount_config.sh: Who call me?" > /dev/console

CONFIG_BLOCK=/dev/mtdblock4
CONFIG_FILE=/tmp/fs.img
CONFIG_MNT=/mnt/jffs2

RESTORE_DEFAULTS=0
RESTORE_DEFAULTS_PATH=/root/mtlk/etc
CONFIG_DEFAULT=$RESTORE_DEFAULTS_PATH/config.img.gz

# First get a lock - to prevent parallel execution of config_mount/umount scripts
config_lock.sh $$
if [ $? -ne 0 ]
then
	echo "config_mount ($$): Could not obtain config lock!"
	logger -t $$ "config_mount: Could not obtain config lock!"
	exit 1
fi

# Check if already mounted - since config_umount.sh now preserves the mount,
# ensure rw mode and return success immediately if valid.
if grep -q " $CONFIG_MNT " /proc/mounts 2>/dev/null || mount | grep -q "$CONFIG_MNT"
then
	mount -o remount,rw "$CONFIG_MNT" 2>/dev/null || true
	if [ -e "$CONFIG_MNT/wlan0.conf" ]
	then
		config_unlock.sh $$
		exit 0
	fi
	umount "$CONFIG_MNT" 2>/dev/null || true
fi

echo " ($$) Mounting configuration filesystem"

if [ ! -e $CONFIG_MNT ]
then
	mkdir $CONFIG_MNT
fi

gunzip -c $CONFIG_BLOCK > $CONFIG_FILE

# Check if gunzip failed - if so, restore configuration sector and defaults.
if [ $? -ne 0 ]
then
	echo " ($$) No config sector found. Copying factory defaults"
	gunzip -c $CONFIG_DEFAULT > $CONFIG_FILE
	RESTORE_DEFAULTS=1
fi

# Mount the existing or newly created config fs
mount $CONFIG_FILE $CONFIG_MNT -o loop,rw

# Check if mount failed - if so, restore configuration sector and defaults.
if [ $? -ne 0 ]
then
	echo " ($$) No valid config filesystem found. Copying factory defaults"
	gunzip -c $CONFIG_DEFAULT > $CONFIG_FILE
	if ! mount $CONFIG_FILE $CONFIG_MNT -o loop,rw
	then
		echo " ($$) Failed mounting factory default configuration filesystem"
		logger -t $$ "Failed mounting factory default configuration filesystem"
		config_unlock.sh $$
		exit 1
	fi
	RESTORE_DEFAULTS=1
fi

# Release lock (before restore defaults - because that can call mount recursively)
config_unlock.sh $$


# Copy default params if needed
if [ "$RESTORE_DEFAULTS" = "1" ]
then
	$RESTORE_DEFAULTS_PATH/mtlk_restore_defaults.sh
fi

# Jacky.Yang 28-Jul-2009, it seems will cause racing condition, because killall -HUP webs will update mapping cache.
# Jacky.Yang 28-Jul-2009, We will check wlan0.conf file first.
echo "U-Media, mount_config.sh: Start to check /mnt/jffs2/wlan.conf." > /dev/console
countTMP=0
while [ 1 ]
do
	countTMP=`expr $countTMP + 1`
	if [ $countTMP != 10 ]
	then
		if [ -e /mnt/jffs2/wlan0.conf ]
		then
			break
		else
			sleep 1
			echo "U-Media, mount_config.sh: No such file /mnt/jffs2/wlan0.conf waiting...$countTMP" > /dev/console
		fi
	else
		echo "U-Media, mount_config.sh: Missing wlan0.conf! Restoring factory defaults now..." > /dev/console
		$RESTORE_DEFAULTS_PATH/mtlk_restore_defaults.sh
		break
	fi
done
echo "U-Media, mount_config.sh: Configuration filesystem ready." > /dev/console
