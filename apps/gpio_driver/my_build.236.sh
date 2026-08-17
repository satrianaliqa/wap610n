#!/bin/sh
make KERNELPATH=/opt/star/kernel/linux-2.6.16-current/ CROSS_COMPILE=/opt/star/tools/arm-uclibc-3.4.6/bin/arm-linux-uclibc- -C /opt/star/kernel/linux-2.6.16-current/ M=`pwd` modules

