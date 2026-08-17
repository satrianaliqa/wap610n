# Comment/uncomment the following line to disable/enable debugging
#DEBUG = y
# This Makefile has been simplified as much as possible, by putting all
# generic material, independent of this specific directory, into
# ./Rules.make. Read that file for details
ARCH=arm

TOPDIR  := $(shell pwd)
#$(shell cd ..; pwd)
##include $(TOPDIR)/Rules.make
ifeq ($(M),)
	M = $(TOPDIR)
endif
#include $(M)/Rules.make
CFLAGS=-Os -mlittle-endian -pipe -fno-common -fno-builtin -Dlinux -D__linux__ -Dunix  -DEMBED

# The headers are taken from the kernel
	INCLUDEDIR = $(KERNELDIR)/include


# This is useful if cross-compiling. Taken from kernel Makefile (CC changed)
AS      =$(CROSS_COMPILE)as
LD      =$(CROSS_COMPILE)ld
CC      =$(CROSS_COMPILE)gcc
CPP     =$(CC) -E
AR      =$(CROSS_COMPILE)ar
NM      =$(CROSS_COMPILE)nm
STRIP   =$(CROSS_COMPILE)strip
OBJCOPY =$(CROSS_COMPILE)objcopy
OBJDUMP =$(CROSS_COMPILE)objdump

# Add your debugging flag (or not) to CFLAGS
ifeq ($(DEBUG),y)
  DEBFLAGS = -O -g -DLEDS_DEBUG # "-O" is needed to expand inlines
else
  DEBFLAGS = -O2
endif

CFLAGS += $(DEBFLAGS)
#CFLAGS += -I..

obj-m := gpio.o
gpio-objs := gpioDevice.o 

##TARGET = led
##OBJS = $(TARGET).o
##SRC = ledDevice.o 

##all: .depend $(TARGET).o

##$(TARGET).o: $(SRC)
##	$(LD) -r $^ $(LDFLAGS) -o $@
##	#@$(CC) $(CFLAGS) $< -c -o $@

# $(TARGET).o: $(SRC:.c=.o)

##%.o: %.c
##	@$(CC) $(CFLAGS) $< -c -o $@

romfs:
	#$(ROMFSINST) /lib/modules/2.4.31-uc0/kernel/$(OBJS)
	#chmod a+x $(ROMFSDIR)/lib/modules/2.4.31-uc0/kernel/$(OBJS)
	#ED
	#cp $(OBJS) ../jffs2-etc/
	#chmod a+x ../jffs2-etc/$(OBJS)
	
clean:
	rm -f *.o *~ core .depend

depend .depend dep:
	$(CC) $(CFLAGS) -M *.c > $@


ifeq (.depend,$(wildcard .depend))
include .depend
endif
