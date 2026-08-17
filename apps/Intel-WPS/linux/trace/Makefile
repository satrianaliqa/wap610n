#Linux Makefile

ARCHIVE  = tutrace.lib
OBJS = tutrace.o

override INCLUDE += -I. -I../../common/inc

CXXFLAGS += $(INCLUDE) -D_TUDEBUGTRACE

$(ARCHIVE): $(OBJS)
	$(LD) -r -o $(ARCHIVE) $(OBJS)

tutrace.o: ../../common/trace/tutrace.cpp ../../common/inc/tutrace.h
	$(CXX) -c $(CXXFLAGS) -o tutrace.o ../../common/trace/tutrace.cpp

clean:
	rm -f $(OBJS) $(OBJS:%.o=%.d)
	rm -f $(ARCHIVE)

debug: default

-include $(OBJS:%.o=%.d)
