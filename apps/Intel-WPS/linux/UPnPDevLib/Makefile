#Linux Makefile

ARCHIVE  = UPnPDevLib.lib
OBJS = UPnPMicroStack.o

override INCLUDE += -I. -I../../common/inc

CFLAGS += $(INCLUDE)

CFLAGS += -D_POSIX

$(ARCHIVE): $(OBJS)
	$(LD) -r -o $(ARCHIVE) $(OBJS)

UPnPMicroStack.o: ../../common/UPnPDevLib/UPnPMicroStack.c ../../common/inc/UPnPMicroStack.h
	$(CC) -c $(CFLAGS) -o UPnPMicroStack.o ../../common/UPnPDevLib/UPnPMicroStack.c

clean:
	rm -f $(OBJS) $(OBJS:%.o=%.d)
	rm -f $(ARCHIVE)

debug: default

-include $(OBJS:%.o=%.d)
