#!/bin/sh

# This is a script to save the kernel log file at the end of the init stage.
# This is done, because the log file may be overwritten due to rotating logs.

# Currently, the log is saved in RAM to reduce flash wear, but it could be moved
# to jffs for post mortem analysis.



# Give some time for the driver to print any interesting messages
sleep 60

if [ -e /var/log/messages ]
then
	nice -n 18 cat /var/log/messages > /var/log/messages.head.txt
else
	nice -n 18 dmesg > /tmp/dmesg.head.txt
fi

