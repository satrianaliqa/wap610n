#Linux Makefile

ARCHIVE = InbIp.lib
OBJS = InbIp.o

override INCLUDE += -I. -I../../common/inc/ -I../inc 
CXXFLAGS += $(INCLUDE) -I$(STL_INCDIR)

$(ARCHIVE): $(OBJS)
	$(LD) -r -o $(ARCHIVE) $(OBJS) -L$(STL_LIBDIR) 

InbIp.o: InbIp.cpp ../../common/inc/InbIp.h 
	$(CXX) -c $(CXXFLAGS) -o InbIp.o InbIp.cpp

clean:
	rm -f $(OBJS) $(OBJS:%.o=%.d)
	rm -f $(ARCHIVE)

debug: default

-include $(OBJS:%.o=%.d)
