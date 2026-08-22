#!/bin/sh
# This is the script that configures the security mode.

#SVN id: $Id: mtlk_init_wps.sh 3839 2009-06-09 08:29:57Z efratl $

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
Security_Wpa=3

##***** Parameters *****/
WSC_CONF_FILE="/tmp/wsc_config.txt"
WLAN_CONF="/tmp/wlan0.conf"
MAC_FILE="/proc/sys/dev/mtlk/wlan0/MAC"
SYS_CONF="/tmp/sys.conf"

# This is how to read the a parameter without caring about whether there are spaces after the "=":
if [ ! $NETWORK_TYPE ]
then
	NETWORK_TYPE=`awk -F "=" '/^network_type/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' ${WLAN_CONF}`
fi
#WPS parameters

ESSID=`cat ${WLAN_CONF} | grep NonProc_ESSID | sed 's#^[^=]*=[ ]*##'`
if [ ! "$ESSID" ]
then 
	ESSID="WPSMetalink"
fi
	
#set a temporary PIN when absent
DEVICE_PIN=`awk -F "=" '/^NonProc_WPS_DevicePIN/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' ${WLAN_CONF}`
if [ ! $DEVICE_PIN ]
then 
	DEVICE_PIN=0
fi
	
STA_MODE=`awk -F "=" '/^NonProc_WPS_StationStatus/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' ${WLAN_CONF}`
if [ ! $STA_MODE ]
then 
	STA_MODE=$Enrollee
fi

AP_MODE=`awk -F "=" '/^NonProc_WPS_ApStatus/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' ${WLAN_CONF}`
if [ ! $AP_MODE ]
then 
	AP_MODE=$Unconfigured_AP
fi

MAC_WLAN=`(set \`cat $MAC_FILE \`; echo $1)`


FREQUENCY_BAND=`awk -F "=" '/^FrequencyBand/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' ${WLAN_CONF}`

RF_BAND=2

if [ $NETWORK_TYPE = 2 ] && [ $FREQUENCY_BAND == 1 ] 
then
	RF_BAND=1
else
	RF_BAND=2
fi


