#
# Copyright (c) 2006-2007 Metalink Broadband (Israel)
#
# @file    Makefile.str9102
#
# @brief   This file for STAR 9102 Dorado to build the Metalink driver
#
# Author:  Oleksandr Baglei <OleksandrB@metalinkbb.com>
#

PATH_TO_TOOLCHAIN = /opt/star/tools/arm-uclibc-3.4.6/bin/
#/opt/toolchains/star/toolchain/tools/arm-uclibc-3.4.6/bin

KERNELDIR = /opt/star/kernel/linux-2.6.16-star/ 
#/opt/toolchains/star/kernel_source
CROSS_COMPILE = ${PATH_TO_TOOLCHAIN}/arm-linux-uclibc-
CC = ${CROSS_COMPILE}gcc
LD = ${CROSS_COMPILE}ld


###EXTRA_CFLAGS=-O2 -DUNSAFE_DEMO_PATCH

export KERNELDIR CROSS_COMPILE EXTRA_CFLAGS

all:
	make -f Makefile.2.6.inc $@

%:
	make -f Makefile.2.6.inc $@

