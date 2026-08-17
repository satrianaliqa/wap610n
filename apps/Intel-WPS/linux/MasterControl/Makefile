#Linux Makefile

ARCHIVE = MasterControl.lib
OBJS = MasterControl.o Info.o

override INCLUDE += -I. -I../../common/MasterControl/ -I../../common/inc/ -I../inc -I../../StateMachine -I../../../e2fsprogs/lib/
CXXFLAGS += $(INCLUDE) -I$(STL_INCDIR)

$(ARCHIVE): $(OBJS)
	$(LD) -r -o $(ARCHIVE) $(OBJS) -L$(STL_LIBDIR)

MasterControl.o: ../../common/MasterControl/MasterControl.cpp ../../common/inc/WscCommon.h
	$(CXX) -c $(CXXFLAGS) -o MasterControl.o ../../common/MasterControl/MasterControl.cpp 

Info.o: ../../common/MasterControl/Info.cpp 
	$(CXX) -c $(CXXFLAGS) -o Info.o ../../common/MasterControl/Info.cpp 

clean:
	rm -f $(OBJS) $(OBJS:%.o=%.d)
	rm -f $(ARCHIVE)

debug: default

-include $(OBJS:%.o=%.d)
