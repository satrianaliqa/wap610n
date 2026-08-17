This directory contains scripts and applications for Metalink wireless solutions.

Copyright (c) 2007,2008,2009 Metalink Ltd.

There are three subdirs in this directory:

The networking and wireless directories contain init scripts and run-time scripts.
These two directories are released under the GPL version 2. 
See the file LICENSE for more details.

The driver_apps directory contains proprietary user space applications, 
Copyright (c) 2008,2009 Metalink Ltd.














Revision history:
=================
This file hasn't been updated in a long while, so the info here should be considered outdated.
See svn log and nv revision history file for current revision history.

3.3.5 (05/03/08 ArnonM)
---------------------
[SVN:
   http://narnia/svn/wl/wl/linux/trunk/snapgear/dongle-2.6/vendors/Intel/mtlk_dongle/jffs2-etc 
 Revision 2410 ]
This is what was released in nv_1.0.7

	Cleaned up some [intentional] init errors, e.g. modprobe e1000
	Added mtlk_restore_defaults.sh script, based on what is run by the external RestoreDongleDefaults app
	added . before running /tmp/restore.def so it will know the system environment variables
	Tried dropping multicast packets by using iptables. Replaced by kernel patch because this wasn't good enough. 
	Fixed bridging init to handle WDS mode
	Moved the gpio driver: it is needed before trying to read the dev/pbc0



3.3.3 (05/03/08 ArnonM)
-----------------------
[SVN:
   http://narnia/svn/wl/wl/linux/trunk/snapgear/dongle-2.6/vendors/Intel/mtlk_dongle/jffs2-etc 
 Revision 2338 ]
