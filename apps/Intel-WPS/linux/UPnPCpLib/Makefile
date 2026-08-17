#Linux Makefile

ARCHIVE  = UPnPCpLib.lib
OBJS = ILibSSDPClient.o UPnPControlPoint.o

override INCLUDE += -I. -I../../common/inc

CFLAGS += $(INCLUDE)

CFLAGS += -D_POSIX

$(ARCHIVE): $(OBJS)
	$(LD) -r -o $(ARCHIVE) $(OBJS)

ILibSSDPClient.o: ../../common/UPnPCpLib/ILibSSDPClient.c ../../common/inc/ILibSSDPClient.h
	$(CC) -c $(CFLAGS) -o ILibSSDPClient.o ../../common/UPnPCpLib/ILibSSDPClient.c

UPnPControlPoint.o: ../../common/UPnPCpLib/UPnPControlPoint.c ../../common/inc/UPnPControlPoint.h
	$(CC) -c $(CFLAGS) -o UPnPControlPoint.o ../../common/UPnPCpLib/UPnPControlPoint.c

clean:
	rm -f $(OBJS) $(OBJS:%.o=%.d)
	rm -f $(ARCHIVE)

debug: default

-include $(OBJS:%.o=%.d)
