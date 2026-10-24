#!/bin/sh

# This script is used to release a lock after doing a mount/umount, to prevent filesystem corruption
# (mainly due to the fact that there is no other sync mechanism, and umount/mount can be called by several
# places in parallel)
#
# Algorithm: Use an atomic system file-handling operation.
#   In this case, we use ln -s with the PID saved as the link target. 
#   If the link already exists, the return status will not be 0. 
#   The unlock file will remove the link

CONFIG_LOCKFILE=/tmp/config_lock

if [ $# -ne 1 ]
then
	echo "config_unlock usage: $0 <PID>"
	exit 2
fi

PID=$1

if [ ! -L "$CONFIG_LOCKFILE" ] && [ ! -e "$CONFIG_LOCKFILE" ]
then
	echo " ($PID) Unable to release config lock. No lock taken"
	exit 1
fi

PID_FOUND=`readlink "$CONFIG_LOCKFILE" 2>/dev/null`
if [ -z "$PID_FOUND" ]; then
	PID_FOUND=`ls -l "$CONFIG_LOCKFILE" 2>/dev/null | awk '{print $NF}'`
fi

if [ "$PID_FOUND" != "$PID" ]
then
	echo " ($PID) Unable to release config lock. Not owner (OLD: $PID_FOUND)"
	exit 1
fi

rm -f "$CONFIG_LOCKFILE"
echo " ($PID) Released config lock."
exit 0
