#!/bin/tclsh

source mtlk_cgicommon.tcl

set read_var [lindex $argv 0];
# this is a workaround to view the selected channel (we can't use two handlers wlan and proc for the same parameter)
if {[string equal $read_var "SelectedChannel"]} {
	set read_var "Channel"
}
set value [get_proc_var $read_var];
# this is an ugly workaround to see the MAC address in xx:xx:xx:xx:xx:xx format
if {[string equal $read_var "MAC"]} {
	set newVal [string toupper $value]
	if {[regexp {(..)(..)(..)(..)(..)(..)} $newVal all a1 a2 a3 a4 a5 a6]} {
		set value "$a1:$a2:$a3:$a4:$a5:$a6"
	}	
}
	
puts "$value"
