#!/bin/tclsh


set _dns ""
set _subnet ""
set _router ""
set _domain "local"
set _lease "864000"
set _wins ""
set _start ""
set _end ""


# usually used with "/etc/udhcpd/udhcpd.conf"
set config_file_defaults "" 


set config ""
 
proc file_load {filename} {
	set contents ""
	if {![catch {set fp [open "$filename" r]}]} {
		set contents [split [read $fp]	"\n"]
		close $fp
	}
	return $contents	
}
 
proc load_current_cfg {} {
	set current_conf [file_load $::config_file_defaults]
	foreach line $current_conf {
		if {[regexp {^#} $line]} {
			continue
		}
		if {[regexp {^start} $line]} {
			regexp {^start (.*)} $line dummy ::_start
			continue
		}
		if {[regexp {^end} $line]} {
			regexp {^end (.*)} $line dummy ::_end
			continue
		}
		if {[regexp {^opt dns} $line]} {
			regexp {^opt dns (.*)} $line dummy ::_dns
			continue
		}
		if {[regexp {^opt router} $line]} {
			regexp {^opt router (.*)} $line dummy ::_router
			continue
		}
		if {[regexp {^opt subnet} $line]} {
			regexp {^opt subnet (.*)} $line dummy ::_subnet
			continue
		}
		if {[regexp {^opt wins} $line]} {
			regexp {^opt wins (.*)} $line dummy ::_wins
			continue
		}
		if {[regexp {^opt domain} $line]} {
			regexp {^opt domain (.*)} $line dummy ::_domain
			continue
		}
		if {[regexp {^opt lease} $line]} {
			regexp {^opt lease (.*)} $line dummy ::_lease
			continue
		}
		
		lappend ::config $line
	}
}

proc save_cfg {} {
	set tmpfile [open /tmp/udhcpd.conf "w" ]
	
	puts $tmpfile "start $::_start"
	puts $tmpfile "end $::_end"
	
	foreach line $::config {
		puts $tmpfile $line
	}
	
	set temp [join $::_dns]
	puts $tmpfile "opt dns $temp"
	
	puts $tmpfile "opt subnet $::_subnet"
	puts $tmpfile "opt router $::_router"
	puts $tmpfile "opt wins $::_wins"
	puts $tmpfile "opt domain $::_domain"
	puts $tmpfile "opt lease $::_lease"
	
	close $tmpfile
}




set config_file_defaults [lindex $::argv 0]

load_current_cfg

set _dns_first 1
set argc [llength $argv]

set i 0
while {$i < $argc} {
	incr i
	set current_arg [lindex $::argv $i]
	if {$current_arg == "dns"} {
		incr i
		set val [lindex $::argv $i]
		if {$_dns_first == 1} {
			set _dns_first 0
			set _dns ""
		}
		lappend _dns "$val "
		continue
	}
	if {$current_arg == "start"} {
		incr i
		set val [lindex $::argv $i]
		set ::_start $val
		continue
	}
	if {$current_arg == "end"} {
		incr i
		set val [lindex $::argv $i]
		set ::_end $val
		continue
	}
	if {$current_arg == "wins"} {
		incr i
		set val [lindex $::argv $i]
		set ::_wins $val
		continue
	}
	if {$current_arg == "lease"} {
		incr i
		set val [lindex $::argv $i]
		set ::_lease $val
		continue
	}
	if {$current_arg == "gw"} {
		incr i
		set val [lindex $::argv $i]
		set ::_router $val
		continue
	}
	if {$current_arg == "domain"} {
		incr i
		set val [lindex $::argv $i]
		set ::_domain $val
		continue
	}
}

save_cfg 
