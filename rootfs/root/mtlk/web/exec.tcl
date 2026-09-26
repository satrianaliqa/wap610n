#!/bin/tclsh

# for use to run tcl commands from the ASP.
# E.G.
# <% 
# exec("/mnt/jffs2/web/exec.tcl","exec echo channel is " + Channel + " > /tmp/channel.txt")
# %>

if {[info exists ::argv]} {
    set cmd [join $::argv]
	eval $cmd
}
