#!/bin/sh
# This is the script that brings up the wireless interface.

#SVN id: $Id: mtlk_init_wls.sh 2278 2008-02-21 15:40:01Z arnonm $

# Defines
if [ ! $BRIDGE_MODE ]
then
	BRIDGE_MODE=`awk -F "=" '/^BridgeMode/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' $CONFIGS_PATH/sys.conf`
fi

if [ ! $NETWORK_TYPE ]
then
	NETWORK_TYPE=`awk -F "=" '/^network_type/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' $CONFIGS_PATH/wlan0.conf`
fi

# Usage Func

# Set driver DB handler  file	
#echo START PREPARE DRIVER HANDLER
#tclsh driver_api.tcl setDriverApiHandler driver_api.ini
#echo END PREPARE DRIVER HANDLER

#  1 )  config MAC clonning bridging mode
config_MAC_clonning_bridging ()
{
	if [ ! $BRIDGE_MODE == 3 ] || [ -e /tmp/mac_cloning.addr ]
	then
		return
	fi
	
	# only if we are in bridge mode and mac clonning
	MacCloningAddr=`awk -F "=" '/^MacCloningAddr/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`
	MAC_WLAN=`echo $MacCloningAddr | tr "[:lower:]" "[:upper:]"`
	if [ $MAC_WLAN == 0 ] || [ $MAC_WLAN == "00:00:00:00:00:00" ]
	then
		# There is no fixed address, so sniff the LAN ports to find the first MAC
		# Get the Mac address of br0
		DST_MAC=`ifconfig br0 | awk 'NR<2 {print $5}'`
		# Get the Mac address of the connected device (the future cloned mac address
		if [ -e /bin/etherdump ]
		then
			# etherdump sniff for the first packet
			MAC_WLAN=`etherdump -i br0 -h | awk -f /root/mtlk/etc/etherdump_awk -v var=$DST_MAC`
		else
			# tcpdump sniff for the first packet
			MAC_WLAN=`tcpdump -i br0 -ec1 ether dst $DST_MAC or broadcast | awk '{print $2}'`
			# change the mac address to upper case
			MAC_WLAN=`echo $MAC_WLAN | tr "[:lower:]" "[:upper:]"`
		fi
		# write the mac cloning address to a file for future use (such as WEB)
		echo $MAC_WLAN > /tmp/mac_cloning.addr
		# Remove colons from the mac address and make sure that each byte consists of 2 chars (this isn't necessarily what tcpdump returns...)
		MAC_WLAN=`echo $MAC_WLAN | awk -F ":" '{for (i=1; i<=6; i++) {str=$i; if (length(str) < 2) str = 0str; printf "%s" str}}'`
	fi
	echo END CONFIG BRIDGING
}

# 2) create links in tmp directory
create_links_in_tmp ()
{
	ln -s $IMAGES_PATH/bootloader.bin  /tmp/bootloader.bin
    ln -s $IMAGES_PATH/ap_upper.bin    /tmp/ap_upper.bin 
    ln -s $IMAGES_PATH/sta_upper.bin   /tmp/sta_upper.bin
    ln -s $IMAGES_PATH/contr_lm.bin    /tmp/contr_lm.bin 
    ln -s $DRIVER_PATH/mtlk.ko   /tmp/mtlk.ko
    ln -s $WEB_PATH/web_config.tcl   /tmp/web_config.tcl
	ln -s $CONFIGS_PATH/wpa_supplicant0.conf   /tmp/config.conf
	cd $IMAGES_PATH
	for progmodel in `ls ProgModel*`;
	do
		ln -s  $IMAGES_PATH/$progmodel  /tmp/$progmodel;
	done
	cd -
	echo END CREATE LINKS IN TMP
}

# 3) make_passphrase if field is empty (restore default) in AP only.
make_passphrase ()
{
	# Check that this is an AP
	if [ $NETWORK_TYPE == 2 ]  
	then
		(. ./mtlk_init_passphrase.sh)
		echo END MAKE PASSPHRASE
	fi
}

#  4) make the DevicePIN for WPS if field is empty
make_device_pin ()
{
	(. ./mtlk_init_pincode.sh)
	echo END MAKE DEVICE PIN
}


