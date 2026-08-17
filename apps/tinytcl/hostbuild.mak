ifndef ROOTDIR
	ROOTDIR = $(shell pwd)/../..

ifdef STAR_PLATFORM
	-include hostbuild.import.STAR
else 
	ifndef E_TOPDIR
		-include hostbuild.import
	endif
endif

	UCLINUX_BUILD_USER=1
	UCLINUX_BUILD_LIB=1
endif
