#
# Makefile for the GoAhead web server reference source base
#  for the Linux OS
#
# Copyright (c) GoAhead Software, Inc. 1995-2000
#
# $Id: Makefile,v 1.2 2002/11/01 16:20:19 mmitchell Exp $
#

all:	compile 

ARCH	= libwebs.a
NAME	= webs

# User Management switch
# UMSW	= -DUSER_MANAGEMENT_SUPPORT
UMSW	= 

# Digest Access switch
DASW	= -DDIGEST_ACCESS_SUPPORT

#
#	SSL switches
#
# You must uncomment this line to enable SSL
# SSLPATCHFILE = ../websSSL.o
#
# Then uncomment these three lines if you are using the RSAREF library
# and set them to the correct path.
#
# SSLINC = /RSASSL/library/include
# SSLLIB = /RSASSL/library/lib/libsslc.a
# SSLSW	= -DWEBS_SSL_SUPPORT -I$(SSLINC)
#
# Or alternatively,
#
# Uncomment these three lines if you are using the OpenSSL library.
# Change the value of SSLINC to the path where your OpenSSL header
# files reside, and change the value of SSLLIB to the path to your
# OpenSSL libssl.a and libcrypto.a.
#
# SSLINC = /usr/include/openssl
# SSLLIB = /usr/lib/libssl.a /usr/lib/libcrypto.a
# SSLSW	= -DWEBS_SSL_SUPPORT -I$(SSLINC) -DOPENSSL


# If-Modified-Support switches (requires math library, libm.a)
# IFMODSW = -DWEBS_IF_MODIFIED_SUPPORT
# IFMODLIB = /usr/lib/libm.a

# Dependencies
DEPEND_FILES	= ../asp.o ../balloc.o ../base64.o ../cgi.o ../default.o  \
				  ../ejlex.o ../ejparse.o ../form.o \
				  ../h.o ../handler.o ../mime.o ../misc.o ../mt_api.o ../mt_validation.o ../page.o ../upldform.o  \
				  ../ringq.o ../rom.o \
				  ../sock.o ../sockGen.o $(SSLPATCHFILE) \
				  ../security.o ../sym.o ../uemf.o ../url.o ../value.o \
				  ../md5c.o ../um.o ../umui.o ../websda.o ../emfdb.o \
				  ../wireless.o ../wps.o ../utils.o ../management.o \
				  ../webrom.o ../webs.o ../websuemf.o ../mt_task.o \
				  ../device_discover.o \
				  ../xml_tree.o ../stack.o ../linked_list.o ../hnap_config_metalink.o ../hnap_implementation.o main.o
# CFLAGS	= -DWEBS -DUEMF -DOS="LINUX" -DLINUX $(UMSW) $(DASW) $(SSLSW) $(IFMODSW)

############### Settings for Snapgear -these are overridden by other platforms ################
PLATFORM_CFLAGS ?= -mbig-endian -mapcs-32 -mcpu=xscale -mtune=xscale -Wa,-mcpu=xscale \
        -malignment-traps -Uarm -O3 -fomit-frame-pointer
# SNAPGEAR = /home/arnonm/Desktop/svn/wl/linux/trunk/snapgear/dongle-2.0
SNAPGEAR ?= /tools/dongle-2.0
CROSS_COMPILE ?= $(SNAPGEAR)/tools/ucfront-gcc arm-linux-
CC ?= $(CROSS_COMPILE)gcc
ADDITIONAL_LIBS ?= -lpthread -lz
ADDITIONAL_OBJS ?=
############### END Settings for Snapgear ################

# CFLAGS	= -DWEBS -DUEMF $(PLATFORM_CFLAGS) $(UMSW) $(DASW) $(SSLSW) $(IFMODSW) -DOS="LINUX" -DLINUX 
CFLAGS  = -DMT_USE_STAT_TASK -DWEBS -DUEMF $(PLATFORM_CFLAGS) $(UMSW) $(DASW) $(SSLSW) $(IFMODSW) -DOS="LINUX" -DLINUX 
OTHERS	= -DB_STATS -DB_FILL -DDEBUG
DEBUG	= -g -Wall
IFLAGS 	= -I.. 
LDFLAGS	= $(SSLLIB) $(IFMODLIB) $(ADDITIONAL_LIBS)

#Add for HNAP - U-Media Ricky Cao on Jun 04 2008
CFLAGS += -I../../expat/lib -I../../expat -DWITH_METALINK -DDUAL_BAND
LDFLAGS += -L../../expat -lexpat -DWITH_METALINK -DDUAL_BAND
#U-Media Ricky Cao on Jun 04 2008

compile: $(NAME)

#
#	Build archive of objects
#
$(ARCH): $(DEPEND_FILES)
	$(AR) $(ARFLAGS) $(ARCH) $?

