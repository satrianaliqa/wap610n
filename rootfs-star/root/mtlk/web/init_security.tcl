#!/bin/tclsh
# Thin compatibility shim delegating to POSIX shell /root/mtlk/web/init_security.sh
set cmd [list /root/mtlk/web/init_security.sh]
if {[info exists ::argv]} {
	set cmd [concat $cmd $::argv]
}
if {[catch {set out [eval exec $cmd]} err]} {
	puts stderr $err
	exit 1
}
if {$out != ""} {
	puts $out
}
