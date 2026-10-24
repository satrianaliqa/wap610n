PLATFORM_TOOLCHAIN ?= ../../../tools/arm-uclibc-3.4.6
CROSS_COMPILE ?= $(PLATFORM_TOOLCHAIN)/bin/arm-linux-
CC = $(CROSS_COMPILE)gcc
AR = $(CROSS_COMPILE)ar
STRIP = $(CROSS_COMPILE)strip
ROOTFS_WEB ?= ../../../rootfs-star/root/mtlk/web
PLATFORM_CFLAGS = -Os -march=armv4 -msoft-float
INCLUDE =
LIBPATH = 
LIB =
ADDITIONAL_LIBS = -lpthread
ADDITIONAL_OBJS = $(PLATFORM_TOOLCHAIN)/lib/libz.a

include Makefile

.PHONY: install

install: webs
	$(STRIP) webs
	cp -f webs $(ROOTFS_WEB)/webs
	cp -f webs ../../../rootfs/root/mtlk/web/webs
