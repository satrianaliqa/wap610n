#!/bin/tclsh


# Source the web_config file that contains platform-dependent paths
if { [file exists /mnt/jffs2/web/web_config.tcl] } { 
	source /mnt/jffs2/web/web_config.tcl
	source /mnt/jffs2/etc/ini.tcl
} elseif { [file exists /root/mtlk/web/web_config.tcl] } { 
	source /root/mtlk/web/web_config.tcl
	source /root/mtlk/etc/ini.tcl
} elseif { [file exists  ../web/web_config.tcl] } {    
        source ../web/web_config.tcl
		source ../etc/ini.tcl
} else {
	# TODO: Error handling!
	puts "Missing web configuration file! This is needed for setting paths."
}


proc file_load {filename} {
	set contents ""
	catch {set contents [exec cat $filename]}
	set contents [split $contents "\n"]
	return $contents	
}

proc read_param_db {dbFile} {

	set sys_tmp [open /tmp/sys_tmp.ini w]
	puts $sys_tmp {[sys]}
	
	set wlan_tmp [open /tmp/wlan_tmp.ini w]
	puts $wlan_tmp {[wlan]}
	
	set lastKey ""

	#puts " reading $dbFile"
	if {[file readable $dbFile] ==  1} {
		set contents [file_load $dbFile]
		foreach line $contents {
			if {[regexp {([^=]*)=(.*)} $line all key value] == 1 } {
				set key [string trim $key]
				set value [string trim $value]
				if {[string compare $key "Name"] == 0} { 
					set lastKey $value 
				} elseif {[string compare $key "ConfFile"] == 0} {
					if {$value == "wlan"} {
						puts $wlan_tmp "${lastKey}=${value}"
					} elseif {$value == "sys"} {
						puts $sys_tmp "${lastKey}=${value}"
					}
				}
			}
		}
		close $sys_tmp
		close $wlan_tmp
		exec cat /tmp/sys_tmp.ini /tmp/wlan_tmp.ini > /tmp/mt_param.ini
		return [IniOpen /tmp/mt_param.ini]
	} else {
		return 0
	}	
}

proc do_config_import {in_file} {
	global g_web_config_folder
	global g_web_web_app_folder
	set sys_conf "${g_web_config_folder}/sys.conf"
	set wlan_conf "${g_web_config_folder}/wlan0.conf"
	set db "${g_web_web_app_folder}/mt_params.db"
	
	set sys_t [open $sys_conf w]
	set wlan_t [open $wlan_conf w]
	set web_db [read_param_db $db]
	set import_lines [file_load $in_file]

	foreach line $import_lines {

		if {[regexp {([^=]*)=(.*)} $line all key value] == 1 } {
			if {[IniGetKey $web_db wlan $key] != ""} {
				puts $wlan_t "$all"
			} 
			if {[IniGetKey $web_db sys $key] != ""} {
				puts $sys_t "$all"
			}
		}
	}
	close $sys_t
	close $wlan_t

	
	set last_path [pwd]	
	cd ${g_web_web_app_folder}
	# noReactivateSecurity because there is a reboot after
	catch {[exec ./update_config_files.tcl noReactivateSecurity]}
	
	cd $last_path
	
}


if {[info exists ::argv]} {
	set in_config_file_name [lindex $::argv 0]
	do_config_import $in_config_file_name
		
} else {
	puts -nonewline ""
}
