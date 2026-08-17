#!/bin/sh

# config_save.sh: unmount and mount the config fs

echo "Saving configuration filesystem"

# Unmount the config fs and copy it to flash
config_umount.sh

# Remount the config fs so that it is available again
config_mount.sh
