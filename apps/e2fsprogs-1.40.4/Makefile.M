E_HOST=arm
E_CC=/usr/local/arm-linux/bin/gcc
E_LD=/usr/local/arm-linux/bin/ld
E_CFLAGS=-I/usr/local/arm-linux/include/
E_LDFLAGS=-L/usr/local/arm-linux/lib/

export
#export E_HOST E_CC E_LD E_CFLAGS E_LDFLAGS

CONF:
	make -f Makefile.MTLK CONF
COMP:
	make -f Makefile.MTLK COMP
