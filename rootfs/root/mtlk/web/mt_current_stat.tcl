#!/bin/tclsh
source mtlk_cgicommon.tcl

set results_lines {}
set web_root [get_env_var ASP_web_root]
set save_file_name [lindex $argv 0];
set time_str [lindex $argv 1];

proc puts_buffer {string_to_print} {
	global results_lines
	lappend results_lines $string_to_print
}

proc print_as_is {data} {
	foreach line $data {
		puts_buffer "$line"
	}	
}

proc build_stats {} {
	global results_lines
	set results_lines {}

	if {![catch {set mtdump [exec ../etc/mtdump wlan0 constatus 2>/dev/null]}]} {
		set mtdumplines [split $mtdump "\n"]
		
		foreach line $mtdumplines {
			if { [regexp {(Noise Level|Channel Load|Disconnections)[ ]+(.+)} $line fullline header val]} {
				puts_buffer "$header\t| $val"
			} else {
				puts_buffer "$line"
			}
			if { [regexp {Noise Level[ ]+} $line]} {
				if {![catch {set iwconfig [exec iwconfig]}]} {
					set iwconfiglines [split $iwconfig "\n"]
					foreach iwline $iwconfiglines {
						if { [regexp {Channel=([0123456789]+)} $iwline fulliwline channel] } {
							puts_buffer "Channel   \t| $channel"
						}
					}
				}
			}
		}
	}
}

proc print_stats {target} {
	global results_lines
	foreach line $results_lines {
		puts $target $line
	}
}

proc SplitIntoWords {block} {

    # We need to split the block up into words but cannot use
    # list operations as they throw away some significant
    # quoting, and [split] ignores braces as it should.
    # Therefore what we do is gradually build up a string out of
    # whitespace seperated strings. We cannot use [split] to
    # split the block into whitespace seperated strings as it
    # throws away the whitespace which maybe important so we
    # have to do it all by hand.

    set words {}
    set word ""

    while {[string length $block]} {
        # Look for the next group of whitespace characters.
        if {[regexp -indices "\[ \t\n\]+" $block all]} {
	    # Remove the text leading up to and including the white space
	    # from the block.
	    set text [string range $block 0 [lindex $all 1]]
	    set block [string range $block [expr {[lindex $all 1] + 1}] end]
	} else {
	    # Take everything up to the end of the block.
	    set text $block
	    set block {}
	}

	# Add the text to the end of the word we are building up.
        append word $text

        if { [catch {llength $word} length] == 0 && $length == 1} {
	    # The word is a valid list so add it to the list.
			if { ![regexp {\{\}} $word ]} {
				lappend words [string trim $word]
			}
			set word {}
        }
    }

    # If the last word has not been added to the list then there
    # is a problem.
    if { [string length $word] } {
	error "incomplete word \"$word\""
    }

    return $words
 }

proc save_stats {} {
	global save_file_name
	global web_root
	global time_str
	set output_file [open $save_file_name "w" ]
	if {$time_str != ""} { 
		puts $output_file "Time taken\t| $time_str"
	}
	if {![catch {set meminfo [exec cat /proc/meminfo]}]} {
		set meminfoline [lindex [split $meminfo "\n"] 0]
		set words [SplitIntoWords $meminfoline]
		set totalMemory [lindex $words 1]
		puts $output_file "Total Memory\t| $totalMemory KB"
	}
	if {![catch {set vmstat [exec vmstat 2 2]}]} {
		set vmstatline [lindex [split $vmstat "\n"] 3]
		set words [SplitIntoWords $vmstatline]
		set freeMemory [lindex $words 3]
		puts $output_file "Free Memory\t| $freeMemory KB"
		set CPUidle [lindex $words 14]
		set CPUusage [expr "100 - $CPUidle"]
		puts $output_file "CPU Usage\t| $CPUusage %"
		puts $output_file ""
	}
	print_stats $output_file
	close $output_file
}

build_stats
save_stats 