#
#	Primary link
#
$(NAME): Makefile $(ARCH)
	$(CC) -o $(NAME) $(DEBUG) \
		$(CFLAGS) $(IFLAGS) \
		main.o $(ARCH) $(LDFLAGS) $(ADDITIONAL_OBJS) 

clean:
	rm -f $(NAME) $(ARCH) $(DEPEND_FILES)

#
#	Dependencies
#
../asp.o:  ../webs.h ../wsIntrn.h ../ej.h ../ejIntrn.h ../uemf.h

../balloc.o: ../balloc.c ../uemf.h

../base64.o:  ../base64.c ../webs.h ../wsIntrn.h  ../ej.h ../ejIntrn.h ../uemf.h

../cgi.o:  ../webs.h ../wsIntrn.h ../uemf.h

../default.o:  ../default.c ../webs.h ../wsIntrn.h ../ej.h ../ejIntrn.h ../uemf.h

../ejlex.o:  ../ejlex.c ../ej.h ../ejIntrn.h ../uemf.h

../ejparse.o:  ../ejparse.c ../ej.h ../ejIntrn.h ../uemf.h ../mt_api.h ../mt_validation.h 

../emfdb.o:  ../emfdb.h ../wsIntrn.h ../uemf.h

../form.o:  ../form.c ../webs.h ../wsIntrn.h ../ej.h ../ejIntrn.h ../uemf.h

../h.o:  ../h.c ../uemf.h

../handler.o:  ../handler.c ../webs.h ../wsIntrn.h ../ej.h ../ejIntrn.h ../uemf.h

../md5c.o:  ../md5.h ../wsIntrn.h ../uemf.h

../mime.o:  ../mime.c ../webs.h ../wsIntrn.h ../ej.h ../ejIntrn.h ../uemf.h

../misc.o:  ../misc.c ../uemf.h

../mt_api.o:	../mt_api.c ../mt_api.h ../mt_validation.h ../uemf.h

../mt_validation.o:	../mt_validation.c ../mt_api.h ../mt_validation.h ../uemf.h

../page.o:  ../page.c ../webs.h ../wsIntrn.h ../ej.h ../ejIntrn.h ../uemf.h

../ringq.o:  ../ringq.c ../uemf.h

../rom.o:  ../rom.c ../webs.h ../wsIntrn.h ../ej.h ../ejIntrn.h ../uemf.h

../security.o:  ../security.c ../webs.h ../wsIntrn.h ../ej.h ../ejIntrn.h ../uemf.h

../sock.o:  ../sock.c ../uemf.h

../sockGen.o:  ../sockGen.c ../uemf.h

../sym.o:  ../sym.c ../uemf.h

../uemf.o:  ../uemf.c ../uemf.h

../um.o:  ../webs.h ../wsIntrn.h ../um.h ../uemf.h

../umui.o:  ../webs.h ../wsIntrn.h ../um.h ../uemf.h

../url.o:  ../url.c ../webs.h ../wsIntrn.h ../ej.h ../ejIntrn.h ../uemf.h

../value.o:  ../value.c ../uemf.h

../webrom.o:  ../webrom.c ../webs.h ../wsIntrn.h ../uemf.h

../webs.o:  ../webs.c ../webs.h ../wsIntrn.h ../ej.h ../ejIntrn.h ../uemf.h ../wireless.h ../wps.h ../management.h ../utils.h

../websda.o:  ../webs.h ../wsIntrn.h ../websda.h ../uemf.h

../websuemf.o:  ../websuemf.c ../webs.h ../wsIntrn.h ../ej.h ../ejIntrn.h ../uemf.h

../websSSL.o:  ../websSSL.c ../websSSL.h ../wsIntrn.h ../ej.h ../ejIntrn.h ../uemf.h

../upldform.o:  ../upldform.c ../webs.h

../xml_tree.o: ../xml_tree.c ../xml_tree.h ../stack.h ../linked_list.h

../stack.o: ../stack.c ../stack.h ../linked_list.h

../linked_list.o: ../linked_list.c ../linked_list.h

../hnap_config_metalink.o: ../hnap_config_metalink.c ../hnap_config_metalink.h ../hnap_interfaces.h ../xml_tree.h ../webs.h

../hnap_implementation.o: ../hnap_implementation.c ../hnap_interfaces.h ../xml_tree.h

../mt_task.o: ../mt_task.h ../mt_api.h ../uemf.h
../device_discover.o: ../device_discover.c ../device_discover.h ../uemf.h

#Jacky.Yang 30-May-2008, Define U-Media API
../wireless.o: ../wireless.c ../wireless.h
../utils.o: ../utils.c ../utils.h
../management.o: ../management.c ../management.h
../wps.o: ../wps.c ../wps.h

main.o:  main.c ../wsIntrn.h ../webs.h ../ej.h ../ejIntrn.h ../uemf.h ../mt_task.h ../hnap_interfaces.h


#
#	Transition rules (add -o to put object in right directory)
#
.c.o:
	$(CC) -c -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $<
