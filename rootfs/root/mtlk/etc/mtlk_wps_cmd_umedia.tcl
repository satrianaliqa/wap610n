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


#fifo and files names
set ::FIFO_PBC "$::g_web_tmp_folder/WPS_PBC"
set ::FIFO_SSID_AP "$::g_web_tmp_folder/WPS_SSID_AP"
set ::FIFO_PIN_ENROLLEE "$::g_web_tmp_folder/WPS_PIN_ENROLLEE"
set ::FIFO_CMD "$::g_web_tmp_folder/WPS_CMD"
set ::FIFO_STATUS "$::g_web_tmp_folder/WPS_WSC_STATUS"
set ::FIFO_MSG "$::g_web_tmp_folder/WPS_WSC_MSG"
set ::FIFO_USER_SELECT_AP "$::g_web_tmp_folder/WPS_SELECT_AP"
set ::FIFO_MAC_AP "$::g_web_tmp_folder/WPS_MAC_AP"
set ::FIFO_DEVICE_PIN "$::g_web_tmp_folder/WPS_DEVICE_PIN"
set ::NEIGHBORS_INFO_FILE	 "$::g_web_tmp_folder/WPS_NEIGHBORS"
set ::FIFO_LED_EVENTS "$::g_web_tmp_folder/WPS_LED_EVENTS"
set ::SUPPLICANT_CONF "$::g_web_tmp_folder/config.conf"
set ::HOSTAPD_CONF "$::g_web_tmp_folder/hostapd.conf"
set ::WPS_ACTION_STATFILE "$::g_web_tmp_folder/wps_action"
set ::WPS_LAST_CODE_STATFILE "$::g_web_tmp_folder/wps_last_code"
set ::WPS_MSG_LOOP_TCL "mtlk_wps_msgloop.tcl"
set ::WPS_START_TIME_STATFILE "$::g_web_tmp_folder/wps_startup_time"
set ::WPS_CURRENT_STATUS_STATFILE "$::g_web_tmp_folder/wps_current_status"
set ::WPSScriptsDbgOut 		"/dev/null"

#Constant variables
set STA 				0
set AP 				2
set NOT_PBC 			0
set IS_PBC 			1
set STA_enrollee			0
set AP_enrollee			2
set AP_proxy			1

set CMD_QUIT 			0
set CMD_AP_GET_CONF 	1
set CMD_REG_CONF_AP	2
set CMD_STA_GET_CONF	3
set CMD_REG_CONF_STA	4

set ::ap_select			0
set ::securityOpen 		1
set ::securityWPAPersonal 	3

set ::cmd_length			2
set ::wps_msg_length		256

set ::WPS_Active_Blink      6
set ::WPS_Idle_Blink        0
#
#  configuration manipulation helper functions
#

# load a file and return it's contents
proc file_load {filename} {
	set contents ""
	if {![catch {set fp [open "$filename" r]}]} {
		set contents [split [read $fp]	"\n"]
		close $fp
	}
	return $contents	
}

# load a configuration file
proc config_load {config_file} {
	return [file_load $config_file]
}

# save configuration file
proc config_save {config_file_name config_file_contents} {
	set config_file_contents [join $config_file_contents "\n"]
	catch [exec echo $config_file_contents > $::g_web_tmp_folder/tmp_config.conf]
	catch [exec mv $::g_web_tmp_folder/tmp_config.conf $config_file_name]
}

# Get parameter value
proc get_param {param_name config_file_contents} {
	set result ""
	foreach line $config_file_contents { 
		if {[regexp "^$param_name" $line]==1} {
			catch {set cmd_res [regexp {.* = (.*)} $line dummy result]	}
		}
	}

	return $result
}

# set parameter value
proc set_param {config_file_contents param value} {
	set new_contents ""
	set found 0
	foreach line $config_file_contents {
		if {[regexp "^$param = " $line]} {
			set new_contents [lappend new_contents "$param = $value"]
			puts "$param = $value"
			set found 1
		} else {
			set new_contents [lappend new_contents $line]
		}
	}
	
	if {$found == 0} {
		set new_contents [lappend new_contents "$param = $value"]
	}
	return $new_contents
}

