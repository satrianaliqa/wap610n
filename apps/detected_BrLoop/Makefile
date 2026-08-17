EXEC = DetectedBrLoop
OBJS = Detected_BrLoop.o linkUtil.o

CFLAGS += -I$(ROOTDIR)/$(LINUXDIR)/drivers/char/

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

romfs:
	$(ROMFSINST) /usr/bin/$(EXEC)

clean:
	-rm -f $(EXEC) *.elf *.gdb *.o

