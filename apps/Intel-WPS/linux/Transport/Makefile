#Linux Makefile

ARCHIVE = Transport.lib
OBJS = Transport.o

override INCLUDE += -I. -I../../common/inc -I../inc 
CXXFLAGS += $(INCLUDE) -I$(STL_INCDIR)

$(ARCHIVE): $(OBJS)
	$(LD) -r -o $(ARCHIVE) $(OBJS) -L$(STL_LIBDIR) 

Transport.o: ../../common/Transport/Transport.cpp ../../common/inc/Transport.h ../../common/inc/TransportBase.h
	$(CXX) -c $(CXXFLAGS) -o Transport.o ../../common/Transport/Transport.cpp

clean:
	rm -f $(OBJS) $(OBJS:%.o=%.d)
	rm -f $(ARCHIVE)

debug: default

-include $(OBJS:%.o=%.d)
