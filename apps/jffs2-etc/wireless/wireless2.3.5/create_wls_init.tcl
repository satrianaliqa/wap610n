#!/bin/tclsh

############################################
# File: create_wls_init.tcl
# Author: Avri G
# Date : 12.2007
# Description:
# This file should generate the wireless init scripts
#
# Inputs : wlanX.conf , driver_api.db , system_paths
#############################################


# read paths of the system
source ../web/web_config.tcl


# no need to load ini.tcl , it is loaded from driver.api
# source ini.tcl

# load driver api library
source driver_api.tcl

set need_fi_at_end 0
set driver_db driver_api.ini
set wlan_index 0
set wlan_card_count 1
set wlan_config "$g_web_config_folder/wlan0.conf"
set sys_config "$g_web_config_folder/sys.conf"
set driver mtlk.ko

# read the configuration file
set conf_handler [IniOpen $wlan_config]
set sys_handler  [IniOpen $sys_config]

#Set driver DB file	
setDriverApiHandler driver_api.ini
#################################################################
#				Functions
#################################################################

# Return an environment variable
proc get_env {name} {
	return [lindex [array get ::env $name] 1]
}

proc print_file {file_name} {
	if {[file readable $file_name] ==  1} {
		set fp [open $file_name r]
		set contents [read $fp]
		close $fp
		puts $contents
	} else {
		puts "echo Can not read file $file_name"
	}
}

proc setConfiguration {interface conf_file } {
	
	set wep 0
	set dbg [get_wlan_param DBG]
	#Read configuration file parameters	
	if {[file readable $conf_file] ==  1} {
		set fp [open $conf_file r]
		set contents [read $fp]
		close $fp
	} else {
		puts "echo Can not read file $conf_file"
		return ''
	}
	#Update parameters according to DB. 
	set contents [split $contents "\n"]
	foreach line $contents {
		if {[regexp {(.*)=(.*)} $line all param value] == 1} {
			set param [string trim $param]
			set value [string trim $value]
			if {$param == "WepEncryption"} {
				set wep $value
				continue
			}
			set ret [DriverSet $interface $param $value]
			if {$ret != ""} {
				if {$dbg == 1} {
					puts "echo \"$ret\""
				}
				puts $ret
			}
		}
	
	}
	# this is an ugly workaround, to make sure that we set WepEncryption after we done with the keys
	set ret [DriverSet $interface "WepEncryption" $wep]
	if {$ret != ""} {
		puts "echo \"$ret\""
		puts $ret
	}
}

proc get_wlan_param {param} {
	global conf_handler
	return [IniGetKey $conf_handler "" $param]
}

proc get_sys_param {param} {
	global sys_handler
	return [IniGetKey $sys_handler "" $param]
}



###################################################################################


