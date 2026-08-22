#!/bin/tclsh

source mtlk_cgicommon.tcl

#### Platform-specific file paths ####

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

set wpa_supplicant               "$g_web_wpa_app_folder/wpa_supplicant"
set hostapd             	     "$g_web_wpa_app_folder/hostapd"
set wpa_passphrase               "$g_web_wpa_app_folder/wpa_passphrase"
set certificate_file             "$g_web_saved_configs_folder/certificate.pem"
set sys_config_file				[get_env_var ASP_web_sys_conf]
set config_file					[get_env_var ASP_web_wlan_conf]
set wlan_index [get_env_var ASP_wlan_index]
#puts "wlan_index is $wlan_index<br>"
set wpa_supplicant_conf_file     "$g_web_wpa_config_folder/wpa_supplicant${wlan_index}.conf"
set hostapd_conf_file 	         "$g_web_wpa_config_folder/hostapd${wlan_index}.conf"


proc debug {data} {
	#puts "$data <br>\n"
}


proc get_parameter {param_name} {
  set ret  [get_env_var "ASP_$param_name"]
  debug "$param_name='$ret'"
  return $ret
}

source update_security.tcl


if {[info exists ::do_save_security]==0 || $::do_save_security==1 } {
	set noMount [is_option_in_commandline "noMount"]
	save_security $noMount

}

