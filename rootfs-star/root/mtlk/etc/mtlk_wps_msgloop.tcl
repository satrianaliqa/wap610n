#!/bin/tclsh

#include platform specific tree structure
if { [file exists /mnt/jffs2/web/web_config.tcl] } {
        source /mnt/jffs2/web/web_config.tcl
} elseif { [file exists /root/mtlk/web/web_config.tcl] } {
        source /root/mtlk/web/web_config.tcl
} elseif { [file exists  ../web/web_config.tcl] } {
        source ../web/web_config.tcl
} elseif { [file exists  /tmp/web_config.tcl] } {
        source /tmp/web_config.tcl
} else {
        # TODO: Error handling!
        puts "Missing web configuration file! This is needed for setting paths."
        exit 1
}

if {[file exists /root/mtlk/etc/mtlk_wps_common.tcl]} {
	source /root/mtlk/etc/mtlk_wps_common.tcl
} elseif {[file exists /mnt/jffs2/etc/mtlk_wps_common.tcl]} {
	source /mnt/jffs2/etc/mtlk_wps_common.tcl
} else {
	puts "Missing mtlk_wps_common.tcl file"
	exit 1
}


#fifo and files names
set ::FIFO_PBC "$g_web_tmp_folder/WPS_PBC"
set ::FIFO_SSID_AP "$g_web_tmp_folder/WPS_SSID_AP"
set ::FIFO_PIN_ENROLLEE "$g_web_tmp_folder/WPS_PIN_ENROLLEE"
set ::FIFO_CMD "$g_web_tmp_folder/WPS_CMD"
set ::FIFO_STATUS "$g_web_tmp_folder/WPS_WSC_STATUS"
set ::FIFO_MSG "$g_web_tmp_folder/WPS_WSC_MSG"
set ::FIFO_USER_SELECT_AP "$g_web_tmp_folder/WPS_SELECT_AP"
set ::FIFO_MAC_AP "$g_web_tmp_folder/WPS_MAC_AP"
set ::NEIGHBORS_INFO_FILE	 "$g_web_tmp_folder/WPS_NEIGHBORS"
set ::SUPPLICANT_CONF "$g_web_tmp_folder/config.conf"
set ::HOSTAPD_CONF "$g_web_tmp_folder/hostapd.conf"
set ::WPS_MSG_LOOP_TCL "mtlk_wps_msgloop.tcl"


set ::cmd_length			2
set ::wps_msg_length		256
set ::WPSScriptsDbgOut 		"/dev/null"


# load a file and return it's contents
proc file_load {filename} {
	set contents ""
	if {![catch {set fp [open "$filename" r]}]} {
		set contents [split [read $fp]	"\n"]
		close $fp
	}
	return $contents	
}

# Load wps log target
proc init_wps_logging {} {
	# Search for debug activation
	if {[file exists /tmp/wps-dbg] == 1} {
		set ::WPSScriptsDbgOut [lindex [file_load /tmp/wps-dbg] 0]
		debug_puts "mtlk_wps_cmd.tcl:wps scripts log to '$::WPSScriptsDbgOut'"
		if {$::WPSScriptsDbgOut == ""} {
			set ::WPSScriptsDbgOut "/dev/console"
		}
	} 

}

proc get_current_uptime {} {
	regexp "^(.+)\..* .*" [exec cat /proc/uptime] dummy CURRENT_TIME
	return $CURRENT_TIME
}

#Main function
set action ""

if {[llength ::argv]==0} {
	set action "start"
} else {
	set action [lindex $::argv 0] 
}

# Star up flags: 
# start
# stop
# start_msg
# start_code

set eventCount 0
init_wps_logging

proc kill_msg_loops {} {
}

if {$action =="start" } {
	
	set ps [split [exec ps | grep "$::WPS_MSG_LOOP_TCL"] "\n"]
	set count [llength $ps]
	if {$count>2} {
		debug_puts "$::WPS_MSG_LOOP_TCL:can't start - already running"
		exit 1
	}
	debug_puts "$::WPS_MSG_LOOP_TCL:starting FIFO loops"
	catch { [exec $::g_web_wpa_app_folder/$::WPS_MSG_LOOP_TCL start_msg &]}
	catch { [exec $::g_web_wpa_app_folder/$::WPS_MSG_LOOP_TCL start_code &]}
	exit 0
}

if {$action =="stop" } {
	debug_puts "$::WPS_MSG_LOOP_TCL:stopping FIFO loops"
	set ps ""
	catch { set ps [split [exec ps | grep $::WPS_MSG_LOOP_TCL] "\n"] }
	foreach line $ps {
		set process_num [lindex $line 0]
		
		# TODO : Why OK ?
		set ok [lindex $line 6] 
		if {$ok!=""} {
			catch {exec kill -9 $process_num}
		}
	}
	exit 0
}

if {$action =="start_msg" } {
	debug_puts "$::WPS_MSG_LOOP_TCL:start msg loop.Waiting for messages."
	set msg_file ""
	while {0 == 0 } {
		if {[catch {set msg_file [open "${::FIFO_MSG}" "r+"]} err]} {
			puts $err
			sleep 1
		} else {
			set msg_length $::wps_msg_length

			set MSG [read $msg_file $msg_length]
			close $msg_file
			debug_puts "$::WPS_MSG_LOOP_TCL:msg($eventCount)=$MSG"
			catch {exec echo "WPS_LastMessage = $MSG" > $::g_web_tmp_folder/wps_last_msg}
			incr eventCount
		}
	}
	
	exit 0
}

if {$action =="start_code" } {
	debug_puts "$::WPS_MSG_LOOP_TCL:start code loop.Waiting for WPS code events"
	set results_file ""
	while {0 == 0 } {
		if {[catch {set results_file [open "${::FIFO_STATUS}" "r+"]} err]} {
			puts $err
			sleep 1
		} else {
			set results [expr [read $results_file ${::cmd_length}]]
			close $results_file
				debug_puts "$::WPS_MSG_LOOP_TCL:code($eventCount)=$results"
			incr eventCount
		}
	}
	
	exit 0
}

debug_puts "$::WPS_MSG_LOOP_TCL:unknown command $action"


