#!/bin/tclsh

#puts "<br>Doing soft reboot..<br>"
cd ../etc
catch {[exec ./reload_mtlk_driver.sh] err}
cd ../web
