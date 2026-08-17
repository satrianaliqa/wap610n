#Linux Makefile

ARCHIVE = InbEap.lib
OBJS = InbEap.o

override INCLUDE += -I. -I../../common/inc -I../inc 
CXXFLAGS += $(INCLUDE) -I$(STL_INCDIR)

$(ARCHIVE): $(OBJS)
	$(LD) -r -o $(ARCHIVE) $(OBJS) -L$(STL_LIBDIR) 

InbEap.o: ../../common/InbEap/InbEap.cpp ../../common/inc/InbEap.h 
	$(CXX) -c $(CXXFLAGS) -o InbEap.o ../../common/InbEap/InbEap.cpp

clean:
	rm -f $(OBJS) $(OBJS:%.o=%.d)
	rm -f $(ARCHIVE)

debug: default

-include $(OBJS:%.o=%.d)
