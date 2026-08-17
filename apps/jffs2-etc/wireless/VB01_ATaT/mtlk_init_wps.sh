#!/bin/sh
# This is the script that configures the security mode.

#SVN id: $Id: mtlk_init_wps.sh 2435 2008-03-24 16:43:56Z ediv $

##TODO LIST:
#1. UUID is used for dual band of PBC (should be different otherwise)
#4. NW_KEY is taken w/o checks on the security status
#6. How to dual band (?)
#7. support mac_addr in accordance with mtlk proc(AP should be bridge)
#8. Device mode: rm upnp registrar. 
#9. Set interface mode dynamically

##***** Constant definitions *****/
#$DEVICE_MODE
Enrollee=0
#Upnp_registrar=1
Wirless_registrar=1
Unconfigured_AP=0
Ap_Proxy_registrar=1
#$DEVICE_TYPE
Infrastructure_station=0
Adhoc_station=1
AP=2
Test_MAC=3
#security
Security_Open=1
Security_Wep=2

##***** Parameters *****/
WSC_CONF_FILE="/tmp/wsc_config.txt"
WLAN_CONF="/tmp/wlan0.conf"
MAC_FILE="/proc/sys/dev/mtlk/wlan0/MAC"
SYS_CONF="/tmp/sys.conf"

# This is how to read the a parameter without caring about whether there are spaces after the "=":
DEVICE_TYPE=`awk -F "=" '/^network_type/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' ${WLAN_CONF}`
#WPS parameters	
#set a temporary PIN when absent
DEVICE_PIN=`awk -F "=" '/^NonProc_WPS_DevicePIN/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' ${WLAN_CONF}`
DEVICE_PIN_WC=`echo $DEVICE_PIN | wc -c`
	if expr $DEVICE_PIN_WC = 1 
	    then 
	    DEVICE_PIN=0
	fi	
STA_MODE=`awk -F "=" '/^NonProc_WPS_StationStatus/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' ${WLAN_CONF}`
STA_MODE_WC=`echo $STA_MODE | wc -c`
	if expr $STA_MODE_WC = 1 
	    then 
	    STA_MODE=$Enrollee
	fi
AP_MODE=`awk -F "=" '/^NonProc_WPS_ApStatus/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' ${WLAN_CONF}`
AP_MODE_WC=`echo $AP_MODE | wc -c`
	if expr $AP_MODE_WC = 1 
	    then 
	    AP_MODE=$Unconfigured_AP
	fi


MAC_WLAN=`(set \`cat $MAC_FILE \`; echo $1)`
SECURITY_MODE=`awk -F "=" '/^NonProcSecurityMode/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' ${WLAN_CONF}`

ESSID=`awk -F "=" '/^NonProc_ESSID/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' ${WLAN_CONF}`
ESSID_WC=`echo $ESSID | wc -c`
	if expr $ESSID_WC = 1 
	    then 
	    ESSID="WPSMetalink"
	fi

#todo-- change when it is not wpa
NW_KEY=`awk -F "=" '/^NonProc_WPA_Personal_PSK/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' ${WLAN_CONF}`
NW_KEY_WC=`echo $NW_KEY | wc -c`

BRIDGE_MODE=`awk -F "=" '/^BridgeMode/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' ${SYS_CONF}`

##***** helper functions *****/