# Load wlan0.conf and sys.conf configuration files
set wlan [config_load $::g_web_config_folder/wlan0.conf]
#Add for pass WPS test of TestDevice.App 11.1.8319.0 by Ricky Cao on Nov. 19 2008
set retry 0
while {$retry < 10} {
	if {$wlan == ""} {
		unset wlan
		sleep 1
		set wlan [config_load $::g_web_config_folder/wlan0.conf]
        	incr retry 1
		exec echo $retry - Retry for open wlan0.conf in mtlk_wps_cm.tcl > /dev/console
	} else {
		break
	}
}
#Ricky Cao on Nov. 19 2008
set sys [config_load $::g_web_config_folder/sys.conf]

# get the network type (AP/STA)
set network_type   [get_param network_type $wlan]


proc debug_puts {str} {
	set CURRENT_TIME [get_current_uptime]
	
	catch {[exec echo "\[$CURRENT_TIME\]$str" >$::WPSScriptsDbgOut]}
	
	set terminals ""
	set _terminals [exec ls -1 /dev/pts]
	set _terminals [split $_terminals "\n"]

	if {[file exists /tmp/wps-dbg-telnet]!=0} {
		foreach terminal $_terminals {
			lappend terminals "/dev/pts/$terminal"
		}
		
		foreach terminal $terminals {
			catch{[exec echo "\[$CURRENT_TIME\]$str" >$terminal ]}
		}
	}
	
}

# Write the current action to /tmp ini file
proc update_wps_action {action} {
	catch {exec echo "WPS_action = $action" > $::WPS_ACTION_STATFILE}
}

