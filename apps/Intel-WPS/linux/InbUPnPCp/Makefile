#Linux Makefile

ARCHIVE = InbUPnPCp.lib
OBJS = InbUPnPCp.o

override INCLUDE += -I. -I../../common/inc -I../inc 
CXXFLAGS += $(INCLUDE) -I$(STL_INCDIR)

$(ARCHIVE): $(OBJS)
	$(LD) -r -o $(ARCHIVE) $(OBJS) -L$(STL_LIBDIR) 

InbUPnPCp.o: ../../common/InbUPnPCp/InbUPnPCp.cpp ../../common/inc/InbUPnPCp.h 
	$(CXX) -c $(CXXFLAGS) -o InbUPnPCp.o ../../common/InbUPnPCp/InbUPnPCp.cpp

clean:
	rm -f $(OBJS) $(OBJS:%.o=%.d)
	rm -f $(ARCHIVE)

debug: default

-include $(OBJS:%.o=%.d)
