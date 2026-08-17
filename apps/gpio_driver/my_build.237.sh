#!/bin/sh
make KERNELPATH=/opt/Networking-versions/nv-1.2.11/kernel/linux-2.6.16-star/ CROSS_COMPILE=/opt/Networking-versions/nv-1.2.11/tools/arm-uclibc-3.4.6/bin/arm-linux-uclibc- -C /opt/Networking-versions/nv-1.2.11/kernel/linux-2.6.16-star/  M=`pwd` modules

sudo cp gpio.ko /mnt/arnonm/STAR/BOARD2-GPB237-5GHz-VB/

