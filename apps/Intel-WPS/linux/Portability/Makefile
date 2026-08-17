#Linux Makefile

ARCHIVE = Portability.lib 
OBJS = Portability.o 

COMMON = ../../common
COMINC = $(COMMON)/inc/

override INCLUDE += -I. -I../../common/inc/ -I../inc -I../Portability
CFLAGS += $(INCLUDE) -g

$(ARCHIVE): $(OBJS) 
	$(LD) -r -o $(ARCHIVE) $(OBJS) 

Portability.o: Portability.c 
	$(CC) -c $(CFLAGS) -o Portability.o Portability.c

clean:
	rm -f $(OBJS) $(OBJS:%.o=%.d)
	rm -f $(ARCHIVE)

debug: default

-include $(OBJS:%.o=%.d)
