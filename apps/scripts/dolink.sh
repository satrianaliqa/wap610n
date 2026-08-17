#!/bin/bash

###############################################################
#            W A R N I N G ! ! !
#
# This file is DEPRECATED!!!
#
# Do not add any new targets to this file.
# It is kept in the toolchain for one purpose only:
# To allow compiling old versions of hostapd/wpa_supplicant
# applications, which expect to find an apps/l/openssl folder.
#
# New versions of hostpad/wpa_supplicant, as well as all 
# other apps, exist directly under the apps folder, 
# and they are compiled if they listed in the 
# boards/<boardname>/appsconfig/.config file.
#
###############################################################

FOUND=
TOPDIR=${E_TOPDIR}
cd ${TOPDIR}

function tst()
{
	if [ $1 != 0 ]
	then
		echo "Error on $2"
		exit $1
	fi
	return 0
}


function lookfor()
{
	FOUND=`ls -t | grep -m1 "$1"`
	tst $? "$1"
	return $?
}


function link()
{
	ln -sf ${TOPDIR}/$1 ${TOPDIR}/$2
	tst $? "link $1 <- $2"
}

cd ${TOPDIR}

lookfor "openssl"
OPENSSL=${FOUND}

if [ ! -d ${TOPDIR}/l ]
then
	mkdir ${TOPDIR}/l
	tst $? "Can't create directory ${TOPDIR}/l"
fi


link ${OPENSSL} l/openssl 

exit 0

