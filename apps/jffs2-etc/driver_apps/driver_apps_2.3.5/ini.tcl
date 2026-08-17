
proc IniInsert { handler name value } {
	set names [lindex $handler 0]
	set values [lindex $handler 1]
	lappend names $name
	lappend values $value
	return [list $names $values]
}
proc IniOpen { iniFile } {
	# handler is a list of two parts: 
	# 1. list of sections
	# 2. list of list of keys per section respectively
	# keys list are also a list of two parts
	# 1. list of keys
	# 2. list of values respectively
	set handler {}
	
	# read all file
	# read data file
	if {[file readable $iniFile] ==  1} {
	
		set fp [open $iniFile r]
		set contents [read $fp]
		close $fp
	} else {
		puts "echo Can not read file '$iniFile'"
		return ''
	}
		
	set contents [split $contents "\n"]
	
	set section {}
	set keys {{} {}}
	#go over the line in fill sections
	foreach line $contents {
		#puts "line '$line'"
		# check if this is a new section
		if {[regexp {^\[(.*)\]$} $line all new_section] == 1} {
			#puts "found section $new_section"
			#insert old section into handler
			set handler [IniInsert $handler $section $keys]
			#set new section and clear keys
			set section $new_section
			set keys {{} {}}
			#puts "new handler $handler"
		} else {
			# check if this is a key
			if {[regexp {([^=]*)=(.*)} $line all key value] == 1 } {
				set key [string trim $key]
				set value [string trim $value]
				#puts "found key $key=$value"
				#insert key into the list of keys
				set keys [IniInsert $keys $key $value]
				#puts "new keys $keys"
			}
			# this is nor key nor section - igonre it
		}
	}
	#insert last section into handler
	set handler [IniInsert $handler $section $keys]
	return $handler
}

proc IniGetKey {handler section key } {
	set ret ""
	set key [string trim $key]
	set section [string trim $section]
	#puts "Getting key '$key' from section '$section'"
	set index [lsearch [lindex $handler 0] $section]
	if {$index != -1} {
		#section found, search the keys
		set keys [lindex [lindex $handler 1] $index]
		set index [lsearch [lindex $keys 0] $key]
		if {$index != -1} {
			set ret [lindex [lindex $keys 1] $index]
		}
			
	}		
	#puts "found '$ret'"
	return $ret
}




