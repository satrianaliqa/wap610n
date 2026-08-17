#Linux Makefile

ARCHIVE = StateMachine.lib 
OBJS = StateMachine.o RegistrarSM.o EnrolleeSM.o APProxy.o

COMMON = ../../common
COMINC = $(COMMON)/inc/
COMSRC = $(COMMON)/StateMachine/

override INCLUDE += -I. -I../../common/inc/ -I../inc -I../StateMachine
CXXFLAGS += $(INCLUDE) -D__LINUX -I$(STL_INCDIR)

$(ARCHIVE): $(OBJS)
	$(LD) -r -o $(ARCHIVE) $(OBJS) -L$(STL_LIBDIR)

StateMachine.o: $(COMSRC)StateMachine.cpp $(COMINC)StateMachine.h
	$(CXX) -c $(CXXFLAGS) -o StateMachine.o $(COMSRC)StateMachine.cpp

RegistrarSM.o: $(COMSRC)RegistrarSM.cpp $(COMINC)StateMachine.h
	$(CXX) -c $(CXXFLAGS) -o RegistrarSM.o $(COMSRC)RegistrarSM.cpp

EnrolleeSM.o: $(COMSRC)EnrolleeSM.cpp $(COMINC)StateMachine.h
	$(CXX) -c $(CXXFLAGS) -o EnrolleeSM.o $(COMSRC)EnrolleeSM.cpp

APProxy.o: $(COMSRC)APProxy.cpp $(COMINC)StateMachine.h
	$(CXX) -c $(CXXFLAGS) -o APProxy.o $(COMSRC)APProxy.cpp

clean:
	rm -f $(OBJS) $(OBJS:%.o=%.d)
	rm -f $(ARCHIVE)

debug: default

-include $(OBJS:%.o=%.d)
