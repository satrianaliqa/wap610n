#$Id: Makefile 4780 2010-07-08 10:48:52Z andreit $
#Linux Makefile

#BLDTYPE = release
#BLDTYPE = debug

#export CXX = g++
#export CC = gcc
#ifeq ($(BLDTYPE),debug)
CXXFLAGS += -MMD -Wall -g -D_REENTRANT -D_TUDEBUGTRACE -DPROVISION_IE -DMTLK_IF=1
CFLAGS += -MMD -Wall -Wnested-externs -g -D_REENTRANT -D_TUDEBUGTRACE -DPROVISION_IE -DMTLK_IF=1
STL_INCDIR ?= /usr/local/include
STL_LIBDIR ?= /usr/local/lib
CXX_LDFLAGS ?= -L/usr/local/lib
#else
#CXXFLAGS += -Wall -D_REENTRANT -DPROVISION_IE
#CFLAGS += -Wall -Wnested-externs -D_REENTRANT -DPROVISION_IE
#endif
LDFLAGS += -r
#LIBPATH = -L/usr/local/lib
#INCLUDE = -I/usr/local/include
UUID_LIB ?= -L. -luuid

ifeq ($(USE_NFC), 1)
NFC_LIBS  = /usr/lib/libnfc_mapping_pn53x.dll /usr/lib/wpsnfc.dll
endif

export CFLAGS CXXFLAGS LDFLAGS STL_INCDIR STL_LIBDIR CXX_LDFLAGS UUID_LIB

LIBS =  trace/tutrace.lib \
	slist/slist.lib \
	Portability/Portability.lib \
	UdpLib/UdpLib.lib \
	InbEap/InbEap.lib \
	InbIp/InbIp.lib \
	InbWlan/InbWlan.lib \
	UPnPILib/UPnPILib.lib \
	UPnPCpLib/UPnPCpLib.lib \
	InbUPnPCp/InbUPnPCp.lib \
	UPnPDevLib/UPnPDevLib.lib \
	InbUPnPDev/InbUPnPDev.lib \
	Transport/Transport.lib \
	StateMachine/StateMachine.lib \
	RegProtocol/RegProtocol.lib \
	MasterControl/MasterControl.lib \
	TlvBase/WscTlvBase.lib\
	WscCmd/WscCmd.lib \
	$(NFC_LIBS) \
	-lpthread -lssl -lcrypto $(LIBM) $(CXXLIBS)

OBJS =  InbEap/InbEap.o \
	InbIp/InbIp.o \
	InbUPnPCp/InbUPnPCp.o \
	InbUPnPDev/InbUPnPDev.o \
	InbWlan/InbWlan.o \
	MasterControl/MasterControl.o \
	MasterControl/Info.o \
	Portability/Portability.o \
	RegProtocol/ProtoUtils.o \
	RegProtocol/RegProtocol.o \
	RegProtocol/RegProtoMsgs.o \
	slist/slist.o \
	StateMachine/StateMachine.o \
	StateMachine/RegistrarSM.o \
	StateMachine/EnrolleeSM.o \
	StateMachine/APProxy.o \
	TlvBase/WscTlvBase.o \
	TlvBase/RegProtoTlv.o \
	trace/tutrace.o \
	Transport/Transport.o \
	UdpLib/UdpLib.o \
	UPnPCpLib/ILibSSDPClient.o \
	UPnPCpLib/UPnPControlPoint.o \
	UPnPDevLib/UPnPMicroStack.o \
	UPnPILib/ILibAsyncServerSocket.o \
	UPnPILib/ILibAsyncSocket.o \
	UPnPILib/ILibAsyncUDPSocket.o \
	UPnPILib/ILibParsers.o \
	UPnPILib/ILibWebClient.o \
	UPnPILib/ILibWebServer.o \
	WscCmd/WscCmd.o \
	WscCmd/wpa_ctrl.o \

all:
	$(MAKE) -C trace
	$(MAKE) -C slist
	$(MAKE) -C Portability
	$(MAKE) -C TlvBase
	$(MAKE) -C UdpLib
	$(MAKE) -C InbEap
	$(MAKE) -C InbIp
	$(MAKE) -C InbWlan
	$(MAKE) -C UPnPILib
	$(MAKE) -C UPnPCpLib
	$(MAKE) -C InbUPnPCp
	$(MAKE) -C UPnPDevLib
	$(MAKE) -C InbUPnPDev
	$(MAKE) -C Transport
	$(MAKE) -C StateMachine
	$(MAKE) -C RegProtocol
	$(MAKE) -C MasterControl
	$(MAKE) -C WscCmd
	$(MAKE) wsccmd

wsccmd: $(OBJS)
	$(CXX) $(CXX_LDFLAGS) -g -o $@ $(OBJS) -lpthread -lssl -lcrypto $(LIBM) $(CXXLIBS) $(UUID_LIB)
#	$(LD) -r -o wsccmd $(OBJS) $(LIBS) -L$(STL_LIBDIR)
#	$(LD) $(LDFLAGS) $(OBJS) $(LIBS) $(LIBPATH) -o wsccmd
	if [ "$(STRIP)" ]; then cp wsccmd wsccmd_debug && $(STRIP) wsccmd; fi

clean:
	$(MAKE) -C trace clean
	$(MAKE) -C slist clean
	$(MAKE) -C Portability clean
	$(MAKE) -C TlvBase clean
	$(MAKE) -C UdpLib clean
	$(MAKE) -C InbEap clean
	$(MAKE) -C InbIp clean
	$(MAKE) -C InbWlan clean
	$(MAKE) -C UPnPILib clean
	$(MAKE) -C UPnPCpLib clean
	$(MAKE) -C InbUPnPCp clean
	$(MAKE) -C UPnPDevLib clean
	$(MAKE) -C InbUPnPDev clean
	$(MAKE) -C Transport clean
	$(MAKE) -C StateMachine clean
	$(MAKE) -C RegProtocol clean
	$(MAKE) -C MasterControl clean
	$(MAKE) -C WscCmd clean
	rm -f wsccmd

romfs:

.FONY: all clean
-include $(OBJS:%.o=%.d)
