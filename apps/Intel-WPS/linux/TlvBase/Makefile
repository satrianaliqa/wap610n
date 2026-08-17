#Linux Makefile

ARCHIVE = WscTlvBase.lib 
OBJS = WscTlvBase.o RegProtoTlv.o

COMMON = ../../common
COMINC = $(COMMON)/inc/
COMSRC = $(COMMON)/TlvBase/

override INCLUDE += -I. -I../../common/inc/ -I../inc -I../TlvBase
CXXFLAGS += $(INCLUDE) -I$(STL_INCDIR)

$(ARCHIVE): $(OBJS)
	$(LD) -r -o $(ARCHIVE) $(OBJS) -L$(STL_LIBDIR)

WscTlvBase.o: $(COMSRC)WscTlvBase.cpp 
	$(CXX) -c $(CXXFLAGS) -o WscTlvBase.o $(COMSRC)WscTlvBase.cpp

RegProtoTlv.o: $(COMSRC)RegProtoTlv.cpp 
	$(CXX) -c $(CXXFLAGS) -o RegProtoTlv.o $(COMSRC)RegProtoTlv.cpp

clean:
	rm -f $(OBJS) $(OBJS:%.o=%.d)
	rm -f $(ARCHIVE)

debug: default

-include $(OBJS:%.o=%.d)