Drop multicast packets by using iptables, instead of changing the br0 interface 
(v3.3.1 implementation didn't solve CPU utilization problem)

3.3.2_2331 (05/03/08 ArnonM)
-----------------------

- TEMPORARY WORKAROUND, BECAUSE THE ENUM VALUE OF BRIDGE MODE CHANGED 
	BETWEEN 2.3.0 AND 2.3.5 BRANCHES.



3.3.1 (03/03/08 ArnonM)
-----------------------
[SVN:
   http://narnia/svn/wl/wl/linux/trunk/snapgear/dongle-2.6/vendors/Intel/mtlk_dongle/jffs2-etc 
 Revision 2310 ]
1. Support both old and new naming conventions for mac cloning, to support 2.3.0 and 2.3.5 releases
2. Remove multicast support from br0. This should fix cpu utilization in reliable multicast streaming.


3.3.0 (21/02/08 ArnonM)
[SVN:
   http://narnia/svn/wl/wl/linux/trunk/snapgear/dongle-2.6/vendors/Intel/mtlk_dongle/jffs2-etc 
 Revision 2288 ]
Separated wireless and networking inits to different folders, and will be released by different groups
Support new naming convention of mac cloning

3.2.8 (28/01/07 ArnonM)
[SVN:
   http://narnia/svn/wl/wl/linux/trunk/snapgear/dongle-2.6/vendors/Intel/mtlk_dongle/jffs2-etc 
 Revision 2093 ]
# TEMPORARY WORKAROUND: increase bridging ageing to about 10 days
# TODO: Replace with good mechanism for handling unidirectional traffic on bridge


3.2.4 (23/01/07 Avri)
[SVN:
   http://narnia/svn/wl/wl/linux/trunk/snapgear/dongle-2.6/vendors/Intel/mtlk_dongle/jffs2-etc 
 Revision 2066 ]

Add error handling by using () when running external scripts
Add handling for wls init to avoid crash when wlan0.conf and sys.conf doesn't exist

3.2.3 (17/01/07 Edi)
[SVN:
   http://narnia/svn/wl/wl/linux/trunk/snapgear/dongle-2.6/vendors/Intel/mtlk_dongle/jffs2-etc 
 Revision 2002 ]

Add PowerIncreaseVsDutyCycle=STD::PROC into driver_api.ini
================
3.2.2 (17/01/07 Arnon)
[SVN:
   http://narnia/svn/wl/wl/linux/trunk/snapgear/dongle-2.6/vendors/Intel/mtlk_dongle/jffs2-etc 
 Revision 1989 ]

1. Support multiple platforms (Dongle + STAR) from same init scripts.
2. Integrate WPS PB Support on STAR platforms
3. Initial support for tftp download of wireless driver and firmware (EXPERIMENTAL: not tested yet)

3.2.1 (16/01/07 Avri)
[SVN:
   http://narnia/svn/wl/wl/linux/trunk/snapgear/dongle-2.6/vendors/Intel/mtlk_dongle/jffs2-etc 
Revision 1969]

1. Add support for new wireless init (generated).
2. fix exit from script incase of an error

3.1.4 (06/01/07)
-----
[SVN:

   http://narnia/svn/wl/wl/linux/trunk/snapgear/dongle-2.6/vendors/Intel/mtlk_dongle/jffs2-etc 

Revision 1933]

1. Generation of a unique uuid is now supported.


3.1.3 (Edi)
-----

add script create_wls_init.tcl that build mtlk_init_wls online


3.1.2 (27/12/07)
-----
[SVN:

   http://narnia/svn/wl/wl/linux/trunk/snapgear/dongle-2.6/vendors/Intel/mtlk_dongle/jffs2-etc 

Revision 1902]

1. Added new mac cloning support in wps

3.1.1 (17/12/07)
-----
[SVN:

   http://narnia/svn/wl/wl/linux/trunk/snapgear/dongle-2.6/vendors/Intel/mtlk_dongle/jffs2-etc 

Revision 1825]
3.1.x: This is the branch for 2.3.x releases

1. Added new mac cloning support in wpa_supplicant.
2. Made the set -x feature a debug option - uncomment in mtlk_init.sh to turn it on.

3.0.x
-----
This is the branch for 2.2.5.x releases


2.6.42 (11/12/07)
-----
[SVN:

   http://narnia/svn/wl/wl/linux/trunk/snapgear/dongle-2.6/vendors/Intel/mtlk_dongle/jffs2-etc 

Revision 1795]


1. Added the printout of all shell commands during init.

2. Synced svn with latest G releases 2.2.4/2.6.41 and 2.2.5/3.0.2:
	1.   mtlk_init_apps.sh now directs wps stdout to dev/null (and not to the serial).
	ii.  removed unneeded files
	iii. removed the qa-routing code that was in 2.6.41
	iv.  fixed path for connected proc in ledman.


2.6.40 - Edi
-----
added :
Fix path in mtlk_sysinfo.sh



2.6.38
-----
[SVN:

   http://narnia/svn/wl/wl/linux/trunk/snapgear/dongle-2.6/vendors/Intel/mtlk_dongle/jffs2-etc 

Revision 1725]
added :
echo 1 > /proc/net/mtlk/debug into mtlk_init_wls.sh

2.6.37 (29/10/07)
-----
[SVN:

   http://narnia/svn/wl/wl/linux/trunk/snapgear/dongle-2.6/vendors/Intel/mtlk_dongle/jffs2-etc 

Revision 1721]

Automated the mechanism that links the ProgModel files to the driver dir (/tmp).
This way, the wls script doesn't have to be modified with each new revision.

2.6.35 (23/10/07)
-------
[SVN Revision 1714]
- Added sleep 5 after activation of wsccmd (mtlk_init_wls_aps.sh).


2.6.33 (16/10/07)
-----
[SVN:

   http://narnia/svn/wl/wl/linux/trunk/snapgear/dongle-2.6/vendors/Intel/mtlk_dongle/jffs2-etc 

Revision 1707]

Added new restore defaults script.
This runs before anything else, and tries to download a file to execute by tftp.


2.6.32 (16/10/07)
-----
[SVN:

   http://narnia/svn/wl/wl/linux/trunk/snapgear/dongle-2.6/vendors/Intel/mtlk_dongle/jffs2-etc 

Revision 1704]

Bugfix:
Load all the init scripts in the same context, so that environment variables are shared.

2.6.31 (10/10/07)
-----
[SVN:

   http://narnia/svn/wl/wl/linux/trunk/snapgear/dongle-2.6/vendors/Intel/mtlk_dongle/jffs2-etc 

Revision 1702]

1. Created a mechanism to skip init scripts when part of the init fails.
Currently, this is used to skip init_wls if the insmod failed.
2. Separated init of web, bcl and leds to a new file: daemons.
3. Separated init of wps and hostapd to a new file: wls_apps (is run last).


2.6.30 (25/09/07)
-----
[SVN:

   http://narnia/svn/wl/wl/linux/trunk/snapgear/dongle-2.6/vendors/Intel/mtlk_dongle/jffs2-etc 

Revision 1687]

1. Merged dongle2.0 mismatch into correct svn (includes TC buffers, Operate/Basic rate support) and WPS
2. WPS changes
	a. Added open security in WPS mode
	b. Added encryption flags to TKIP+CCMP
	c. A temporal fix to see beacons (wsccmd is activated at the end)
	d. A fix to see the scan results of a station 


2.6.29 (18/09/07)
-----
[SVN:

   http://narnia/svn/wl/wl/linux/trunk/snapgear/dongle-2.6/vendors/Intel/mtlk_dongle/jffs2-etc 

Revision 1659]

Added support for 802.11D.
Integrated 2.6.28 change to SVN.
(NOTE: WPS FEATURE STILL ISN'T IN G: VERSION)

2.6.28 (17/09/07)
-----
(not in SVN)
Changed tc queue size.

2.6.27 (04/09/07)
-----
(not in SVN)
1. Removed set basic/operate set code, that was set according to protocol type
2. Changed tc queue sizes.

[SVN:

   http://narnia/svn/wl/wl/linux/trunk/snapgear/dongle-2.6/vendors/Intel/mtlk_dongle/jffs2-etc 

Revision 1646]

NOT IN G:
Added support for activating WPS . The feature is currently disabled.
A new file is added to initialize the WPS application: mtlk_init_wps.sh

2.6.26 (26/08/07)
-----
Added support for progmodels for new 5GHz single band cards.

2.6.25 (21/08/07)
-----
New hostapd and wpa_supplicant for WPS support.

2.6.24 (05/08/07)
-----
[SVN:

   http://narnia/svn/wl/wl/linux/trunk/snapgear/dongle-2.0/vendors/Intel/mtlk_dongle/jffs2-etc 

Revision 1524]


1. Sleep 3 seconds between bringing the wls interface up and calling hostapd.
This should be replaced by an indication from the driver, instead of a hard coded time.

2.6.23 (01/08/07)
-----
[SVN:

   http://narnia/svn/wl/wl/linux/trunk/snapgear/dongle-2.0/vendors/Intel/mtlk_dongle/jffs2-etc 

Revision 1515]


1. Support for concurrent dual band. (driver is in .../mtlk/wlan0 and .../mtlk/wlan1)
2. Support linux 2.6 and linux 2.4 from the same init scripts.
3. Temporary? Removed the mtlk_init_security.sh script. It is currently done by the web.
4. Temporary? start 2 web servers (port 80 and 81)

2.6.22.26 (23/07/07)
-----
[SVN:

   http://narnia/svn/wl/wl/linux/trunk/snapgear/dongle-2.0/vendors/Intel/mtlk_dongle/jffs2-etc 

Revision 1503]


1. Adding a mtlk_init_security.sh script - writing the hostapd/supplicant
   conf files.


2.6.20 (8/07/07)
-----
[SVN:
   http://narnia/svn/wl/wl/linux/trunk/snapgear/dongle-2.0/vendors/Intel/mtlk_dongle/jffs2-etc 
Revision 1490]

1. Support gigabit ethernet - if eth2 exists, add it to bridge
2. Don't try to add usb0 to bridge if it doesn't exist


2.6.19 (8/07/07)
-----
[SVN:
   http://narnia/svn/wl/wl/linux/trunk/snapgear/dongle-2.0/vendors/Intel/mtlk_dongle/jffs2-etc 
Revision 1487]

1. Remove unsupported progmodels.
2. Remove 20 second sleep. TBD: What to do with this???
3. Remove hostapd bridge configuration
4. Separate [hardcoded] basic/supported rates for STA and AP.


2.6.17 (28/06/07)
-----
[SVN:
   http://narnia/svn/wl/wl/linux/trunk/snapgear/dongle-2.0/vendors/Intel/mtlk_dongle/jffs2-etc 
Revision 1486]

wait 20 secs for PHY calibration 


2.6.16 (12/06/07)
-----
[SVN:
   http://narnia/svn/wl/wl/linux/trunk/snapgear/dongle-2.0/vendors/Intel/mtlk_dongle/jffs2-etc 
Revision 1482]

Fixes to wireless bridging:
1. Run hostapd in bridge mode
2. Support all new bridging modes (wireless_bridging > 0)


2.6.11 (12/06/07)
-----
[SVN:
   http://narnia/svn/wl/wl/linux/trunk/snapgear/dongle-2.0/vendors/Intel/mtlk_dongle/jffs2-etc 
Revision 1461]

Bugfix to mtlk_sysinfo.sh script

2.6.10 (10/06/07)
-----
[SVN:
   http://narnia/svn/wl/wl/linux/trunk/snapgear/dongle-2.0/vendors/Intel/mtlk_dongle/jffs2-etc 
Revision 1454]

Added support for new web configuration tool:
  Start web server on init.


2.6.9 (06/06/07)
-----
[SVN:
   http://narnia/svn/wl/wl/linux/trunk/snapgear/dongle-2.0/vendors/Intel/mtlk_dongle/jffs2-etc 
Revision 1449]

Added new progmodels for RevE

2.6.8 (27/05/07) - ArnonM
-----
[SVN:
   http://narnia/svn/wl/wl/linux/trunk/snapgear/dongle-2.0/vendors/Intel/mtlk_dongle/jffs2-etc 
Revision 1433]

1. Added sleep before setting route, to give the wireless time to bring the interface up.
This is someting that is needed in versions 2.2+, and must be investigated [sometime].

2. Save the syslog to /var/log/messages.head.txt after init, so that it will be available
even if the log is overwritten due to rotating logs.

3. Init optimization: Remove the 30 second delay of the bridge interface.


2.6.7 (15/05/07) - Edi
-----
was added hostapd application
led.o and mtlk_led ... sh from version 2.1
Under "(2) 802.11b/g" the operational-rates  0x00007FFF and basic 0x00007800
Under "(3) 802.11g only" the operational-rates 0x00007FFF and basic 0x00007815
Under "(6) 802.11n/g/b" the operational-rates 0xFFFFFFFF and basic 0x00007800
Under "(5) 802.11n/g" the operational-rates 0xFFFFFFFF and basic 0x00007815

2.6.6 (14/05/07)
-----
Changed queue size to 2000.

2.6.3 (26/04/07)
-----
Bugfix: Don't check for existance of wlan MAC in sys.conf
It won't be there if working with MAC from EEPROM.

2.6.2 (26/04/07)
-----
Added 4 new progmodels


V 2.6.1 (April 22, 2007):   [Subversion revision 1356]
------------------------------------------------------
1) Merge between 2.3.1 and 2.6

V 2.3.1 (April 12, 2007):   [Subversion revision 1356]
------------------------------------------------------

1) WLAN MAC is read from EEPROM.
   When MAC cloning is used overwrite the mac address in 'proc/.../mtlk/MAC' .
   This change involved a change in the CLI scripts.
