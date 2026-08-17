#!/bin/sh

# Change to the directory of the mtlk init scripts and run them
# TODO: the path might have to be changed
cd /root/mtlk/etc

# Add a link for the RW fs mointpoint - this is needed by the mtlk web server
ln -s /mnt/jffs2 /tmp/jffs2

. ./mtlk_init.sh

# Jacky.Yang 28-Nov-2008, We don't want auto init telnet daemon.
# Start the telnetd server that is missing by default on STAR
#telnetd -f /etc/issue
