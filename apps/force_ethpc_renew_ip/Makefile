EXEC = force_ethpc_renew_ip
OBJS = force_ethpc_renew_ip.o

.PHONY: all clean

all: 	$(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

clean:
	-rm -f $(EXEC) *.elf *.gdb *.o
