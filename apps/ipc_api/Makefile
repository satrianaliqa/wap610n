# Jacky.Yang 25-Mar-2008
# Makefile for send_event
#
# Makefile by Jacky Yang,
# 
# IPC, for send event to goahead that will reload firewall rules.
#

EXEC = send_event
CFLAGS += -I$(ROOTDIR)/$(LINUXDIR)/drivers/char

all: $(EXEC)

$(EXEC): $(EXEC).c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $@.c $(LDLIBS)

romfs:
	$(ROMFSINST) /usr/sbin/$(EXEC)

clean:
	-rm -f $(EXEC) *.elf *.gdb *.o