if [ $NETWORK_TYPE = 2 ]
then
	ENCRYPTION_MODE=`awk -F "=" '/^NonProc_WPA_Personal_Mode/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' ${WLAN_CONF}`
	if [ $ENCRYPTION_MODE = "1" ]
	then 
		ENCRYPTION="0x0003"
	elif [ $ENCRYPTION_MODE = "2" ]
	then
		ENCRYPTION="0x0021"
	else
		ENCRYPTION="0x0023"
	fi
	 
	 
	ENCAPSULATION_MODE=`awk -F "=" '/^NonProc_WPA_Personal_Encapsulation/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' ${WLAN_CONF}`
	if [ $ENCAPSULATION_MODE = "0" ]
	then
		ENCAPSULATION="0x0004"
	elif [ $ENCAPSULATION_MODE = "1" ]
	then
		ENCAPSULATION="0x0008"
	else
		ENCAPSULATION="0x000c"
	fi


	SECURITY_MODE=`awk -F "=" '/^NonProcSecurityMode/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' ${WLAN_CONF}`
	if [ $SECURITY_MODE == $Security_Wpa ]
	then		
		NW_KEY=`cat ${WLAN_CONF} | grep NonProc_WPA_Personal_PSK | sed 's#^[^=]*=[ ]*##'`				
	elif [ $SECURITY_MODE == $Security_Wep ]
	then
		NW_KEY0=`awk -F "=" '/^WepKeys_DefaultKey0/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' ${WLAN_CONF}`
		NW_KEY1=`awk -F "=" '/^WepKeys_DefaultKey1/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' ${WLAN_CONF}`
		NW_KEY2=`awk -F "=" '/^WepKeys_DefaultKey2/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' ${WLAN_CONF}`
		NW_KEY3=`awk -F "=" '/^WepKeys_DefaultKey3/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' ${WLAN_CONF}`
		if [ $NW_KEY0 ] && [ ${#NW_KEY0} = 12 ] || [ ${#NW_KEY0} = 28 ]
		then
				NW_KEY0=`echo $NW_KEY0 | awk -F "0x" '{print $2}'`
		fi
		if [ $NW_KEY1 ] && [ ${#NW_KEY1} = 12 ] || [ ${#NW_KEY1} = 28 ]
		then
				NW_KEY1=`echo $NW_KEY1 | awk -F "0x" '{print $2}'`
		fi
		if [ $NW_KEY2 ] && [ ${#NW_KEY2} = 12 ] || [ ${#NW_KEY2} = 28 ]
		then
				NW_KEY2=`echo $NW_KEY2 | awk -F "0x" '{print $2}'`
		fi
		if [ $NW_KEY3 ] && [ ${#NW_KEY3} = 12 ] || [ ${#NW_KEY3} = 28 ]
		then
				NW_KEY3=`echo $NW_KEY2 | awk -F "0x" '{print $2}'`
		fi
	fi
fi


if [ ! $BRIDGE_MODE ]
then
	BRIDGE_MODE=`awk -F "=" '/^BridgeMode/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' ${SYS_CONF}`
fi

WPS_IGNORESELECTEDREGISTRAR=0
WPS_IGNORESELECTEDREGISTRAR=`awk -F "=" '/^WPS_IgnoreSelectedRegistrar/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' ${WLAN_CONF}`

BSSID_USAGE=1
BSSID_USAGE=`awk -F "=" '/^WPS_BSSID_USAGE/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' ${WLAN_CONF}`


# Vendor specific information
MANUFACTURER=`get_env_param manufacturer`
MODEL_NAME=`get_env_param model_name`
MODEL_NUMBER=`get_env_param model_number`
DEVICE_NAME=`get_env_param device_name`

# Serial number can be read from the ENV. uncomment next line to take serial number from ENV
#SERIAL_NUMBER=`get_env_param serial_no`

# Serial number can be set to a constant number, to prevent privacy issues
SERIAL_NUMBER="000"

# Use defined UUID, otherwise use a default
UUID_STORED=`grep "^UUID *= *" ${WSC_CONF_FILE}`
if [ ! $UUID_STORED ]; then UUID_STORED="UUID=00000000-0000-0000-0000-000000000000"; fi

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
	$UUID_STORED
	VERSION=0x10
	DEVICE_NAME=$DEVICE_NAME
# Primary Device Categories: Please refer to the SC spec for
# values for the following types
	PRI_DEV_CATEGORY=$PRI_DEV_CATEGORY
	PRI_DEV_OUI=0x0050F204
	PRI_DEV_SUB_CATEGORY=1
# MAC Address of the local device, 6 byte value
	MAC_ADDRESS=$MAC_WLAN
# Config Methods: bitwise OR of values 
	CONFIG_METHODS=$CONFIG_METHODS
# Auth type flags: bitwise OR of values 
	AUTH_TYPE_FLAGS=$AUTH_TYPE_FLAGS
# Encr type flags: bitwise OR of values 
	ENCR_TYPE_FLAGS=$ENCR_TYPE_FLAGS
	CONN_TYPE_FLAGS=$CONN_TYPE_FLAGS
	RF_BAND=$RF_BAND
	OS_VER=0x80000000
	FEATURE_ID=0x80000000
	
# Ignore SelectedRegistrar flag in Beacons and Probe Responces.
# This flag was made to workaround bug in some APs (like WRT610N)
# when they not update SelectedRegistrar flag to true when activated
# for WPS. With this flag set we will try each WPS AP around
# one by one ignoring this flag. As result overlap detection
# won't work and overall connection time may increase that may
# lead to Walk timeout depending of how many WPS APs are around in air.
# By default it is off (zero).
IGNORE_SELECTED_REGISTRAR=$WPS_IGNORESELECTEDREGISTRAR

# When creating supplicant config file it is possible to specify BSSID
# along with ESSID to which we want to connect. This parameter defines
# the policy of this behaviour: 0 - don't use BSSID, 1 - use BSSID (default),
# 2 - use BSSID and on second connection use the MAC Address attribute
# as BSSID from received credentials in M8 message.
BSSID_USAGE_IN_CONNECTION_POLICY=$BSSID_USAGE

	
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
	          1234567890123456789012345678901234567890123456789012345678901234


# vendor specific info for WPS packets
MANUFACTURER=$MANUFACTURER
MODEL_NAME=$MODEL_NAME
MODEL_NUMBER=$MODEL_NUMBER
SERIAL_NUMBER=$SERIAL_NUMBER
DEVICE_NAME=$DEVICE_NAME

" > ${WSC_CONF_FILE}


if [ $DEVICE_PIN != 0 ]
then
	echo "	DEVICE_PIN=$DEVICE_PIN" >> ${WSC_CONF_FILE}
fi
#set network key for wpa
if [ "$NW_KEY" ]
then
	echo "	NW_KEY=$NW_KEY" >> ${WSC_CONF_FILE}
fi
#set network keys for wep
if [ $NW_KEY0 ]
then
	echo "	NW_KEY=$NW_KEY0" >> ${WSC_CONF_FILE}
fi
if [ $NW_KEY1 ]
then
	echo "	NW_KEY=$NW_KEY1" >> ${WSC_CONF_FILE}
fi
if [ $NW_KEY2 ]
then
	echo "	NW_KEY=$NW_KEY2" >> ${WSC_CONF_FILE}
fi
if [ $NW_KEY3 ]
then
	echo "	NW_KEY=$NW_KEY3" >> ${WSC_CONF_FILE}
fi

if [ $SUPPLICANT_PARAMS_WC != 1 ]
then
	echo "	SUPPLICANT_PARAMS=$SUPPLICANT_PARAMS" >> ${WSC_CONF_FILE}
fi

}



##***** initialization *****/
# STA

if [ $NETWORK_TYPE == $Adhoc_station ] || [ $NETWORK_TYPE == $Infrastructure_station ]
then
	USE_UPNP=0 
	#default (enrollee)
	#if expr $STA_MODE = $Enrollee
	#then
		CONFIGURED_MODE=2
		REGISTRAR_WIRELESS=0
	    #DEVICE_NAME="Enrollee"
		KEY_MGMT=IEEE8021X
	#fi
	if [ $STA_MODE == $Wirless_registrar ]
	then
		#station wireless registrar
		CONFIGURED_MODE=3
		REGISTRAR_WIRELESS=1
		#DEVICE_NAME="Wireless registrar"
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
	AUTH_TYPE_FLAGS="0x0023"
	ENCR_TYPE_FLAGS="0x000f"
	CONN_TYPE_FLAGS="0x01"
	PRI_DEV_CATEGORY=1
	SUPPLICANT_PARAMS="-bbr0 "
	
	# if the bridge mode is mac_clon
	if [ $BRIDGE_MODE == 3 ] && [ -e /tmp/mac_cloning.addr ]
	then
		macl=`cat /tmp/mac_cloning.addr`
		SUPPLICANT_PARAMS="-bbr0 -p maclone=$macl "
	fi

fi


# AP
if [ $NETWORK_TYPE == $AP ]
then
	#default
	#if expr $AP_MODE = $Unconfigured_AP
	#then
		CONFIGURED_MODE=1
		AUTH_TYPE_FLAGS="0x0022"
		ENCR_TYPE_FLAGS="0x000c"
		CONN_TYPE_FLAGS="0x01"
		KEY_MGMT="WPA-EAP"
		#DEVICE_NAME="UnConfigured AP"
	#fi
	if [ $AP_MODE == $Ap_Proxy_registrar ]
	then
		CONFIGURED_MODE=5
		AUTH_TYPE_FLAGS="$ENCRYPTION"
		#support TKIP+ CCMP
		ENCR_TYPE_FLAGS="$ENCAPSULATION"
		CONN_TYPE_FLAGS="0x01"
		KEY_MGMT="WPA-PSK"
		#DEVICE_NAME="AP_PROXY_REGISTRAR"
		#open networks
		if [ $SECURITY_MODE == $Security_Open ]
		then
			KEY_MGMT="NONE"
			ENCR_TYPE_FLAGS="0x0001"
			AUTH_TYPE_FLAGS="0x0001"
		fi
		#wep security
		if [ $SECURITY_MODE == $Security_Wep ]
		then
			KEY_MGMT="NONE"
			ENCR_TYPE_FLAGS="0x0002"
			AUTH_TYPE_FLAGS="0x0001"
			if [ $WepTxKeyIdx ] && [ $WepTxKeyIdx != 1 ]
			then
				echo "#Set WEP TX KEY ID if not a default (1)" >> ${WSC_CONF_FILE}
				echo "	WEP_TX_KEY_IDX=$WepTxKeyIdx" >> ${WSC_CONF_FILE}
			fi
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