# 1) get standard paramters from wlan.conf
set network_type [get_wlan_param network_type]
set bridge_mode [get_sys_param BridgeMode]
# A.G - no need for that. the external init define IP_WLAN and SUBNET_WLAN
# TODO: Check if needed or remove
set ip_wlan [get_sys_param ip_wlan]
set subnet_wlan [get_sys_param subnet_wlan]
puts {#!/bin/sh}
puts {# This is the script that brings up the wireless interface.}
puts "# The script was auto generated"
puts "# check if the ip_wlan is set. If not, set it"
puts {IP_WLAN_count=`echo $IP_WLAN | wc -c`}
puts "if expr \$IP_WLAN_count = 1 "
puts "then"
puts "IP_WLAN=$ip_wlan" 
puts "SUBNET_WLAN=$subnet_wlan" 
puts "export IP_WLAN" 
puts "export SUBNET_WLAN" 
puts "fi"

# 2) config bridging
proc config_bridging {} {
	global wlan_card_count
	# only if we are in bridge mode and mac clonning.
	if {$wlan_card_count == 1} {		
		if {$::bridge_mode > 0} { 
			puts {}
			
			if {$::bridge_mode == 3 && ! [file exists /tmp/mac_cloning.addr]} { 
				puts "# we are in bridge mode - mac cloning"
				set mac_wlan [string toupper [get_sys_param MacCloningAddr]]
				puts "MAC_WLAN=$mac_wlan" 
				if { $mac_wlan == 0 || $mac_wlan == "00:00:00:00:00:00"} { 
					puts {# There is no fixed address, so sniff the LAN ports to find the first MAC}
					puts {# Get the Mac address of br0}
					puts {DST_MAC=`ifconfig br0 | awk 'NR<2 {print $5}'`}

					puts {# Get the Mac address of the connected device (the future cloned mac address)}
					if {[file exists "/bin/etherdump"] } {
						puts {# etherdump sniff for the first packet}
						puts {MAC_WLAN=`etherdump -i br0 -h | awk -f /root/mtlk/etc/etherdump_awk -v var=$DST_MAC`}
					} else {
						puts {# tcpdump sniff for the first packet}
						puts {MAC_WLAN=`tcpdump -i br0 -ec1 ether dst $DST_MAC or broadcast | awk '{print $2}'`}
						puts {# change the mac address to upper case}
						puts {MAC_WLAN=`echo $MAC_WLAN | tr "[a-z]" "[A-Z]"`}
					}
				}
				puts {# write the mac cloning address to a file for future use}
				puts {echo $MAC_WLAN > /tmp/mac_cloning.addr}
				
			}
		}
	}
}

# 3) create links in tmp directory
proc create_links_in_tmp {} {
	global g_web_images_folder
	global g_web_driver_folder
	global g_web_tmp_folder
	global g_web_web_app_folder
	global g_web_wpa_config_folder
	global driver
	
	puts {}
	puts {# First make sure that there symbolic links to all the bins, driver and progmodels in temp directory}
	puts "ln -s $g_web_images_folder/bootloader.bin  $g_web_tmp_folder/bootloader.bin"
	puts "ln -s $g_web_images_folder/ap_upper.bin    $g_web_tmp_folder/ap_upper.bin" 
	puts "ln -s $g_web_images_folder/sta_upper.bin   $g_web_tmp_folder/sta_upper.bin"
	puts "ln -s $g_web_images_folder/contr_lm.bin    $g_web_tmp_folder/contr_lm.bin" 
	puts "ln -s $g_web_driver_folder/$driver   $g_web_tmp_folder/$driver"
	puts "ln -s $g_web_web_app_folder/web_config.tcl   $g_web_tmp_folder/web_config.tcl"
	# this link is for the wps, so it will use the same config file that the web is using
	puts "ln -s $g_web_wpa_config_folder/wpa_supplicant0.conf   $g_web_tmp_folder/config.conf"

	puts {# Link all the progmodel versions to temp directory}
	puts "cd $g_web_images_folder"
	puts {for progmodel in `ls ProgModel*`;}
	puts {do }
	puts "	ln -s  $g_web_images_folder/\$progmodel /$g_web_tmp_folder/\$progmodel; "
	puts "done"
	puts "cd -"
}

# 4) insmod ap/sta ,and verify
proc insmod {} {
	global g_web_tmp_folder
	global network_type
	global wlan_card_count
	global driver
	puts {}
	puts {# Start the driver - insmod }
	puts "cd $g_web_tmp_folder"
	if  {$network_type == 2} {
		if {$wlan_card_count == 2} {
			puts {# This is a dual band AP}
			puts "insmod $driver ap=1,1"
		} else {
			puts {# This is a single band AP}
			puts "insmod $driver ap=1"
		}
	} else {
		puts {# This is a regular station}
		puts "insmod $driver"
	}


	puts "# verify that insmod was successful"
	puts {INSMOD_SUCCEEDED=`ifconfig -a | grep wlan | wc -l`}
	puts {if expr $INSMOD_SUCCEEDED = 0}
	puts {then}
	puts {# insmod of the driver failed - no reason to continue with wls init}
	puts "	echo Wireless driver insmod failed. > $g_web_tmp_folder/init_failed"
	set ::need_fi_at_end 1
	puts "cd -"
	puts "else"
}

# 5) QOS
proc qos {} {
	set use_qos [get_env CONFIGURE_TC]
	if {$use_qos != 1} {
		return
	}
	set 11QMap ""
	set 11QMap [get_wlan_param Use11QMap]
	if {$11QMap == 1} {
		print_file qos_q.sh
	} else {
		print_file qos_d.sh
	}
	if {$::wlan_card_count == 2} {
		print_file qos2.sh
	}
}

# 6) writing driver params
proc driver_params {} {
	puts {}
	puts "\n\n# Writing driver parameters"
	setConfiguration "wlan0" "$::g_web_config_folder/wlan0.conf" 
	# this is a driver parameter but we hold it in the sys.conf
	puts [DriverSet wlan0 BridgeMode [get_sys_param BridgeMode]]

	if {$::wlan_card_count == 2} {
		setConfiguration "wlan1" "$::g_web_config_folder/wlan1.conf"
		# this is a driver parameter but we hold it in the sys.conf
		puts [DriverSet wlan1 BridgeMode [get_sys_param BridgeMode]]
	}
}

# make_passphrase if field is empty (restore default) in AP only.
proc make_passphrase {} {
	global network_type	
	if  {$network_type == 2} {
		puts "tclsh make_passphrase.tcl"
		puts "cp /mnt/jffs2/wlan0.conf /tmp/wlan0.conf"
		puts "dos2unix -u /tmp/wlan0.conf"
	}
}

# make the DevicePIN for WPS if field is empty
proc make_device_pin {} {
	puts "tclsh make_device_pin.tcl"
	puts "cp /mnt/jffs2/wlan0.conf /tmp/wlan0.conf"
	puts "dos2unix -u /tmp/wlan0.conf"
}

# 7) 802.11D
proc hw_limits {} {
	puts {}
	puts "cd -"
	puts "tclsh rdlim.tcl"
	puts "cd $::g_web_tmp_folder"
}


# 8) in case of mac clonning, write mac address to driver
# Before ifconfig, if we are using a cloned MAC write the new one now.
# When the EEPROM is implemented, only overwrite MAC when using MAC cloning.
proc write_mac_address {} { 
	puts {}

	if {$::wlan_card_count == 1} {
		if {$::bridge_mode == 3} {
			puts {# Remove colons from the mac address and make sure that each byte consists of 2 chars (this isn't necessarily what tcpdump returns...)}
			puts {MAC_WLAN=`echo $MAC_WLAN | awk -F ":" '{for (i=1; i<=6; i++) {str=$i; if (length(str) < 2) str = 0str; printf "%s" str}}'`}
			puts {# Write to the driver}
			puts [DriverSet wlan0 MAC {$MAC_WLAN}]
		} elseif {$::bridge_mode == 2} {
			puts {DST_MAC=`ifconfig br0 | awk 'NR<2 {print $5}'`}
			puts [DriverSet wlan0 L2NAT_LocMAC {$DST_MAC}]
		}
	} 
}


proc setCountry {} {
	puts {}
	puts {/root/mtlk/etc/mtlk_init_country.sh /mnt/jffs2 0}
}

# 9) ifconfig
proc ifconfig {} {
	#puts {sleep 3}
	puts {}
	# TODO: not in bridging mode... currently this is always created, and later removed in routing mode
	if {$::wlan_card_count == 2} {
		puts {# There are 2 bands - add them to a bridge} 
		puts {ifconfig wlan0 0.0.0.0}
		puts {ifconfig wlan1 0.0.0.0}
		puts {brctl addif br1 wlan0}
		puts {brctl addif br1 wlan1}
		puts {# bring the bridge up}
		puts {ifconfig br1 $IP_WLAN netmask $SUBNET_WLAN}
	} else {
		puts {# bring wireless interface up}
		puts {ifconfig wlan0 $IP_WLAN netmask $SUBNET_WLAN}
	}
	puts "cd -"
}

# 10) bridging/routing callback
proc post_ifconfig {} {

}
###############################################################
# don't change the order of these functions
# if needed , you can comment out some of these function calls


config_bridging
if {[file readable /tmp/reload] ==  0} {
create_links_in_tmp
}
make_device_pin
insmod
qos
driver_params
if {[get_wlan_param UpDebugLevel]} {
	puts {echo 1 > /proc/net/mtlk/debug}
}
hw_limits
write_mac_address
setCountry
ifconfig
# post_ifconfig
if {$need_fi_at_end == 1} {
 puts "fi" 
}
#################################################################
