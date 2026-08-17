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


if [ ! $# -eq 1 ]
then
	echo "config_unlock usage: $0 <PID>"
	return 2
fi

PID=$1

LOCKFOUND=`ls -l $CONFIG_LOCKFILE` 2> /dev/null
if [ ! $? -eq 0 ]
then
	# No config lock is taken - return fail
	echo " ($PID) Unable to release config lock. No lock taken"
	return 1
fi

# Make sure that we are the owners of the lock
PID_FOUND=`ls -l $CONFIG_LOCKFILE | awk '{print $11}'` 2> /dev/null

if [ ! $PID_FOUND -eq $PID ]
then
	# Config lock isn't owned by us - return fail
	echo " ($PID) Unable to release config lock. Not owner (OLD: $PID_FOUND)"
	return 1
fi

# We can now be sure that we own the lock, so delete it
rm -f $CONFIG_LOCKFILE
echo " ($PID) Released config lock."
return 0
