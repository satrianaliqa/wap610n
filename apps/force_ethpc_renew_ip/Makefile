EXEC = force_ethpc_renew_ip
OBJS = force_ethpc_renew_ip.o

all: 	$(EXEC)

$(EXEC): $(OBJS)
	$(CC) -o $@ $(OBJS)

clean:
	-rm -f $(EXEC) *.elf *.gdb *.o
