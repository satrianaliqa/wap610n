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
	
proc ProcGet {interface param} {
	exec cat /proc/sys/dev/mtlk/$interface/$param
}

proc ProcSet {interface param value} {
	return "echo $value > /proc/sys/dev/mtlk/$interface/$param"
}	

proc ProcDebugGet {interface param} {
	exec cat /proc/net/mtlk/$interface/$param
}

proc ProcDebugSet {interface param value} {
	return "echo $value > /proc/net/mtlk/$interface/$param"
}	

proc IWPRIV_Set {interface param value} {
	return "iwpriv $interface s_$param $value"
}
proc IWPRIV_Set_Vec {interface param value value1} {
    return "iwpriv $interface s_$param $value $value1"
}
proc IWPRIV_Get {interface param} {
	return "iwpriv $interface g_$param"
}

proc AC_Set {interface param value} {
	regexp {AC_(VO|VI|BE|BK)(\.|_)(.*)} $param all type connector sufix
	return [ProcSet $interface "AC_$type/$sufix" $value]
}

proc AC_Get {interface param} {
	regexp {AC_(VO|VI|BE|BK)(\.|_)(.*)} $param all type connector sufix
	return [ProcGet $interface "AC_$type/$sufix"]
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

proc AocsRestrictCh_Set {interface value} {
	return "echo 20,2,AOCS,AocsDontUseChannels,$value > /proc/sys/dev/mtlk/$interface/ConfigCommand\n cat /proc/sys/dev/mtlk/wlan0/ConfigCommand"
}

proc AocsRestrictCh_Get {interface} {
	return "echo 20,3,AOCS,AocsDontUseChannels > /proc/sys/dev/mtlk/$interface/ConfigCommand\n cat /proc/sys/dev/mtlk/wlan0/ConfigCommand"
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
		# check if the return value is a section
		set index [lsearch [lindex $driverApiHandler 0] $retVal]
		if {$index != -1} {
			# if so, read command from this section
			set retVal [IniGetKey $driverApiHandler $retVal $section]
		}
	}
	return $retVal;	
}
	