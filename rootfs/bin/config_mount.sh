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
if [ ! $? -eq 0 ]
then
	echo "config_mount ($$): Could not obtain config lock!"
	logger -t $$ "config_mount: Could not obtain config lock!"
	exit 1
fi

# Check if already mounted - if so, warn and do nothing
CONFIG_MOUNTED=`mount | grep $CONFIG_MNT | wc -l`
if [ $CONFIG_MOUNTED -gt 0 ]; 
then 
	echo " ($$) Configuration filesystem is already mounted!"
	logger -t $$ "Configuration filesystem is already mounted!"

	# Release lock
	config_unlock.sh $$

	exit 1
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
mount $CONFIG_FILE $CONFIG_MNT -o loop

# Check if mount failed - if so, restore configuration sector and defaults.
if [ $? -ne 0 ]
then
	echo " ($$) No valid config filesystem found. Copying factory defaults"
	gunzip -c $CONFIG_DEFAULT > $CONFIG_FILE
	mount $CONFIG_FILE $CONFIG_MNT -o loop
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
		echo "U-Media, mount_config.sh: it seems can't get /mnt/jffs2/wlan0.conf file." > /dev/console
		break
	fi
done
echo "U-Media, mount_config.sh: Stop to check /mnt/jffs2/wlan.conf." > /dev/console
