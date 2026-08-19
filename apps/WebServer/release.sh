#!/bin/sh

#svn update
WEBROOTDIR=`pwd`
ROOTDIR=`cd ../.. ; pwd`
cd ${WEBROOTDIR}/LINUX

rm -f webs.platform.STAR
rm -f webs.platform.DONGLE

make -f Makefile.snapgear clean
make -f Makefile.snapgear
arm-linux-strip webs
mv webs webs.platform.DONGLE
cp webs.platform.DONGLE  /mnt/P/tools/WebServer/

make -f Makefile.STAR clean
make -f Makefile.STAR
/opt/star/tools/arm-uclibc-3.4.6/bin/arm-linux-strip webs
#mv webs webs.platform.STAR
#cp webs.platform.STAR  /mnt/P/tools/WebServer/
#cp webs /home/jacky.yang/METALINK/11/nv-1.2.10-UMedia/_wlan-vb01.04-4MB/web/
#cp webs /home/jacky.yang/METALINK/VB1.09-WLS-2.3.5.37/nv-1.2.12/_wlan.tar.gz/web/
cp webs ${ROOTDIR}/_wlan.tar.gz/web

