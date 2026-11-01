#!/bin/sh

# mount_config.sh
#
# This script mounts the config filesystem, after loading it from flash

CONFIG_BLOCK=/dev/mtdblock4
CONFIG_FILE=/tmp/fs.img
CONFIG_MNT=/mnt/jffs2

RESTORE_DEFAULTS=0
RESTORE_DEFAULTS_PATH=/root/mtlk/etc
CONFIG_DEFAULT=$RESTORE_DEFAULTS_PATH/config.img.gz
DIAG_LOG=/bin/diag_log.sh
$DIAG_LOG "config_mount: starting" 2>/dev/null || true

# First get a lock - to prevent parallel execution of config_mount/umount scripts
config_lock.sh $$
if [ $? -ne 0 ]
then
	echo "config_mount ($$): Could not obtain config lock!"
	$DIAG_LOG "config_mount: could not obtain configuration lock" 2>/dev/null || true
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
	$DIAG_LOG "config_mount: invalid or unreadable MTD4 image; using factory image" 2>/dev/null || true
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
		$DIAG_LOG "config_mount: failed to mount factory configuration filesystem" 2>/dev/null || true
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
	if ! "$RESTORE_DEFAULTS_PATH/mtlk_restore_defaults.sh" 1 0
	then
		echo " ($$) Failed restoring factory defaults after mounting fallback image"
		$DIAG_LOG "config_mount: factory restore failed after fallback mount" 2>/dev/null || true
		logger -t $$ "Failed restoring factory defaults after mounting fallback image"
		exit 1
	fi
fi

# Jacky.Yang 28-Jul-2009, it seems will cause racing condition, because killall -HUP webs will update mapping cache.
# Jacky.Yang 28-Jul-2009, We will check wlan0.conf file first.
$DIAG_LOG "config_mount: checking wlan0.conf" 2>/dev/null || true
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
			$DIAG_LOG "config_mount: wlan0.conf missing; retry $countTMP" 2>/dev/null || true
		fi
	else
		$DIAG_LOG "config_mount: wlan0.conf missing; restoring factory defaults" 2>/dev/null || true
		if ! "$RESTORE_DEFAULTS_PATH/mtlk_restore_defaults.sh" 1 0
		then
			echo " ($$) Failed restoring factory defaults after missing wlan0.conf"
			$DIAG_LOG "config_mount: wlan0.conf missing and factory restore failed" 2>/dev/null || true
			logger -t $$ "Failed restoring factory defaults after missing wlan0.conf"
			exit 1
		fi
		break
	fi
done
$DIAG_LOG "config_mount: configuration filesystem ready" 2>/dev/null || true
