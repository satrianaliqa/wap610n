#!/bin/sh

# This script is used to get a lock before doing a mount/umount, to prevent filesystem corruption
# (mainly due to the fact that there is no other sync mechanism, and umount/mount can be called by several
# places in parallel)
#
# Algorithm: Use an atomic system file-handling operation.
#   In this case, we use ln -s. If the link already exists, the return status will not be 0. 
#   The PID of the locker is saved in the link target, to make sure that stale locks aren't left in the system.
#   Before declaring failure, we see if the old locking process is still running. If not, we can delete the lock
#   and try to reclaim this lock.
# This script is based on ideas from:
# http://bash-hackers.org/wiki/doku.php?id=howto:mutex
# and http://www.unixreview.com/documents/s=1344/ur0402g/
# (This is a simpler version, without adding system traps to the locks)

# USAGE:
#    config_lock.sh $$
#    if [ ! $? -eq 0 ]; then dont_continue; fi
#    do_something
#    config_unlock.sh $$

CONFIG_LOCKFILE=/tmp/config_lock

if [ ! $# -eq 1 ]
then
	echo "config_lock usage: $0 <PID>"
	exit 2
fi

PID=$1

# Get lock by creating a directory - 
# This is an atomic operation, because you can use the return value to see if the lock was successful
ln -s $PID $CONFIG_LOCKFILE 2> /dev/null

if [ $? -eq 0 ]
then
	# Lock was successful
	# We can now be sure that we own the lock
	echo " ($PID) Got config lock"
	exit 0
fi


# Lock failed - check if old process still exists, or we need to clean up a stale lock and try again

OLDPID=$(ls -l "$CONFIG_LOCKFILE" 2>/dev/null | awk '{print $11}')
if [ -z "$OLDPID" ]
then
	# The lock was released since we issued the ln command.
	# Try obtaining a lock again
	echo " ($PID) Lock was removed. Restarting config_lock attempt."
	# Recursively call this app with the original arguments
	exec $0 $@
	
fi

# kill -0 will return success if the process exists, but won't send a real signal to it
kill -0 $OLDPID 2> /dev/null
if [ ! $? -eq 0 ]
then
	# Stale lock - remove it and try again

	# Extra validation needed: it could be that you will rm a newly obtained lock,
	# if the old lock was deleted between the cat and the kill, and a different process got the lock,
	# so make sure that the lock PID didn't change
	LOCKPID=$(ls -l "$CONFIG_LOCKFILE" 2>/dev/null | awk '{print $11}')
	if [ -n "$LOCKPID" ] && [ "$LOCKPID" = "$OLDPID" ]
	then 
		echo " ($PID) Found stale config lock (OLD: $OLDPID)"
		
		echo " ($PID) Removing stale config lock (OLD: $OLDPID)"
		rm -f $CONFIG_LOCKFILE
		
		# Recursively call this script with the original arguments
		echo " ($PID) Restarting config_lock attempt."
		exec $0 $@
	fi
fi

# Config lock is already taken - return fail
echo " ($PID) Can't get config lock"
exit 1
