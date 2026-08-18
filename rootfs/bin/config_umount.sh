#!/bin/sh

# umount_config.sh
#
# This script unmounts the config filesystem, and saves it to flash

echo "U-Media, umount_config.sh: Who call me?" > /dev/console

CONFIG_BLOCK=/dev/mtdblock4
CONFIG_FILE=/tmp/fs.img
CONFIG_GZ=${CONFIG_FILE}.gz
CONFIG_MNT=/mnt/jffs2


# First get a lock - to prevent parallel execution of config_mount/umount scripts
config_lock.sh $$
if [ ! $? -eq 0 ]
then
	echo " ($$) config_umount: Could not obtain config lock!"
	logger -t $$ "config_umount: Could not obtain config lock!"
	exit 1
fi

echo " ($$) Unmounting configuration filesystem"

UMOUNT_RETRY_COUNT=0
# Try umounting a number of times
umount $CONFIG_MNT
while [ $? -gt 0 ] 
do
	# umount failed - try again
	UMOUNT_RETRY_COUNT=`expr $UMOUNT_RETRY_COUNT + 1`
	if [ $UMOUNT_RETRY_COUNT -gt 5 ]
	then
		echo " ($$) Failed unmounting and saving configuration filesystem!"
		logger -t $$ "Failed unmounting and saving configuration filesystem!"

		# Release lock
		config_unlock.sh $$

		exit 1
	fi
	sleep 1
	
	# Try umounting again
	umount $CONFIG_MNT
done

echo " ($$) Unmounted configuration filesystem (in $UMOUNT_RETRY_COUNT retry attempts)"

echo " ($$) Saving configuration filesystem to flash"
## UNSAFE BUT SIMPLE METHOD: gzip -c $CONFIG_FILE > $CONFIG_BLOCK

# Compress filesystem
if [ -e $CONFIG_GZ ]
then 
	rm $CONFIG_GZ
fi
gzip $CONFIG_FILE

# Copy to flash
cp $CONFIG_GZ $CONFIG_BLOCK

# Validate image integrity - do a binary diff of tmp file and flash
CP_RETRY_COUNT=0
simpdiff -s $CONFIG_GZ $CONFIG_BLOCK
while [ $? -gt 0 ] 
do
	# Failed writing to flash - try again.
	CP_RETRY_COUNT=`expr $CP_RETRY_COUNT + 1 `
	if [ $CP_RETRY_COUNT -gt 5 ]
	then
		echo " ($$) Failed copying configuration filesystem to flash!"
		logger -t $$ "Failed copying configuration filesystem to flash!"

		# Release lock
		config_unlock.sh $$

		exit 1
	fi

	# Try copying again
	cp $CONFIG_GZ $CONFIG_BLOCK
	simpdiff -s $CONFIG_GZ $CONFIG_BLOCK
done
echo " ($$) Configuration saved (in $CP_RETRY_COUNT retry attempts)"

# Delete tmp gzip file
rm $CONFIG_GZ

# Release lock
config_unlock.sh $$
