#!/bin/sh

# This is a script to save the kernel log file at the end of the init stage.
# This is done to assist debugging, because the log file may be overwritten 
# due to rotating logs.




# Give some time for the driver to print any interesting messages
sleep 15

nice -n 18 dmesg > /tmp/dmesg.head.txt

