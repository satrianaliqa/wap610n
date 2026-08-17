#Linux Makefile

ARCHIVE  = RegProtocol.lib
OBJS = RegProtocol.o ProtoUtils.o RegProtoMsgs.o

COMMON = ../../common
COMINC = $(COMMON)/inc/
COMSRC = $(COMMON)/RegProtocol/

override INCLUDE += -I. -I../inc -I../../common/inc -I../../../../lib/libssl/include

CXXFLAGS += $(INCLUDE) -I$(STL_INCDIR)

$(ARCHIVE): $(OBJS)
	$(LD) -r -o $(ARCHIVE) $(OBJS) -L$(STL_LIBDIR)

RegProtocol.o: $(COMSRC)RegProtocol.cpp 
	$(CXX) -c $(CXXFLAGS) -o RegProtocol.o $(COMSRC)RegProtocol.cpp

ProtoUtils.o: $(COMSRC)ProtoUtils.cpp 
	$(CXX) -c $(CXXFLAGS) -o ProtoUtils.o $(COMSRC)ProtoUtils.cpp

RegProtoMsgs.o: $(COMSRC)RegProtoMsgs.cpp 
	$(CXX) -c $(CXXFLAGS) -o RegProtoMsgs.o $(COMSRC)RegProtoMsgs.cpp

clean:
	rm -f $(OBJS) $(OBJS:%.o=%.d)
	rm -f $(ARCHIVE)

debug: default

-include $(OBJS:%.o=%.d)
