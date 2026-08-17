#!/bin/sh
make clean && ./my_build.236.sh
# Add to rootfs
cp --reply=yes gpio.ko /opt/rootfs-star/lib/modules/2.6.16-star/
/opt/devscripts/mkrootfs.sh
# add to bootpImage
cd /opt/star/kernel/linux-2.6.16-current
./mkbootp.sh
cp --reply=yes ../../output/bootpImage /tftpboot/


