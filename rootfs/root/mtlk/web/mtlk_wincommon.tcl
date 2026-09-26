
#
# WINDOWS SPECIFIC COMMANDS
#
proc mt_exec args {
   set command ""
   foreach arg $args {
      set command "$command $arg"
   }
   exec $command
}

proc mt_print_os_type {} {
	puts "WIN<br>"
}

proc mt_get_file_lines {filename} {
	return [exec cmd /c type $filename]
}

#
# END OF WINDOWS SPECIFIC COMMANDS
#
