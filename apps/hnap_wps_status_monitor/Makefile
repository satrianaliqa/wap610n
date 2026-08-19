EXEC = hnap_wps_status_monitor
OBJS = hnap_wps_status_monitor.o

.PHONY: all clean

all: 	$(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

clean:
	-rm -f $(EXEC) *.elf *.gdb *.o
