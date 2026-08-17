#!/bin/tclsh

source ini.tcl

set driverApiHandler {}

###################################
#		TCL functions to be used by the driver api.ini
###################################
proc test {condition ret_if_true ret_if_false} {
	if {[expr $condition]} { 
		return $ret_if_true
	} else {
		return $ret_if_false
	}
}

proc ProcSetDebugLevel {param value} {
	return "echo $value > /proc/net/mtlk/$param"
}
	
proc ProcGet {interface param} {
	exec cat /proc/net/mtlk/$interface/$param
}

proc ProcSet {interface param value} {
	return "echo $value > /proc/net/mtlk/$interface/$param"
}	

proc ProcDebugGet {interface param} {
	exec cat /proc/net/mtlk/$interface/$param
}

proc ProcDebugSet {interface param value} {
	return "echo $value > /proc/net/mtlk/$interface/$param"
}	

proc IWPRIV_Set {interface param value} {
	return "iwpriv $interface s$param $value"
}

proc IWPRIV_Set_Vec {interface param value value1} {
    return "iwpriv $interface s_$param $value $value1"
}

proc IWPRIV_Get {interface param} {
	return "iwpriv $interface g$param"
}

proc iwconfig {interface param value} {
	if {$value == ""} {
		set value auto
	}
	return "iwconfig $interface $param $value"
}
proc iwpriv {interface param value} {
	return "iwpriv $interface $param $value"
}
proc iwgetid {interface param} {
	return "iwgetid $interface $param"
}

# execute the command, run regexp on the response and return first match
proc parse {cmd reg} {
	if {$cmd != ""} {
		if {![catch {set ret [eval "exec $cmd"]}]} {
			if {[regexp $reg $ret all match] > 0} {
				return $match
			}			
		}
	}
	return ""
}
###################################################
#  Driver API functions
#
#
###################################################

proc DriverGet {interface param} {
  set param [string trim $param]
  set retVal [getParamExecuteCmd GET $param]
  if {$retVal != ""} {
		set retVal [eval $retVal]
  }  
  return $retVal

}

proc DriverSet {interface param value} {
	set param [string trim $param]
	set value [string trim $value]
	set retVal [getParamExecuteCmd SET $param]
	if {$retVal != ""} {
		if {[catch {set ret [eval $retVal]}]} {
			return "# failed to eval $retVal"
		}
		return $ret
	}  
	return $retVal
}

proc DriverSet_vec {interface param value value1} {
        set param [string trim $param]
        set value [string trim $value]
        set value1 [string trim $value1]
        set retVal [getParamExecuteCmd SET $param]
        if {$retVal != ""} {
                if {[catch {set ret [eval $retVal]}]} {
                        return "# failed to eval $retVal"
                }
                return $ret
        }
        return $retVal
}

proc setDriverApiHandler {db_file} {
	global driverApiHandler
	# load the ini file
	set driverApiHandler [IniOpen $db_file]	
}


proc getParamExecuteCmd {section param} {
	global driverApiHandler	
	# get the parameter key from the command section
	set retVal [IniGetKey $driverApiHandler $section $param]
	if {$retVal != ""} {
			set retVal1 [IniGetKey $driverApiHandler $retVal $section]
			if {$retVal1 != ""} {
				return $retVal1
			}	
	}
	return $retVal;	
}
proc DriverSetAll {interface conf_file} {
	
	set conf_param [open /tmp/set_driver_params.sh w]
	set wep 0
	
	if {[file readable $conf_file] ==  1} {
		set fp [open $conf_file r]
		set contents [read $fp]
		close $fp
	} else {
		puts "echo Can not read file $conf_file"
		return ''
	}
	
	#for run conf_param.conf as sh script
	puts $conf_param {#!/bin/sh}
	
	#Update parameters according to DB. 
	set contents [split $contents "\n"]
	foreach line $contents {
		if {[regexp {(.*) = (.*)} $line all param value] == 1} {
			if {$param == "WepEncryption"} {
				set wep $value
				continue
			}
			set ret [DriverSet $interface $param $value]
			if {$ret != ""} {
				puts $conf_param $ret
				#eval "exec $ret"
			}
		}
	}
	# this is an ugly workaround, to make sure that we set WepEncryption after we done with the keys
	set ret [DriverSet $interface "WepEncryption" $wep]
	if {$ret != ""} {
		puts $conf_param $ret
		#eval "exec $ret"
	}


	# BridgeMode a driver parameter but we hold it in the sys.conf
	if {[file readable /mnt/jffs2/sys.conf] ==  1} {
		set fp [open /mnt/jffs2/sys.conf r]
		set contents [read $fp]
		close $fp
	} else {
		#puts "echo Can not read file $conf_file"
		return ''
	}
	set contents [split $contents "\n"]
	regexp {BridgeMode = ([0-9])} $contents all value	
	set ret [DriverSet wlan0 BridgeMode $value]
	if {$ret != ""} {
		puts $conf_param $ret
		#eval "exec $ret"
	}
	puts $conf_param "echo DriverSET Param is DONE"
	close $conf_param
}

#usege for command line interface
if {[info exists ::argv]} {
	set driverApiHandler [IniOpen driver_api.ini]
	if {[lindex $argv 0] == "DriverSetAll"} {		
		DriverSetAll [lindex $argv 1] [lindex $argv 2]
	}
	if {[lindex $argv 0] == "DriverParamSet"} {
		eval "exec [DriverSet [lindex $argv 1] [lindex $argv 2] [lindex $argv 3]]"
	}		
} else {
	set driverApiHandler {}
}
