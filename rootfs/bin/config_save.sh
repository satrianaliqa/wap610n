#!/bin/sh

# config_save.sh: unmount and mount the config fs

echo "Saving configuration filesystem"

# Unmount the config fs and copy it to flash
if ! config_umount.sh
then
	echo "Configuration filesystem was not saved"
	exit 1
fi

# Remount the config fs so that it is available again
if ! config_mount.sh
then
	echo "Configuration filesystem was saved but could not be remounted"
	exit 1
fi