# 5) insmod ap/sta ,and verify
insmod_driver ()
{
	cd /tmp
	if [ $NETWORK_TYPE = 2 ]
	then
		insmod mtlk.ko ap=1
	else
		insmod mtlk.ko
	fi

	# verify that insmod was successful
	INSMOD_SUCCEEDED=`ifconfig -a | grep wlan | wc -l`		
	if [ $INSMOD_SUCCEEDED = 0 ]
	then
		count=0
		INSMOD_SUCCEEDED=`ifconfig -a | grep wlan | wc -l`
		while [ $INSMOD_SUCCEEDED = 0 ]
		do
			count=`expr $count + 1`
			if [ $count != 10 ]
			then
				sleep 1
				INSMOD_SUCCEEDED=`ifconfig -a | grep wlan | wc -l`
			else
				echo "insmod of the driver failed - no reason to continue with wls init" > /dev/console
				echo Wireless driver insmod failed. > /tmp/init_failed
				break
			fi
		done
		echo count=$count
	fi
	
	cd -
	echo END INSMOD DRIVER
}

# 6) QOS
qos ()
{
	# CONFIGURE_TC defined into the mtlk_init_platform.sh
	if [ $CONFIGURE_TC != 1 ]
	then
		echo TC unneeded for this platform
		return
	fi
	
	QMap=`awk -F "=" '/^Use11QMap/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/wlan0.conf`
	if [ $QMap == 1 ]
	then
		(. ./qos_q.sh)
	else
		(. ./qos_d.sh)
	fi
	echo END QOS
}

# 7) writing driver params
driver_params ()
{	
	#Read configuration file parameters	
	if [ ! -e /tmp/wlan0.conf ]
	then
		echo Can not read file wlan0.conf
		return
	fi
	#Update parameters according to DB. 
	tclsh driver_api.tcl DriverSetAll wlan0 /tmp/wlan0.conf
	if [ $? != 0 ]
	then
		echo ERROR: WRITING DRIVER PARAMS FAILED >> /tmp/init_failed
		return
	fi
	
	if [ -e /tmp/set_driver_params.sh ]
	then
		chmod +x /tmp/set_driver_params.sh
		/tmp/set_driver_params.sh
	else
		echo ERROR: "set_driver_params.sh is absent" >> /tmp/init_failed
	fi
	
	echo END DRIVER PARAMS
}

# 8) 802.11D
hw_limits ()
{
	grep -v "^count" rdlim.ini > /tmp/rdlim.ini
	mv /tmp/rdlim.ini rdlim.ini 
	
	tclsh rdlim.tcl
	echo END HW LINITS
}

# 9) in case of mac clonning, write mac address to driver
# Before ifconfig, if we are using a cloned MAC write the new one now.
# When the EEPROM is implemented, only overwrite MAC when using MAC cloning.
write_mac_address ()
{ 
if  [ $BRIDGE_MODE == 3 ]
then
	# Write to the driver
	tclsh driver_api.tcl DriverParamSet wlan0 MAC $MAC_WLAN
	if [ $? != 0 ]
	then
		echo "ERROR: Config MAC in MAC Cloning mode is failed"  >> /tmp/init_failed
		return
	fi
fi

if [ $BRIDGE_MODE == 2 ]
then
	DST_MAC=`ifconfig br0 | awk 'NR<2 {print $5}'`
	tclsh driver_api.tcl DriverParamSet wlan0 L2NAT_LocMAC $DST_MAC
	if [ $? != 0 ]
	then
		echo ERROR: Config L2NAT_LocMAC is failed  >> /tmp/init_failed
		return
	fi
fi

echo END WRITE MAC ADDRESS
}
# 10) setCountry
setCountry ()
{
	EEPROMCountryValid=`awk -F "=" '/^EEPROMCountryValid/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/wlan0.conf`
	if [ ! $EEPROMCountryValid ]
	then
		(. ./mtlk_init_country.sh $CONFIGS_PATH 0)
		echo END SET COUNTRY
	fi
}

# 11) ifconfig
ifconfig_up ()
{
	#if is an AP, hostapd will up wlan0 interface
	if [ $NETWORK_TYPE = 0 ]
	then
		if [ $IP_WLAN ] && [ $SUBNET_WLAN ]
		then
			ifconfig wlan0 $IP_WLAN netmask $SUBNET_WLAN > /dev/null
		else
			ifconfig wlan0 up
		fi
		echo END IFCONFIG UP
	fi
}


###############################################################
# don't change the order of these functions
# if needed , you can comment out some of these function calls

config_MAC_clonning_bridging

create_links_in_tmp

make_passphrase

make_device_pin

insmod_driver

if [ ! -e /tmp/init_failed ]; then	qos; fi
	
if [ ! -e /tmp/init_failed ]; then driver_params; fi
	
if [ ! -e /tmp/init_failed ]; then 	hw_limits; fi
	
if [ ! -e /tmp/init_failed ]; then 	write_mac_address; fi
	
if [ ! -e /tmp/init_failed ]; then 	setCountry; fi
	
if [ ! -e /tmp/init_failed ]; then 	ifconfig_up; fi
#################################################################