#	
# Helper functions
#
proc kill_supplicant {} {
	debug_puts "mtlk_wps_cmd.tcl:Killing supplicant"
	set application "wpa_supplicant"
	
	set ps ""
	catch { set ps [split [exec ps | grep "$application"] "\n"] }
	
	foreach line $ps {
		catch {exec kill [lindex $line 0]}
	}
	catch {exec ifconfig wlan0 down}
	catch {exec iwconfig wlan0 essid ''} 
	catch {exec ifconfig wlan0 up}
		
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


# Send WPS registration command to the command and PBC FIFO's
proc WPS_Registration {cmd pbc} {
	debug_puts "WPS_Registration cmd=$cmd pbc=$pbc"
	exec echo $cmd > ${::FIFO_CMD}
	exec echo $pbc > ${::FIFO_PBC}
}

# Send the Enrollee PIN to the FIFO
proc WPS_SendEnrolleePIN {pin} {
	exec echo $pin > ${::FIFO_PIN_ENROLLEE}
}

# Stop the WPS Message loop
proc stopWPSMessageLoop {} {
	set ps ""
	catch { set ps [split [exec ps | grep $::WPS_MSG_LOOP_TCL] "\n"] }
	foreach line $ps {
		set process_num [lindex $line 0]
		catch {exec kill -9 $process_num}
	}
}

# Start the WPS Message loop
proc startWPSMessageLoop {} {
	catch {exec $::g_web_wpa_app_folder/$::WPS_MSG_LOOP_TCL start &}
}

# Restart the WPS Message loop 
proc restartWPSMessageLoop {} {
	stopWPSMessageLoop
	sleep 1
	startWPSMessageLoop
}

# Get the current uptime and return it
proc get_current_uptime {} {
	regexp "^(.+)\..* .*" [exec cat /proc/uptime] dummy CURRENT_TIME
	return $CURRENT_TIME
}

# Reset the start time ini file
proc resetStartTime {} {
	catch {exec rm $::WPS_START_TIME_STATFILE}
	set CURRENT_TIME [get_current_uptime]
	catch {exec echo "WPS_StartTime = $CURRENT_TIME" > $::WPS_START_TIME_STATFILE}
	#Jacky.Yang 30-Nov-2008, for GUI Dispaly
	catch {exec echo "$CURRENT_TIME" > /var/wps_start_time}
}

# Reset the last WPS code received.
proc resetLastWPSCode {} {
	catch {exec rm $::WPS_LAST_CODE_STATFILE}
}

proc resetCurrentWPSStatus {} {
	catch {exec rm $::WPS_CURRENT_STATUS_STATFILE}
}

proc getWPSSessionLen {} {
	set conf_start [config_load $::WPS_START_TIME_STATFILE]
	set startupTime [get_param WPS_StartTime $conf_start]
	set currentTime [get_current_uptime]
	if {$startupTime == 0} {
		return 0
	} else {
		return [expr $currentTime - $startupTime]
	}
}

proc getWPSSessionStat {} {
	set conf [config_load $::WPS_LAST_CODE_STATFILE]
	set lastCode [get_param WPS_LastErrorCode $conf]
	debug_puts "isWPSSessionActive : CODE='$lastCode'"
	return $lastCode
}

# Stop the WPS Message loop
proc stopWPSPBC {} {

	debug_puts "mtlk_wps_cmd.tcl: Terminating WPS_PBC.sh"
	catch {exec killall WPS_PBC.sh}

	set conf ""
	catch {set conf [exec grep WPS_PB /tmp/HW.ini]}
	if {$conf != ""} {
		set wpsPBCgpio ""
		catch {set cmd_res [regexp {.*=(.*)} $conf dummy wpsPBCgpio]	}
		if {$wpsPBCgpio != ""} {
			debug_puts "mtlk_wps_cmd.tcl: Terminating gpio $wpsPBCgpio listeners"
			set ps ""
			catch { set ps [split [exec ps | grep "cat $wpsPBCgpio"] "\n"] }
			foreach line $ps {
				set process_num [lindex $line 0]
				catch {exec kill -9 $process_num}
			}
		} else {
			debug_puts "mtlk_wps_cmd.tcl: Failed to get WPS_PB gpio from /tmp/HW.ini"
		}
	}

}


# Stop WPS Close all WPS related applications.
proc WPSActionStop {} {

	# Stop the message loop 
	debug_puts "mtlk_wps_cmd.tcl:Stopping WPS"
	stopWPSMessageLoop
	
	stopWPSPBC
	
	catch {exec rm $::WPS_ACTION_STATFILE}

	# notify driver helper to close the FIFO
	debug_puts "mtlk_wps_cmd.tcl:signaling USR1 to drvhlpr"
	catch {exec killall -USR1 drvhlpr}
	
	# kill wsccmd
	catch {exec killall wsccmd}
	sleep 1
	
	# make sure that wsccmd is really killed.
	catch {exec killall -9 wsccmd}
	
	# In case this is an AP, kill the hostapd as well
	if {$::network_type== $::AP} {
		catch {exec killall -9 hostapd}
	}
	
	sleep 1

	# remove the FIFO's
	catch {exec rm ${::FIFO_DEVICE_PIN}}
	catch {exec rm ${::FIFO_PIN_ENROLLEE}}
	catch {exec rm ${::FIFO_MSG}}
	catch {exec rm ${::FIFO_PBC}}
	catch {exec rm ${::FIFO_CMD}}
	catch {exec rm ${::FIFO_SSID_AP}}
	catch {exec rm ${::FIFO_STATUS}}
	catch {exec rm ${::NEIGHBORS_INFO_FILE}}
	catch {exec rm ${::FIFO_USER_SELECT_AP}}
	catch {exec rm ${::FIFO_MAC_AP}}
	catch {exec rm ${::FIFO_LED_EVENTS}}
	
	debug_puts "mtlk_wps_cmd.tcl:WPS Stopped"
}

# Start all WPS related applications
proc WPSActionStart {} {
	debug_puts "mtlk_wps_cmd.tcl:Starting WPS"
	set cur [pwd]
	cd /tmp
	
	set wsccmd_count 0
	catch {set wsccmd_count [exec ps | grep wsccmd | grep -v wsccmd_debug | grep -c -v grep]}
	debug_puts "mtlk_wps_cmd.tcl:wsccmd count:$wsccmd_count"
	
	if {$wsccmd_count == 0} {
	
		debug_puts "mtlk_wps_cmd.tcl:Initializing WPS"
		catch {exec ./mtlk_init_wps.sh}
		# Search for debug activation
		set dbgOut "/dev/null"
		if {[file exists /tmp/wsccmd-dbg] == 1} {
			set dbgOut [lindex [file_load /tmp/wsccmd-dbg] 0]
			debug_puts "mtlk_wps_cmd.tcl:wsccmd logs to '$dbgOut'"
			if {$dbgOut == ""} {
				set dbgOut "/dev/console"
			}
		} 
		debug_puts "mtlk_wps_cmd.tcl:running wsccmd"
		catch {exec ./wsccmd 1 2 3 4 > $dbgOut 2> $dbgOut &}
		sleep 2
		cd $::g_web_wpa_app_folder
	
		# notify driverhelper open the FIFO
		debug_puts "mtlk_wps_cmd.tcl:signaling USR2 to drvhlpr"
		catch {exec killall -USR2 drvhlpr}
	}
	
	# Reload the WPS_PBC.sh if it's not there.
	set ps ""
	catch {set ps [split [exec ps | grep "WPS_PBC.sh"] "\n"]}
	set count [llength $ps]
	debug_puts "mtlk_wps_cmd.tcl:PBC Script count $count"
	incr count -1
	
	if { $count < 1  } {
		debug_puts "mtlk_wps_cmd.tcl:starting PBC script"
		catch {exec /tmp/WPS_PBC.sh 1 $::network_type > /dev/null &}
	}

	cd $cur
	debug_puts "mtlk_wps_cmd.tcl:WPS started"
}

# Check if we're in progress (
proc CheckWPSSessionInProgress {} {
	if { $::wpsSessionStat == 6 } {
		debug_puts "mtlk_wps_cmd.tcl: WPS Session already in progress (Registering)."
		exit 0
	}
}

# Check if the manual session has timed out
proc CheckWPSManualSessionTimeout {} {
	if { $::wpsSessionStat == 5 } {
		debug_puts "mtlk_wps_cmd.tcl:manual session - resetting"
		WPSActionStop
		WPSActionStart
	}
}


proc SetGPIO {gpio value} {
	set conf ""
	set conf [exec grep $gpio /tmp/HW.ini]
	if {$conf != ""} {
		set ledGPIO ""
		catch {set cmd_res [regexp {.*=(.*)} $conf dummy ledGPIO]	}
		if {$ledGPIO != ""} {
			debug_puts "mtlk_wps_cmd.tcl:LED GPIO $ledGPIO"
			catch {exec echo $value > $ledGPIO}
		} else {
			debug_puts "mtlk_wps_cmd.tcl:Can not find $gpio in /tmp/HW.ini"
		}
	}

}

proc StopWPSLedBlink {} {
	SetGPIO WPS_activity_LED $::WPS_Idle_Blink
	SetGPIO WPS_error_LED $::WPS_Idle_Blink
}

proc startWPSLedBlink {} {
	SetGPIO WPS_error_LED $::WPS_Idle_Blink
	SetGPIO WPS_activity_LED $::WPS_Active_Blink
}

proc showUsage {} {
	puts "\nMetalink WPS control script"
	puts "This script is used to control the WPS Session state"
	puts ""
	puts "Syntax :"
	puts "mtlk_wps_cmd.tcl action "
	puts "\nactions : "
	puts "conf_via_pin <PIN> : Configure a STA via PIN. "
	puts "\t PIN : PIN Number"
	puts "conf_via_pbc : Configure a STA using PBC"
	puts "get_conf_via_pin <MANUAL>: Get configured by an AP using PIN"
	puts "\t MANUAL : 1 for manual AP selection, 0 for automatic AP selection"
	puts "get_conf_via_pbc : Get configured by an AP using PBC"
	puts "ap_selected_from_list <MAC> : Select an AP to connect to."
	puts "\t MAC  : MAC Address of AP to connect to"
	puts "show_settings : Display current WPS settings"
	puts "stop : Stop the WPS stack"
	puts "start : Start the WPS stack"
	puts "abort : Abort current WPS session on STA. call start to start the stack again."
	puts "save_settings : Save current WPS session settings to configuration."
	puts "script_debug <path> : Activate/Disable script debug level"
	puts "\t path  : location for log. Empty disables logging"
	puts "wsccmd_debug <path> : Activate/Disable wsccmd debug level"
	puts "\t path  : location for log. Empty disables logging"
}

#Main function

# Possible actions :
# get_params (???????????)
# conf_via_pin 
# conf_via_pbc
# get_conf_via_pin
# get_conf_via_pbc
# ap_selected_from_list SSID MAC
# show_settings
# stop
# start
# save_settings
# script_debug
# wsccmd_debug

# TODO : Must kill all previous instances before continuing, to prevent problems.

# Get the command line action
set action [lindex $::argv 0]
set actionValid 0
set wpsSessionStat [getWPSSessionStat]
init_wps_logging



if {$action == "-h" || $action == "/h" || $action == "help" || $action == "-help" || $action == "/help"} {
	set actionValid 1
	debug_puts "mtlk_wps_cmd.tcl:help"
	showUsage
}

# Configure enrollee via PIN	
if {$action =="conf_via_pin" } {
	set actionValid 1
	
	startWPSLedBlink
	
	set AP_SSID [get_param NonProc_ESSID $wlan]
	set enrollee_type [lindex $::argv 2]
	set enrollee_PIN [lindex $::argv 1]
		
	debug_puts "mtlk_wps_cmd.tcl:conf_via_pin '$network_type' '$AP_SSID' '$enrollee_type' '$enrollee_PIN'" 
	
	resetStartTime
	startWPSMessageLoop
	resetLastWPSCode
	update_wps_action "conf_via_pin"
	
	if {$network_type== $AP} {
		# enrollee type is STA
		WPS_Registration $CMD_REG_CONF_STA $NOT_PBC
	} elseif {$network_type== $STA} { 
		#exteranl registrar
		if {$enrollee_type == $AP_enrollee} {
			exec echo $CMD_REG_CONF_AP > ${::FIFO_CMD}
			exec echo "${param_AP_SSID}" > ${::FIFO_SSID_AP}
		}
		if {$enrollee_type == $STA_enrollee} {
			WPS_Registration $CMD_REG_CONF_STA $NOT_PBC
		}
	}	
	WPS_SendEnrolleePIN	$enrollee_PIN
}


# Configure enrollee via PBC
if {$action =="conf_via_pbc" } {
	if {$network_type==2} {
	
		startWPSLedBlink
		
		set actionValid 1
		debug_puts "mtlk_wps_cmd.tcl:conf_via_pbc" 
		
		resetStartTime
		startWPSMessageLoop
		resetLastWPSCode
		update_wps_action "conf_via_pbc"
		
		WPS_Registration $CMD_REG_CONF_STA $IS_PBC
	} else {
		debug_puts "mtlk_wps_cmd.tcl:This action is not valid for STA"
	}
}



# Get the device configured via PIN
if {$action =="get_conf_via_pin" } {
	if {$network_type==0} {
		set manual_AP [lindex $::argv 1]
		debug_puts "mtlk_wps_cmd.tcl:get_conf_via_pin '$network_type' '$::manual_AP'" 
	
		startWPSLedBlink
		kill_supplicant
		
		set actionValid 1
		
		

		WPSActionStart
		startWPSLedBlink
		
		CheckWPSManualSessionTimeout
		CheckWPSSessionInProgress
		
		
		resetStartTime
		startWPSMessageLoop
		resetLastWPSCode
		update_wps_action "get_conf_via_pin"
		
		if {$network_type== $AP} {
			exec echo $CMD_AP_GET_CONF > ${::FIFO_CMD}
		}
		if {$network_type== $STA} {
			WPS_Registration $CMD_STA_GET_CONF $NOT_PBC
			
			if {$manual_AP != "" } {
				set ::ap_select $manual_AP
			}
			exec echo $::ap_select > ${::FIFO_USER_SELECT_AP}
		}	
	} else {
		debug_puts "mtlk_wps_cmd.tcl:This action is not valid for AP"
	}
}

# Get the device configured via PBC
if {$action =="get_conf_via_pbc" } {
	if {$network_type==0} {
		debug_puts "mtlk_wps_cmd.tcl:get_conf_via_pbc" 
	
		startWPSLedBlink
		kill_supplicant
		
		set actionValid 1
		
		WPSActionStart

		startWPSLedBlink
		
		CheckWPSManualSessionTimeout
		CheckWPSSessionInProgress

		resetStartTime
		startWPSMessageLoop
		resetLastWPSCode
		update_wps_action "get_conf_via_pbc"
		
		WPS_Registration $CMD_STA_GET_CONF $IS_PBC
	} else {
		debug_puts "mtlk_wps_cmd.tcl:This action is not valid for AP"
	}
}

# Set WPS Scripts debug output
if {$action == "script_debug"} {
	set actionValid 1
	if {[llength $::argv] == 2} {
		set dbg_target [lindex $::argv 1]
		catch { exec echo $dbg_target >$::g_web_tmp_folder/wps-dbg }
	} else {
		catch { exec rm $::g_web_tmp_folder/wps-dbg }
	}
	
	restartWPSMessageLoop	
}

# Set wsccmd debug output
if {$action == "wsccmd_debug"} {
	set actionValid 1
	if {[llength $::argv] == 2} {
		set dbg_target [lindex $::argv 1]
		catch { exec echo $dbg_target >$::g_web_tmp_folder/wsccmd-dbg }
	} else {
		catch { exec rm $::g_web_tmp_folder/wsccmd-dbg }
	}

	WPSActionStop
	WPSActionStart
	restartWPSMessageLoop
}

# Show and update current AP security settings
if {$action =="show_settings" } {
	set actionValid 1
	debug_puts "mtlk_wps_cmd.tcl:show_settings"
	set NonProc_ESSID [get_param NonProc_ESSID $wlan]
	set NonProcSecurityMode [get_param NonProcSecurityMode $wlan]
	set NonProc_WPA_Personal_PSK [get_param NonProc_WPA_Personal_PSK $wlan]
	
	# Jacky.Yang 30-Nov-2008, for GUI get SSID
	#catch {exec echo jacky get ESSID > /dev/console}
	catch { exec echo $NonProc_ESSID > /var/wps_success_ssid }
	
	#puts "SSID: $NonProc_ESSID"
	#puts "Security Mode: $NonProcSecurityMode"
	#puts "PSK: $NonProc_WPA_Personal_PSK"
}

# Connect to a specific AP
if {$action =="ap_selected_from_list" } {
	if {$network_type==0} {
		
		set actionValid 1
		if {[llength $::argv] == 2} {
			set mac [lindex $::argv 1]
		} else {
			set mac [lindex $::argv 2]
		}
		debug_puts "mtlk_wps_cmd.tcl:ap_selected_from_list '$mac'"
		exec echo WPS_LastErrorCode = 1 > $::WPS_LAST_CODE_STATFILE
		debug_puts " $::WPS_LAST_CODE_STATFILE"
				
		resetStartTime				
		
		# send the command to the FIFO's
		exec echo $mac > ${::FIFO_MAC_AP}
	} else {
		debug_puts "mtlk_wps_cmd.tcl:This action is not valid for AP"
	}
}

if {$action == "restart"} {
	if {$network_type==0} {
		set actionValid 1
		debug_puts "mtlk_wps_cmd.tcl:WPS restart"
		WPSActionStop
		WPSActionStart
		restartWPSMessageLoop
	} else {
		debug_puts "mtlk_wps_cmd.tcl:This action is not valid for AP"
	}
}

# Stop WPS
if {$action == "stop"} {
	#Jacky.Yang 27-Nov-2008, delete tag from GUI setting.
	catch {exec echo mtlk_wps_cmd.tcl, stop: delete webCommit > /dev/console}
	catch {exec rm -f /var/webCommit}
	catch {exec rm -f /var/gui_wps_waiting}

	if {$network_type==0} {
		set actionValid 1
		debug_puts "mtlk_wps_cmd.tcl:WPS Stop"
		WPSActionStop
	} else {
		debug_puts "mtlk_wps_cmd.tcl:This action is not valid for AP"
	}
	
}



# Abort WPS
if {$action == "abort"} {
	#Jacky.Yang 27-Nov-2008, delete tag from GUI setting.
	catch {exec echo mtlk_wps_cmd.tcl, abort: delete webCommit > /dev/console}
	catch {exec rm -f /var/webCommit}
	catch {exec rm -f /var/gui_wps_waiting}

	if {$network_type==0} {
		set actionValid 1
		debug_puts "mtlk_wps_cmd.tcl:WPS Abort"
		WPSActionStop
		StopWPSLedBlink
		resetCurrentWPSStatus
		resetLastWPSCode

		set current [pwd]
		cd $::g_web_web_app_folder
		debug_puts "mtlk_wps_cmd.tcl:Reconnecting to last good config AP"
		exec $::g_web_web_app_folder/init_security.tcl reactivate &
		cd $current
	} else {
		debug_puts "mtlk_wps_cmd.tcl:This action is not valid for AP"
	}
	
}


# Start WPS
if {$action == "start"} {
	if {$network_type==0} {
		set actionValid 1
		debug_puts "mtlk_wps_cmd.tcl:WPS start"
		WPSActionStart
		restartWPSMessageLoop
	} else {
		debug_puts "mtlk_wps_cmd.tcl:This action is not valid for AP"
	}
}

# notify the web that manual AP selection should take place now
if {$action == "manual_ap_selection"} {

}

# Save settings to wlan0.conf and sys.conf
if {$action == "save_settings"} {

	#
	# save the wps settings
	#
	
	set actionValid 1
	debug_puts "mtlk_wps_cmd.tcl:save_settings"

	# see if this is not the first instance, to prevent problems with mounting and unmounting.
	set ps ""
	catch {set ps [split [exec ps | grep "mtlk_wps_cmd.tcl"] "\n"]}
	set count [llength $ps]
	if { $count >2 } {
		debug_puts "mtlk_wps_cmd.tcl:save_settings aborted (already doing something)"
		exit
	}
	
	# get current settings.
	set ssid		[get_param "NonProc_ESSID" $wlan]
	set psk			[get_param "NonProc_WPA_Personal_PSK" $wlan]
	set security  	[get_param "NonProcSecurityMode" $wlan]

	# if this is an AP, update accordingly
	if {$network_type==2} {
		set conf_file $::HOSTAPD_CONF 
		if {[file exists $conf_file] == 1} {
			set security $::securityOpen
		}
		set conf_file_lines [file_load $conf_file] 

		foreach line $conf_file_lines {
			set line [string trim $line]
			# Ignore commented lines
			if {[regexp {^#} $line dummy]>0} {
				continue
			}

			if {[regexp {([^ ]+)=(.+)} $line dummy pname pvalue]>0} {
				if {[string compare $pname "ssid"]==0} {
					set ssid  $pvalue
				}
				if {[string compare $pname "wpa_psk"]==0} {
					set psk  $pvalue
				}
				if {[string compare $pname "wpa_passphrase"]==0} {
					set psk  $pvalue
				}
				if {[string compare $pname "wpa"]==0} {
					if {$pvalue != 0} {
						set security  $pvalue
					}
				}
			}
		}
	} 
	
	# if this is a STA, update accordingly
	if {$network_type==0} {
		# wait some time to give wsccmd a chance to update supplicant config file
		sleep 5

		set conf_file $::SUPPLICANT_CONF 
		if {[file exists $conf_file] == 1} {
			set security $::securityOpen
		}
		set conf_file_lines [file_load $conf_file] 
		foreach line $conf_file_lines {
		set line [string trim $line]
		
			# Ignore commented lines
			if {[regexp {^#} $line dummy]>0} {
				continue
			}
			
			if {[regexp {(.+)=(.+)} $line dummy pname pvalue]>0} {
				if {[string compare $pname "ssid"]==0} {
					set ssid  $pvalue
					set ssid [string trim $ssid \" ]
				}
				if {[string compare $pname "psk"]==0} {
					set psk  $pvalue
					set psk [string trim $psk \" ]
				}
				if {[string compare $pname "key_mgmt"]==0 && [string compare $pvalue "WPA-PSK"]==0 } {
					set security $::securityWPAPersonal
				}
			}
		}
	} 
	
	# set new values
	set wlan [set_param $wlan "NonProc_ESSID" ${ssid}] 
	set wlan [set_param $wlan "NonProc_WPA_Personal_PSK" ${psk}] 
	set wlan [set_param $wlan "NonProcSecurityMode" ${security}] 
	# Jacky.Yang 40-Nov-2008, for set correct probe info. in the wireless
	set wlan [set_param $wlan "Wildcard_ESSID" ${ssid}]
	if {$network_type==2} {
		set wlan [set_param $wlan NonProc_WPS_ApStatus ${AP_proxy}] 
	}
	
	debug_puts "mtlk_wps_cmd.tcl:SSID=${ssid} PSK=${psk} SecurityMode=${security}"
	
	# save the configuration to file
	config_save $::g_web_config_folder/wlan0.conf $wlan
	
	# copy the configuration to /tmp
	catch {exec cp $::g_web_config_folder/wlan0.conf $::g_web_tmp_folder/wlan0.conf }
	
	# create the user defined ASP update file 
	catch {exec cp $::g_web_web_app_folder/web_update_wps.asp $::g_web_tmp_folder/user_defined.asp}

	# store conf file to check that it was updated before saving to flash
	exec cat /mnt/jffs2/wpa_supplicant0.conf > /tmp/new_supplicant1.conf

	catch {exec config_umount.sh}
	catch {exec config_mount.sh}
	
	# store conf file to check that it was updated before saving to flash
	exec cat /mnt/jffs2/wpa_supplicant0.conf > /tmp/new_supplicant2.conf

	# signal the web that the settings have changed.
	catch { exec killall -HUP webs}
	
	catch { exec $::g_web_combined_ver_folder/etc/mtlk_init_wls_apps.sh}
}

# If action was not valid, print error
if {$actionValid==0} {
	debug_puts "Invalid action specified ($action)"
	showUsage
} 

# Make sure we're out of here !
exit 0
