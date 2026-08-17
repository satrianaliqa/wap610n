
CC = arm-linux-gcc
PATH := $(PATH):/opt/star/tools/arm-uclibc-3.4.6/bin
###CFLAGS = -g -fno-common -fno-builtin -Wall  -Dlinux -D__linux__ -Dunix
INCLUDE =
###LIBPATH = -lpthread
LIB =

STAR_PLATFORM = yes

include Makefile

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.



# IF YOU PUT ANYTHING HERE IT WILL GO AWAY
