#!/bin/tclsh

set config_umount_command "config_umount.sh"
set config_mount_command "config_mount.sh"
set config_folder "/mnt/jffs2"
set wlan0_config_file "${config_folder}/wlan0.conf"

proc save_flash {} {
	global config_umount_command
	global config_mount_command
	set config_command ""
	set config_command [exec which $config_umount_command]
	if {$config_command == ""} {
		puts "The HW is a Dongle"
		return
	}
	exec $config_command
	set config_command [exec which $config_mount_command]
	exec $config_command
}

puts "make_device_pin:Starting";

# Get current Passphrase
set existing ""
set curr ""
catch {set existing [exec cat ${wlan0_config_file} | grep "NonProc_WPS_DevicePIN "]} err]
catch {set dummy [regexp {.* = (.*)} [exec grep "NonProc_WPS_DevicePIN " ${wlan0_config_file}] dummy curr]}

# If it's not empty, abort
if {$existing == "" || $curr == ""} {

	set env_dev_pin ""
	catch {set env_dev_pin [exec get_env_param device_pin_code] err}
	if {$env_dev_pin == ""} {
		puts "make_device_pin:Could not retrieve DevicePIN from ENV, setting default 12345670."
		set env_dev_pin "12345670"
	}

	# set the passphrase config line
	set str "NonProc_WPS_DevicePIN = $env_dev_pin"
	puts "make_device_pin:$str"			
	# add the passphrase config line to the config file
	set wlan_cfg_lines [split [exec cat ${wlan0_config_file} | grep -v "NonProc_WPS_DevicePIN"] "\n"]
	set new_wlan_file [open ${wlan0_config_file} "w" ]						
	foreach line $wlan_cfg_lines {
		puts $new_wlan_file $line
	}
	puts $new_wlan_file $str
	close $new_wlan_file
	puts "make_device_pin:Saving..."
	save_flash
	puts "make_device_pin:Done"
} else {
	puts "End"
}





