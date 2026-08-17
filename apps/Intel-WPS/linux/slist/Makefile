#Linux Makefile

ARCHIVE  = slist.lib
OBJS = slist.o

override INCLUDE += -I. -I../../common/inc

CFLAGS += $(INCLUDE)

$(ARCHIVE): $(OBJS)
	$(LD) -r -o $(ARCHIVE) $(OBJS)

slist.o: ../../common/slist/slist.c ../../common/inc/slist.h
	$(CC) -c $(CFLAGS) -o slist.o ../../common/slist/slist.c

clean:
	rm -f $(OBJS) $(OBJS:%.o=%.d)
	rm -f $(ARCHIVE)

debug: default

-include $(OBJS:%.o=%.d)