##write the file
write_wps_config_file()
{
rm -rf ${WSC_CONF_FILE}
echo "# Simple Config Configuration File 
# This lines are commented out with #
# Each line should not exceed 80 characters 
# Format: TYPE=value 
# 
# Configured Mode: 1=Unconfigured AP, 2=Station enrollee, 3=Registrar,
# 4=AP with Proxy, 5 = AP with Proxy and Registrar
        CONFIGURED_MODE=$CONFIGURED_MODE

# Is the standalone Registrar (mode 3) wireless-enabled
# Yes: 1, No:0
	REGISTRAR_WIRELESS=$REGISTRAR_WIRELESS
# Should UPnP be used (for modes 1 and 3)
# Yes: 1, No:0
	USE_UPNP=$USE_UPNP
	UUID=00000000-0000-0000-0000-000000000000
	VERSION=0x10
	DEVICE_NAME=$DEVICE_NAME
# Primary Device Categories: Please refer to the SC spec for
# values for the following types
	PRI_DEV_CATEGORY=$PRI_DEV_CATEGORY
	PRI_DEV_OUI=0x0050F204
	PRI_DEV_SUB_CATEGORY=1
# MAC Address of the local device, 6 byte value
	MAC_ADDRESS=$MAC_WLAN
	MANUFACTURER=Metalink
	MODEL_NAME=Sample String 1
	MODEL_NUMBER=1234
	SERIAL_NUMBER=9876
# Config Methods: bitwise OR of values 
	CONFIG_METHODS=$CONFIG_METHODS
# Auth type flags: bitwise OR of values 
	AUTH_TYPE_FLAGS=$AUTH_TYPE_FLAGS
# Encr type flags: bitwise OR of values 
	ENCR_TYPE_FLAGS=$ENCR_TYPE_FLAGS
	CONN_TYPE_FLAGS=$CONN_TYPE_FLAGS
	RF_BAND=1
	OS_VER=0x80000000
	FEATURE_ID=0x80000000
# SSID:
	SSID=$ESSID
# Key Mgmt for Supplicant (Station enrollee, Registrar):
# Unconfigured, doing WSC: WPA-EAP IEEE8021X
# Configured after WSC (will be done by the s/w): WPA-PSK
# Key Mgmt for Hostapd (AP, AP with Registrar):
# Unconfigured, doing WSC: WPA-EAP
# Configured after WSC (will be done by the s/w): WPA-PSK
# Configured, plus Registrar: WPA-EAP WPA-PSK
	KEY_MGMT=$KEY_MGMT
# Are we using a USB key to transfer PIN/Credential?
# Yes: 1, No:0
	USB_KEY=0
#interface name
	INTERFACE_NAME=wlan0
# Is the Network Key set?
# Yes: 0xValue or passphrase, No: comment out line
# NW_KEY=0x000102030405060708090A0B0C0D0E0F000102030405060708090A0B0C0D0E0F
	#NW_KEY=0xC37844A77F463D59AF2D2E7FEC6D014E8FE5E510674F2AE56447DD443A147666
" > ${WSC_CONF_FILE}
if expr $DEVICE_PIN != 0
then
	echo "	DEVICE_PIN=$DEVICE_PIN" >> ${WSC_CONF_FILE}
fi
if expr $NW_KEY_WC != 1 
then
	echo "	NW_KEY=$NW_KEY" >> ${WSC_CONF_FILE}
fi

if expr $SUPPLICANT_PARAMS_WC != 1 
then
	echo "	SUPPLICANT_PARAMS=$SUPPLICANT_PARAMS" >> ${WSC_CONF_FILE}
fi

}


##***** initialization *****/
# STA

if expr $DEVICE_TYPE = $Adhoc_station || expr $DEVICE_TYPE = $Infrastructure_station
then
	USE_UPNP=0 
	#default (enrollee)
	#if expr $STA_MODE = $Enrollee
	#then
		CONFIGURED_MODE=2
		REGISTRAR_WIRELESS=0
	        DEVICE_NAME="Enrollee"
		KEY_MGMT=IEEE8021X
	#fi
	if expr $STA_MODE = $Wirless_registrar
	then
		#station wireless registrar
		CONFIGURED_MODE=3
		REGISTRAR_WIRELESS=1
		DEVICE_NAME="Wireless registrar"
		KEY_MGMT=IEEE8021X
	fi
#Removed (we're not upnp registrar)
#	if expr $STA_MODE = $Upnp_registrar
#	then
#		CONFIGURED_MODE=3
#		REGISTRAR_WIRELESS=0
#		DEVICE_NAME="UPNP registrar"
#	fi
	CONFIG_METHODS="0x0086"
	AUTH_TYPE_FLAGS="0x0002"
	ENCR_TYPE_FLAGS="0x0004"
	CONN_TYPE_FLAGS="0x01"
	PRI_DEV_CATEGORY=1
	SUPPLICANT_PARAMS=" "
	
	# if the bridge mode is mac_clon
	if expr $BRIDGE_MODE = 3
	then
		macl=`cat /tmp/mac_cloning.addr`
		SUPPLICANT_PARAMS="-p maclone=$macl "
	fi

fi

# AP
if expr $DEVICE_TYPE = $AP
then
	#default
	#if expr $AP_MODE = $Unconfigured_AP
	#then
		CONFIGURED_MODE=1
		AUTH_TYPE_FLAGS="0x0002"
		ENCR_TYPE_FLAGS="0x0004"
		CONN_TYPE_FLAGS="0x01"
		KEY_MGMT="WPA-EAP"
		DEVICE_NAME="UnConfigured AP"
	#fi
	 if expr $AP_MODE = $Ap_Proxy_registrar
	then
		CONFIGURED_MODE=5
		AUTH_TYPE_FLAGS="0x0002"
		#support TKIP+ CCMP
		ENCR_TYPE_FLAGS="0x0004"
		CONN_TYPE_FLAGS="0x01"
		KEY_MGMT="WPA-PSK"
		DEVICE_NAME="AP_PROXY_REGISTRAR"
		#open networks (+wep)
		if expr $SECURITY_MODE == $Security_Open ||expr $SECURITY_MODE == $Security_Wep
		then
			KEY_MGMT="NONE"
			ENCR_TYPE_FLAGS="0x0001"
			AUTH_TYPE_FLAGS="0x0001"
		fi
	fi
	
        CONFIG_METHODS="0x0086"
	REGISTRAR_WIRELESS=1
	NW_KEY=$NW_KEY
	PRI_DEV_CATEGORY=6
	USE_UPNP=0
fi
SUPPLICANT_PARAMS_WC=`echo $SUPPLICANT_PARAMS | wc -c`
write_wps_config_file 
dos2unix $WSC_CONF_FILE
chmod a+x $WSC_CONF_FILE 
