#!/bin/sh
rm upnpd.platform.*

make -f makefile.snapgear clean
make -f makefile.snapgear
arm-linux-strip upnpd
mv upnpd upnpd.platform.DONGLE
cp upnpd.platform.DONGLE /mnt/P/tools/upnpd

make -f makefile.STAR clean
make -f makefile.STAR 
~/star/Networking-versions/star/tools/arm-uclibc-3.4.6/bin/arm-linux-strip upnpd
mv upnpd upnpd.platform.STAR
cp upnpd.platform.STAR /mnt/P/tools/upnpd

