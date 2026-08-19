#########################################################
#
#

EXEC = get_env_param
OBJS = get_env_param.o
CFLAGS+=-I. -g -Wall -DSTANDALONE

all: $(EXEC)
$(EXEC): $(OBJS)  
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS) 

romfs:
	$(ROMFSINST)    /bin/$(EXEC)

clean:
	-rm -f $(EXEC) *.elf *.gdb *.o