2) Add version number for init scripts in mtlk_init.sh

2.1.3 (05/02/07)
-----
SVN revision 1285

init wireless change:
Added support for selection from multiple progmodels.


2.1.2 (05/02/07)
-----
SVN revision 1276

New feature: 
Support STA Scan:
won't connect if there is no ESSID or channel

Change:
Use awk script to init the wireless parameters. 
(this change should be invisible to users)


2.1.1 (27/12/06)
-----
New feature: LEDs
1 - Power (init has finished, telnet and ping should work)
2 - Connected
3 - Tx
4 - Rx


2.1.0 (18/12/06)
-----
This is the first Gen2 release, although it should also be compatible with Gen1.

Changes:
conf_wlan.sh Ver. 2.0 (which loads wireless parameters to the driver) now uses a dynamic mechanism
to discover what parameters the driver exports, instead of having to maintain a static list.


2.0.8 (27/12/06)
-----
New feature: LEDs
1 - Power (init has finished, telnet and ping should work)
2 - Connected
3 - Tx
4 - Rx


2.0.7 (07/12/06)
-----
Bugfix:
Fixed syntax error when setting HTSupportedRates (channel bonding)


2.0.6 (06/12/06)
-----
Bugfix to Auto MAC Cloning bug:
Make sure that each byte in the MAC address consists of 2 chars (this isn't necessarily what tcpdump returns...)


2.0.5 (30/11/06)
-----
Support in init for Auto reconnect, PhyControlMask, Rate adaptation, 
and Reliable multicast parameters.


2.0.4 (26/11/06)
-----
Support in init for Auto Aggregate parameter


2.0.3 (26/11/06)
-----
SAFETY FEATURES to protect against editing files on windows:
1. Run dos2unix on wlan.conf to make sure that Windows newlines are removed.
2. Remove Windows newlines on parameters returned by awk.

Optimization:
Copy wlan.conf to tmp and use the tmp file, because tmp is ramdisk and not flash.
This isn't a major optimization, but it will prevent dos2unix from writing to the flash
on each reboot.


2.0.2
-----
SAFETY FEATURE:
Init doesn't try to set the MAC address of the ethernet interfaces if they are missing
in the config file.
  ( - this would kill the LAN interfaces).


2.0.1
-----
This is the first version that went to QA testing.
