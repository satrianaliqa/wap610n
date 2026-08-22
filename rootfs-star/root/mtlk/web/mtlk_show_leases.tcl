#!/bin/tclsh



proc show_leases {} {
		set leases ""
		set leases_html ""
		
		catch {set leases [exec dumpleases -f /tmp/udhcpd.leases]}
		#puts $leases
		if {[regexp "can't open" $leases]==0 && $leases!=""} {
			set leases [split $leases "\n"]
			set leases_html ""
			set first 1
			foreach line $leases {
				set a1 "<b>Mac Address</b>"
				set a2 "<b>IP-Address</b>"
				set a3 "<b>Expires in</b>"
				if {$first!=1} {
					regexp {(^[0-9a-hA-H][0-9a-hA -H]:[0-9a-hA-H][0-9a-hA -H]:[0-9a-hA-H][0-9a-hA -H]:[0-9a-hA-H][0-9a-hA -H]:[0-9a-hA-H][0-9a-hA -H]:[0-9a-hA-H][0-9a-hA -H]) ([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+) (.*)$} $line dummy a1 a2 a3
				}
				lappend leases_html "<tr><td>$a1</td><td>$a2</td><td>$a3</td></tr>"
				set first 0
			}
			set leases_html [join $leases_html "\n"]
		} else {
			set leases_html "<tr><td>No leases. </td></tr>"
		}
		
		
		return "<h2>Current Leases:</h2>\n<p><table style=\"font-family: monospace;\">\n$leases_html\n</table></p>"

}


set a [show_leases]
puts $a

regexp "(.+:.+:.+:.+:.+:.+) (.+\..+\..+\..+) (.*)" $a dummy a1 a2 a3
regexp {^[0-9]:} $a
regexp {(^[0-9a-hA-H][0-9a-hA -H]:[0-9a-hA-H][0-9a-hA -H]:[0-9a-hA-H][0-9a-hA -H]:[0-9a-hA-H][0-9a-hA -H]:[0-9a-hA-H][0-9a-hA -H]:[0-9a-hA-H][0-9a-hA -H]) ([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+) (.*)$} $a dummy a1 a2 a3