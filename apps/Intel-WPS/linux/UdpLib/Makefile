#Linux Makefile

ARCHIVE  = UdpLib.lib
OBJS = UdpLib.o

override INCLUDE += -I. -I../../common/inc

CXXFLAGS += $(INCLUDE)

$(ARCHIVE): $(OBJS)
	$(LD) -r -o $(ARCHIVE) $(OBJS)

UdpLib.o: ../../common/UdpLib/UdpLib.cpp ../../common/inc/UdpLib.h
	$(CXX) -c $(CXXFLAGS) -o UdpLib.o ../../common/UdpLib/UdpLib.cpp

clean:
	rm -f $(OBJS) $(OBJS:%.o=%.d)
	rm -f $(ARCHIVE)

debug: default

-include $(OBJS:%.o=%.d)
