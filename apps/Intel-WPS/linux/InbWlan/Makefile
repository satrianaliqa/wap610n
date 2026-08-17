#Linux Makefile

ARCHIVE = InbWlan.lib
OBJS = InbWlan.o

override INCLUDE += -I. -I../../common/inc -I../inc 
CXXFLAGS += $(INCLUDE) -I$(STL_INCDIR)

$(ARCHIVE): $(OBJS)
	$(LD) -r -o $(ARCHIVE) $(OBJS) -L$(STL_LIBDIR)

InbWlan.o: ../../common/InbWlan/InbWlan.cpp ../../common/inc/InbWlan.h 
	$(CXX) -c $(CXXFLAGS) -o InbWlan.o ../../common/InbWlan/InbWlan.cpp

clean:
	rm -f $(OBJS) $(OBJS:%.o=%.d)
	rm -f $(ARCHIVE)

debug: default

-include $(OBJS:%.o=%.d)
