#!/bin/sh
make clean && ./my_build.237.sh
# Add to rootfs
cp -f gpio.ko /opt/rootfs-star/lib/modules/2.6.16-star/
/opt/devscripts/mkrootfs.sh
# add to bootpImage
cd /opt/star/kernel/linux-2.6.16-star
./mkbootp.sh
cp -f ../../output/bootpImage /tftpboot/


