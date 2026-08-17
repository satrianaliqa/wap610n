#Linux Makefile

ARCHIVE = WscCmd.lib
OBJS = WscCmd.o wpa_ctrl.o

override INCLUDE += -I. -I../../common/WscCmd/ -I../../common/MasterControl/ -I../../common/inc/ -I../inc -I../../StateMachine
CXXFLAGS += $(INCLUDE) -I$(STL_INCDIR)

$(ARCHIVE): $(OBJS)
	$(LD) -r -o $(ARCHIVE) $(OBJS) -L$(STL_LIBDIR) 

#WscCmd.o: WscCmdL.cpp 
WscCmd.o: ../../common/WscCmd/WscCmd.cpp 
	$(CXX) -c $(CXXFLAGS) -o WscCmd.o ../../common/WscCmd/WscCmd.cpp 

wpa_ctrl.o: ../../common/WscCmd/wpa_ctrl.c
	$(CC) -c $(CXXFLAGS) -o $@ ../../common/WscCmd/wpa_ctrl.c

clean:
	rm -f $(OBJS) $(OBJS:%.o=%.d)
	rm -f $(ARCHIVE)

debug: default

-include $(OBJS:%.o=%.d)
