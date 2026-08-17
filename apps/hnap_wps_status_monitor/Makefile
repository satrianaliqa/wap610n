EXEC = hnap_wps_status_monitor
OBJS = hnap_wps_status_monitor.o

all: 	$(EXEC)

$(EXEC): $(OBJS)
	$(CC) -o $@ $(OBJS)

clean:
	-rm -f $(EXEC) *.elf *.gdb *.o
