#!/bin/tclsh

source mtlk_cgicommon.tcl
#puts "Initializing security from wlan"


set Set_security_only 0
# WPA-Supplicant
global wpa_supplicant_conf_file  
global hostapd_conf_file 	     
global wpa_supplicant            
global hostapd             	     
global wpa_passphrase            
global certificate_file          
global sys_config_file				
global config_file					
global wlan_index

#print_debug_info

# WPA-Supplicant
set wlan_index 0

if {[info exists ::argv]} {
	set wlan_index [lindex $argv 0]
	if {$wlan_index!=0 && $wlan_index!=1} {
		set wlan_index 0
	}
}

set wpa_supplicant_conf_file     "$g_web_wpa_config_folder/wpa_supplicant.conf"
set hostapd_conf_file 	         "$g_web_wpa_config_folder/hostapd.conf"
set wpa_supplicant               "$g_web_wpa_app_folder/wpa_supplicant"
set hostapd             	     "$g_web_wpa_app_folder/hostapd"
set wpa_passphrase               "$g_web_wpa_app_folder/wpa_passphrase"
set certificate_file             "$g_web_saved_configs_folder/certificate.pem"
set sys_config_file				 "$g_web_config_folder/sys.conf"
set config_file					 "$g_web_config_folder/wlan${wlan_index}.conf"

#puts $sys_config_file
#puts $config_file

proc debug {data} {
	#puts "$data <br>\n"
}

proc get_proc_root {} {
	global wlan_index
	return "${g_web_proc_root_folder}${wlan_index}"
}

proc get_parameter {param_name} {
  global config_file
  global sys_config_file
  set params [file_load $config_file]
  foreach param $params {
	if {[regexp {^([^ =]+)[ ]*=[ ]?(.+)$} $param dummy pname pval]>0} {
		if {[string compare $param_name $pname]==0} {
			debug "(wlan0.conf) '$param_name=$pval'"
			return $pval
		}
	}
  }
  
  set params [file_load $sys_config_file]
  foreach param $params {
	if {[regexp {^([^ =]+)[ ]*=[ ]?(.+)$} $param dummy pname pval]>0} {
		if {[string compare $param_name $pname]==0} {
			debug "(sys.conf) '$param_name=$pval'"
			return $pval
		}
	}
  }
  
  #puts "'$param_name='"
  return ""
}

set wpa_supplicant_conf_file     "$g_web_wpa_config_folder/wpa_supplicant${wlan_index}.conf"
set hostapd_conf_file 	         "$g_web_wpa_config_folder/hostapd${wlan_index}.conf"

if {[info exists ::argv] && [lindex $::argv 0]=="Set_security_only"} {
	set Set_security_only 1
}
source update_security.tcl
if {$Set_security_only == 1 || ([info exists ::argv] && [lindex $::argv 0]=="reactivate")} {
	save_security noSave
} else {
	save_security
}
#Add for clear configuration of wpa_supplicant when unconfigured=1 - Ricky Cao on Dec. 24 2008
set unconfigured [get_parameter "unconfigured"]
if {$unconfigured == 1} {
	catch {exec echo unconfigured=1, so clear configuration of wpa_supplicant > /dev/console}
	set wpa_fp [open $wpa_supplicant_conf_file w]
	close $wpa_fp
}
if {[file exists $wpa_supplicant_conf_file]} {
	catch {exec cat $wpa_supplicant_conf_file}
}
#Ricky Cao on Dec. 24 2008

# Save the settings to flash - needed on platforms without jffs2 fs
if {$Set_security_only == 0} {
	# Apply runtime ChannelBonding change immediately when saving WebUI Basic Wireless Settings
	set cb_proc "/proc/sys/dev/mtlk/wlan${wlan_index}/ChannelBonding"
	set cfg_cb [string trim [get_parameter "ChannelBonding"]]
	if {($cfg_cb == "0" || $cfg_cb == "1") && [file exists $cb_proc]} {
		set cur_cb ""
		catch {set cur_cb [string trim [exec cat $cb_proc]]}
		if {$cur_cb != "" && $cur_cb != $cfg_cb} {
			catch {exec ifconfig "wlan${wlan_index}" down}
			catch {
				set cbfp [open $cb_proc w]
				puts $cbfp $cfg_cb
				close $cbfp
			}
			catch {exec ifconfig "wlan${wlan_index}" up}
		}
	}

	if {[catch {set ret [exec which config_umount.sh]}] == 0 && $ret != "" } {
		exec config_umount.sh
		exec config_mount.sh
	}

	if {[info exists ::argv] && [lindex $::argv 0]=="reactivate"} {
		set dev_type [get_parameter network_type]
		puts "Connecting"
		if {$dev_type == 2} {
			set ap_ssid [get_parameter NonProc_ESSID]
			set sec_mode [get_parameter NonProcSecurityMode]
			if {$ap_ssid != ""} {
				set ::env(MTLK_SSID) $ap_ssid
				set ::env(MTLK_IDX) $wlan_index
				catch {exec sh -c {iwconfig "wlan$MTLK_IDX" essid -- "$MTLK_SSID" >/dev/null 2>&1}}
			}
			catch {exec killall hostapd}
			sleep 2
			if {$sec_mode > 1 && [file exists $hostapd] && [file exists $hostapd_conf_file]} {
				catch {exec $hostapd -d $hostapd_conf_file > /dev/null 2>/dev/null &}
			}
		} else {
			Reactivate_Hostapd_Supplicant STA
		}
	}
}
#puts "Done!"