proc debug_puts {str} {
	set CURRENT_TIME [get_current_uptime]
	
	catch {[exec echo "\[$CURRENT_TIME\]$str" >$::WPSScriptsDbgOut]}
	
	set terminals ""

	if {[file exists /tmp/wps-dbg-telnet]!=0} {
		set _terminals [exec ls -1 /dev/pts]
		set _terminals [split $_terminals "\n"]
		foreach terminal $_terminals {
			lappend terminals "/dev/pts/$terminal"
		}
		
		foreach terminal $terminals {
			catch {[exec echo "\[$CURRENT_TIME\]$str" >$terminal ]}
		}
	}
	
}
