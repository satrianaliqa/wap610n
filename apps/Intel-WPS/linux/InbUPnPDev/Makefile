#Linux Makefile

ARCHIVE = InbUPnPDev.lib
OBJS = InbUPnPDev.o

override INCLUDE += -I. -I../../common/inc -I../inc -I../../common/MasterControl
CXXFLAGS += $(INCLUDE) -I$(STL_INCDIR)

$(ARCHIVE): $(OBJS)
	$(LD) -r -o $(ARCHIVE) $(OBJS) -L$(STL_LIBDIR) 

InbUPnPDev.o: ../../common/InbUPnPDev/InbUPnPDev.cpp ../../common/inc/InbUPnPDev.h 
	$(CXX) -c $(CXXFLAGS) -o InbUPnPDev.o ../../common/InbUPnPDev/InbUPnPDev.cpp

clean:
	rm -f $(OBJS) $(OBJS:%.o=%.d)
	rm -f $(ARCHIVE)

debug: default

-include $(OBJS:%.o=%.d)
