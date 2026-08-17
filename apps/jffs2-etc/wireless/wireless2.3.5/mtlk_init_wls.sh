#!/bin/sh
# This is the script that brings up the wireless interface.

#SVN id: $Id: mtlk_init_wls.sh 2278 2008-02-21 15:40:01Z arnonm $

echo creating wireless init script
tclsh create_wls_init.tcl > /tmp/init_wls.sh
chmod +x /tmp/init_wls.sh
echo runing wireless init script
if [ -e /tmp/init_wls.sh ]; then . /tmp/init_wls.sh; fi
echo Done.
