#!/bin/bash

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

lookfor "simpdiff"
SIMPDIFF=${FOUND}
lookfor "crc"
CRC=${FOUND}
lookfor "get_env_param"
GET_ENV_PARAM=${FOUND}
lookfor "set_env_param"
SET_ENV_PARAM=${FOUND}
lookfor  "busybox"
BUSYBOX=${FOUND}
#lookfor "bcl"
#BCL=${FOUND}
#lookfor "driver-latest"
#DRIVERLATEST=${FOUND}
lookfor "dutserver"
DUTSERVER=${FOUND}
lookfor "e2fsprogs"
E2FS=${FOUND}
lookfor "gpio_driver"
GPIO=${FOUND}
lookfor "hostapd"
HOSTAPD=${FOUND}
lookfor "Intel-WPS"
INTELWPS=${FOUND}
lookfor "iperf"
IPERF=${FOUND}
lookfor "iproute2"
IPROUTE=${FOUND}
lookfor "jffs2-etc"
JFFS2ETC=${FOUND}
lookfor "openssl"
OPENSSL=${FOUND}
lookfor "pciutils"
PCIUTILS=${FOUND}
#lookfor "tcpdump"
#TCPDUMP=${FOUND}
lookfor "tinytcl"
TINYTCL=${FOUND}
lookfor "wpa_supplicant"
WPASUP=${FOUND}
lookfor "mtlk_vb_upnpd"
UPNPD=${FOUND}
#Add XML Parser for HNAP - Ricky Cao
lookfor "expat"
EXPAT=${FOUND}
lookfor "hnap_wps_status_monitor"
HNAPWPSSTATUSMONITOR=${FOUND}
lookfor "force_ethpc_renew_ip"
FORCEETHPCRENEWIP=${FOUND}
#Jacky.Yang 9-Feb-2009, ipc multi-lang
#lookfor "Multi-Lang"
#MULTILANG=${FOUND}

if [ ! -d ${TOPDIR}/l ]
then
	mkdir ${TOPDIR}/l
	tst $? "Can't create directory ${TOPDIR}/l"
fi


link ${CRC} l/crc 
link ${SIMPDIFF} l/simpdiff
link ${GET_ENV_PARAM} l/get_env_param
link ${SET_ENV_PARAM} l/set_env_param
link ${BUSYBOX} l/busybox 
#link ${BCL} l/bcl 
#link ${DRIVERLATEST} l/driver 
link ${DUTSERVER} l/dutserver 
link ${E2FS} l/e2fs 
link ${GPIO} l/gpio 
link ${HOSTAPD} l/hostapd 
link ${INTELWPS} l/intelwps 
link ${IPERF} l/iperf 
link ${IPROUTE} l/iproute 
link ${JFFS2ETC} l/jffs2-etc 
link ${OPENSSL} l/openssl 
link ${PCIUTILS} l/pciutils 
#link ${TCPDUMP} l/tcpdump 
link ${TINYTCL} l/tinytcl 
link ${WEBSERVER} l/webserver 
link ${WPASUP} l/wpasup 
link ${UPNPD} l/mtlk_vb_upnpd
#Add XML Parser for HNAP - Ricky Cao
link ${EXPAT} l/expat
link ${HNAPWPSSTATUSMONITOR} l/hnap_wps_status_monitor
link ${FORCEETHPCRENEWIP} l/force_ethpc_renew_ip
#Jacky.Yang 9-Feb-2009, ipc for multi-lang
#link ${MULTILANG} l/Multi-Lang

exit 0

