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

if [ $# -ne 1 ]
then
	echo "config_lock usage: $0 <PID>"
	exit 2
fi

PID=$1
MAX_WAIT=20
WAIT_COUNT=0

get_lock_owner() {
	OWNER=`readlink "$CONFIG_LOCKFILE" 2>/dev/null`
	if [ -z "$OWNER" ] && [ -L "$CONFIG_LOCKFILE" ]; then
		OWNER=`ls -l "$CONFIG_LOCKFILE" 2>/dev/null | awk '{print $NF}'`
	fi
	echo "$OWNER"
}

while [ $WAIT_COUNT -lt $MAX_WAIT ]
do
	if ln -s "$PID" "$CONFIG_LOCKFILE" 2>/dev/null
	then
		echo " ($PID) Got config lock"
		exit 0
	fi

	OLDPID=`get_lock_owner`
	if [ -z "$OLDPID" ]
	then
		# Lock was just released between ln and readlink; retry immediately
		continue
	fi

	if [ "$OLDPID" = "$PID" ]
	then
		# Already owned by this PID
		exit 0
	fi

	if ! kill -0 "$OLDPID" 2>/dev/null
	then
		# Stale lock - verify owner didn't change before removing
		LOCKPID=`get_lock_owner`
		if [ -n "$LOCKPID" ] && [ "$LOCKPID" = "$OLDPID" ]
		then
			echo " ($PID) Removing stale config lock (OLD: $OLDPID)"
			rm -f "$CONFIG_LOCKFILE"
			continue
		fi
	fi

	# Lock is held by an active process; wait 1s for it to finish
	WAIT_COUNT=`expr $WAIT_COUNT + 1`
	sleep 1
done

echo " ($PID) Can't get config lock after ${MAX_WAIT}s"
exit 1
