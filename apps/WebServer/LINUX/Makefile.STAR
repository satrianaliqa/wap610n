#PATH := ~/star/Networking-versions/star/tools/arm-uclibc-3.4.6/bin:$(PATH)
#PLATFORM_TOOLCHAIN = ~/star/Networking-versions/star/tools/arm-uclibc-3.4.6
#CC = $(PLATFORM_TOOLCHAIN)/bin/arm-linux-gcc
#ROOTFS_WEB = /opt/rootfs-star/root/mtlk/web
CC = arm-linux-gcc
PATH := $(PATH):/opt/star/tools/arm-uclibc-3.4.6/bin
PLATFORM_TOOLCHAIN = /opt/star/tools/arm-uclibc-3.4.6
#ROOTFS_WEB = ~/METALINK/2.3.5.UM.1.08/src/UMEDIA-nv-1.0.8/_wlan-2.3.5.24/web
ROOTFS_WEB = /home/jacky.yang/METALINK/11/nv-1.2.10-UMedia/_wlan-vb01.04-4MB/web
#ROOTFS_WEB = /home/jacky.yang/METALINK/VB1.09-WLS-2.3.5.37/nv-1.2.12/_wlan.tar.gz/web
###CFLAGS = -fno-common -fno-builtin -Wall  -Dlinux -D__linux__ -Dunix
PLATFORM_CFLAGS = -Os -march=armv4 -msoft-float
INCLUDE =
LIBPATH = 
LIB =
ADDITIONAL_LIBS = -lpthread
ADDITIONAL_OBJS = $(PLATFORM_TOOLCHAIN)/lib/libz.a

include Makefile

install: webs
	cp webs $(ROOTFS_WEB)

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.



# IF YOU PUT ANYTHING HERE IT WILL GO AWAY
