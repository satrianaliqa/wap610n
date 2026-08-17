#
# Copyright (c) 2006-2007 Metalink Broadband (Israel)
#
# @file    Makefile.str9102
#
# @brief   This file for STAR 9102 Dorado to build the wpa_supplicant utility
#
# Author:  Oleksandr Baglei <OleksandrB@metalinkbb.com>
#

CROSS_COMPILE=/opt/star/tools/arm-uclibc-3.4.6/bin/arm-linux-uclibc-
CFLAGS=-Os -I/opt/apps-rootfs/openssl-0.9.8a/include
LIBS=-L/opt/apps-rootfs/openssl-0.9.8a 
LIBS_p=-L/opt/apps-rootfs/openssl-0.9.8a 
CC=${CROSS_COMPILE}gcc
export CC CFLAGS LIBS LIBS_p

all:
	make

clean:
	make clean

