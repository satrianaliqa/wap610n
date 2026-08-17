#!/bin/sh

# This script sets platform specific settings for the STAR platforms.
# Currently, this is one script for all STAR boards.

# TODO: Currently this assumes that everything is on the jffs/ext2 partition
# So we want to change paths to the rootfs partition?
RWFS_PATH=/mnt/jffs2
ROOT_PATH=/root/mtlk
CONFIGS_PATH=$RWFS_PATH
IMAGES_PATH=$ROOT_PATH/images
DRIVER_PATH=$ROOT_PATH/driver
ETC_PATH=$ROOT_PATH/etc
WEB_PATH=$ROOT_PATH/web
BCL_PATH=$ROOT_PATH/bcl
SAVED_CONFIG_PATH=$ROOT_PATH/saved_configs
WPA_APPS_PATH=$ETC_PATH
WPA_CONFIGS_PATH=$RWFS_PATH

# Don't start the led manager on STAR platforms
LEDMAN=0

# STAR platforms have pushbutton for WPS
WPS_PB=1

# Lower memory usage - use smaller net queues
LOWER_MEM_USAGE=1

# Enable tc queues
CONFIGURE_TC=1

# Don't mangle multicast using iptables - this is done in a kernel patch (for CPU utilization)
MULTICAST_MANGLING=0

# GPIO pins used for output LEDs
GPIO_LEDS=0,3
# GPIO pins used for pushbuttons
GPIO_PUSHBUTTONS=1,13


### Export all configuration options that need to be run outside a shell, e.g. from tclsh
# TODO: For safety's sake - export everything from this file?
# You could then use this file to initialize web_config.tcl
export CONFIGURE_TC RWFS_PATH ROOT_PATH CONFIGS_PATH IMAGES_PATH DRIVER_PATH ETC_PATH WEB_PATH BCL_PATH SAVED_CONFIG_PATH WPA_APPS_PATH WPA_CONFIGS_PATH
