#!/bin/tclsh

# Source the web_config file that contains platform-dependent paths
if { [file exists /mnt/jffs2/web/web_config.tcl] } { 
	source /mnt/jffs2/web/web_config.tcl
} elseif { [file exists /root/mtlk/web/web_config.tcl] } { 
	source /root/mtlk/web/web_config.tcl
} elseif { [file exists  ../web/web_config.tcl] } {    
        source ../web/web_config.tcl 
} else {
	# TODO: Error handling!
	puts "Missing web configuration file! This is needed for setting paths."
	exit 1
}


puts "Restoring defaults"

exec /root/mtlk/bcl/bcl_util.tcl loadconfig ${g_web_saved_configs_folder}/restore_defaults.conf 0

if {[catch {set ret [exec which config_umount.sh]}] == 0 && $ret != "" } {
	exec config_umount.sh
	exec config_mount.sh
}

puts "Done!"
