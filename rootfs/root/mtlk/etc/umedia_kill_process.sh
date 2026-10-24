#!/bin/sh

TARGET="$1"
if [ -z "$TARGET" ]; then
	echo "U-Media: no process name supplied" > /dev/console
	exit 1
fi

attempt=0
while [ $attempt -lt 10 ]; do
	A=`ps | grep -F "$TARGET" | grep -v grep | grep -v umedia_kill_process.sh | awk -v self=$$ '$1 != self {print $1}'`
	if [ -z "$A" ]; then
		echo "U-Media: Leave kill $TARGET loop." > /dev/console
		exit 0
	fi
	echo "U-Media: $TARGET=$A" > /dev/console
	for a in $A
	do
		echo "U-Media($TARGET): kill -9 $a" > /dev/console
		kill -9 "$a" 2>/dev/null || true
	done
	attempt=`expr $attempt + 1`
	sleep 1
done

echo "U-Media: timed out killing $TARGET" > /dev/console
exit 1
