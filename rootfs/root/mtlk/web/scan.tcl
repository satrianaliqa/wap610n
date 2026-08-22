#!/bin/tclsh

# source mtlk_cgicommon.tcl - no need , post_apply will do this
# post apply without params only loads functions
set do_save_security 0
source post_apply.tcl

# compare two scan results by RSSI
proc ScanCompare {a b} {
    # get RSSI value
	set a0 [lindex $a 5]
    set b0 [lindex $b 5]
    if {$a0 < $b0} {
        return -1
    } elseif {$a0 > $b0} {
        return 1
    }
    return 0
}


proc set_parameter_force {dummy param_name param_val} {
	set ::env($param_name) $param_val
}

# save current routing of 
proc get_route {interface} {
	set routing {}
	catch {set routing [exec route | grep $interface]}
	set lines [split $routing "\n"]
	return $lines
}

proc restore_route {route_list} {
	foreach line $route_list {
		# add only line where there is a gateway
		if {[string length [lindex $line 1]] > 6} {
			catch {exec route add -net [lindex $line 0] gw [lindex $line 1] netmask [lindex $line 2]  dev [lindex $line 7]}
		}
	}
	# TODO - add verfification of routing
}

# kill the suplicant and return previous routing
proc kill_suplicant {wlan_index} {
	# save current route for given interface
	set route_list [get_route "wlan$wlan_index"]
	
	#kill suplicant
	catch {exec killall wpa_supplicant}
	
	# wait for the supplicant to be really dead ...
	set supplicant_alive 1
	while {$supplicant_alive > 0} {
		sleep 1
		catch {set supplicant_alive [string first wpa_supplicant [exec ps | grep wpa]]} 	
	}
	return $route_list
}

proc ParseData {data dataReg newEventReg} {
	set retValue {}
	# split into lines
	set lines [split $data "\n"]
	# get the number of items
	set count [llength $dataReg]
	
	set EmptyValues {}
	# create an empty list the size of "regs"
	
	foreach item $dataReg {
		lappend EmptyValues -
	}
	
	set values $EmptyValues
	
	foreach line $lines {
		# check if we have a new event
		if {[regexp $newEventReg $line]} {
			# if we have some data
			if {[string compare $EmptyValues $values]!=0} {
				# add the data into the return list
				lappend retValue $values
			}
			#clean current values
			set values $EmptyValues
		}
		
		for {set index 0} {$index < $count} {incr index} {
			set reg [lindex $dataReg $index]
			
			if {[regexp $reg $line all match1]} {
				set values [lreplace $values $index $index $match1]
				break
			}
		}
			
	}
		
	if {[string compare $EmptyValues $values]!=0} {
		# add the data into the return list
		lappend retValue $values
	}
	
	return $retValue
}

proc get_div_id {new_ESSID new_Channel unique_id} {
	if {[regsub -all {( )} $new_ESSID "_" div_id]<1} {
		set div_id $new_ESSID
	}
	set div_id "DIVID_${div_id}_${new_Channel}_${unique_id}"
	
	set div_id "DIVID_${unique_id}"
	return $div_id
}

##################################################################
# Wait till supplicant is reactivated or wait 5 sec
##################################################################

proc waitTillSupReady { } {
	
	set status   ""
	set finished 0
	set timeout  5
	set counter  0
	
	while {$counter < $timeout && $finished != 1} {
		catch {set status [split [exec $::g_web_wpa_app_folder/wpa_cli status | grep wpa_state 2>/dev/null] "\n"]}
		if {[regexp {wpa_state( *)=( *)(.*)} $status all sp1 sp2 value] == 1} {
			if {$value == "COMPLETED"} {
				set finished 1
			} 
		}
		if {$finished == 0} {
			sleep 1			
		}
		incr counter
    }
	return
}

##################################################################
# Prints the scan results, using the /proc file created by the driver
##################################################################

proc do_web_scan {} {
	
	puts "<p><div id='scan_prompt'><h2>Performing Scan, Please wait. This might take a few seconds.</h2></div></p>"
	flush stdout
	
	global wpa_supplicant
	set wildcard " "
	set wildcard [get_env_var ASP_Wildcard_ESSID]
	set proc_err [set_proc_var $wildcard "Wildcard_ESSID"]
		
	# only 8 are real captions
	set cpation_count 8 
	#				0					1				2					3							4									5						6						7				8						9                    10
	set captions {ESSID  			Channel  			Band  				Bandwidth 				 {Encryption}		  			{AP Signal Strength}  	{High Throughput} 		{WPS Enabled}        {WPA/WPA2}  		     {WPA2}					{Radius}	         					  }
	set regs     { {ESSID:\"(.*)\"}  {Channel[:=]([0-9]+)}	{Extra:(.+) band}	{Extra:(40|20) MHz}	 {Encryption key:(off|on)}	  		{Signal level=-([0-9]+) dBm} 	{Extra:(not HT|HT)}   {Extra:(not WPS|WPS)}         {IE:.+(WPA) Version} 	{IE:.+(WPA2) Version} 		{Authentication Suites.+(802.1X)}        }
	set newEventReg {Cell [0-9]+ -}
	
	set wlan_index [get_env_var ASP_wlan_index]
		
	set connectedToAP [isConnected "wlan$wlan_index"]	
	if {[get_cgi_var "init"] == 1 && $connectedToAP == 1} {	
		# reactivate supplicant
		Reactivate_Hostapd_Supplicant STA
		waitTillSupReady		
	}	
		
	# get scan result	
	set iwlist [exec iwlist "wlan$wlan_index" scan]
			
	#parse results
	set scan_results [ParseData $iwlist $regs $newEventReg]
	set scan_results [lsort -command ScanCompare $scan_results]
		
	set len [llength $scan_results]

	puts "<script>showhide('scan_prompt','false');</script>"
	
	puts "<h2>Click on AP line to select and configure connection.</h2>"
	
	if {$len>0} {
		puts "<table><tr>"
		for {set j 0} {$j < $cpation_count} {incr j} {
			puts "<th> [lindex $captions $j] </th>"
		}	
		puts "</tr>"
	} else {
		puts "<p><h2>No AP's found.</h2></p>"
		puts "$iwlist<br>"
		puts "$scan_results<br>"
	}
	
	set cannot_connect 0
	for {set i 0} {$i < $len} {incr i} {
		#read line
		set line_params [lindex $scan_results $i]

		set new_ESSID   [lindex $line_params 0]		
		# in case of hidden essid , don't show it.
		if {$new_ESSID == "" } {
			continue
		}
		
		set new_Channel [lindex $line_params 1]
		set wep		 	[lindex $line_params 4]
		set signal      [lindex $line_params 5]
		set wpa         [lindex $line_params 8]
		set wpa2        [lindex $line_params 9]
		set radius      [lindex $line_params 10]
		set signal      [lindex $line_params 5]
		set wpsEnabled  [lindex $line_params 7]
		
		# Check if WPS is enabled or not on this AP
		if {$wpsEnabled=="WPS"} {
			set wpsEnabled "YES"
		} elseif {$wpsEnabled=="not WPS"} {
			set wpsEnabled "NO"
		} else {
			set wpsEnabled "Unknown"
		}
		
		# extract the security type
		set security 0
		if {$wep == "on"} {
			incr security 1
		}	
		if {$wpa == "WPA"} {
			incr security 1
		} 
		if {$wpa2 == "WPA2"} {
			incr security 2
		}
		if {$radius == "802.1X"} {
			set security 5
		}
		
		set security  [lindex {None WEP WPA WPA2 WPA+WPA2 RADIUS} $security]
				
		# change the security mode
		set line_params [lreplace $line_params 4 4 $security]
		set line_params [lreplace $line_params 7 7 $wpsEnabled]
		
		#TODO - we have now most of the security paramter that we need, send them to the security setting
		
		# fix AP signal strength -
		#1	Poor		-	-83
		#2	Bad			-83	-77
		#3	Normal		-77	-71
		#4	Good		-71	-65
		#5	Excellent	-65	-		
		if {$signal > 83} {
			set signal "Poor (-$signal dBm)"
		} elseif {$signal <= 83 && $signal > 77} {
			set signal "Bad (-$signal dBm)"
		} elseif {$signal <= 77 && $signal > 71} {
			set signal "Normal (-$signal dBm)"
		} elseif {$signal <= 71 && $signal > 65} {
			set signal "Good (-$signal dBm)"
		} else  {
			set signal "Excellent (-$signal dBm)"
		}
		set line_params [lreplace $line_params 5 5 $signal]
		
		set NonProcSecurityMode 1
		set wep_enc 0
		set cancel_authentication "&NonProc_Authentication=0"

		if {$security == "WEP"} {
			set NonProcSecurityMode 2
			set wep_enc 1
			set cancel_authentication ""
		} elseif {$security == "WPA" || $security == "WPA2" || $security == "WPA+WPA2"} {
			set NonProcSecurityMode 3
		} elseif {$security == "RADIUS"} {
			set NonProcSecurityMode 4
		} 
		
		set onClickAction "" 
		set apName [lindex $line_params 0]
		
		set onClickAction "onClick=\"window.location='/Security.asp?WepEncryption=${wep_enc}&NonProc_tmpESSID=$new_ESSID&NonProcSecurityMode=$NonProcSecurityMode${cancel_authentication}';\""
		puts "<tr onmouseover=\"setbgcolor(this,'#e2e2e2');\" onmouseout=\"setbgcolor(this,'');\"> "

		# loop over the line and print it
		for {set j 0} {$j < $cpation_count} {incr j} {
			set center ""
			if {$j!=0} {
				set center "align='CENTER'"
			}
			puts "<td ${center} $onClickAction >[join [lindex $line_params $j]]</td>"
		}
		
		puts "</tr>"
	}
	puts "</table><br>"
	if {$cannot_connect==1} {
		puts "<p id='prompt' class='ERROR'>&nbsp</p>"
	}
	if {$connectedToAP == 1} {
		puts "<p id='prompt' class='ERROR'>Warning: Refreshing Scan will temporarily disconnect you from the AP</p>"
	}
	puts "<p><input type='BUTTON' Name='RefreshBtn' Value='Refresh Scan' onClick='window.location=\"/cgi-bin/scan.tcl?init=1\"'></p>"
	#print_debug_info

	return ""
}

proc print_html_head_and_scripts {} {
	puts "content-type: text/html"
	puts ""
	puts ""
	puts "<html>\n<!- SD  ->\n<head>\n<title></title>\n<link rel='stylesheet' href='/normal_ws.css' type='text/css'>\n"
	puts "<script language=\"javascript\">
<!--
var state = 'none';
var divs=new Array;
function showhide(layer_ref,visible) {
	if (layer_ref.length<=1) {
		return;
		}

	if (visible == 'false') {
		state = 'none';
	}
	else {
		state = 'block';
	}
	if (document.all) { 
		eval( 'document.all.' + layer_ref + '.style.display = state');
	}
	else if (document.getElementById &&!document.all) {
		hza = document.getElementById(layer_ref);
		hza.style.display = state;
	}
}

function ShowOnly(layer_ref) {
    for (i = 0; i < divs.length; i++) {
		showhide(divs\[i\],'false');
    }
	if (layer_ref.length>=1) {
		showhide(layer_ref,'true');
		}
}

function setbgcolor(element, color) {
	if (element.style) element.style.background = color;
}

function setcolor(element, color) {
if (element.style) element.style.color = color;
}

//-->
</script> "
	puts "</head>\n<body>\n"
}

proc activate_security_and_connect {{commandline 0} {wait 1} } {
	if {$commandline!=1} {
		save_security
	}		

	set wlan_index [get_env_var ASP_wlan_index]

	# clean ESSID from the driver
	catch {exec iwconfig wlan$wlan_index ESSID ""}
	Reactivate_Hostapd_Supplicant STA	

	set connection 0
	set counter 0
	
	# if wait is disabled, sleep for 2 seconds to allow the dongle to disconnect.
	if {$wait==0} {
		sleep 2
	}
	
	while {$connection == 0 && $counter < 20 && $wait==1} {
		if {$commandline==0} {
		puts "."
		}

		flush stdout
		sleep 1
		if {![catch {set connected_str [exec iwconfig "wlan$wlan_index"]}]} {
			if {[string first "Access Point: Not-Associated" $connected_str] < 0} {
				set connection 1
			}
		}
	  incr counter
	}
	
	if {$connection > 0} {
		return 1
	} else {
		return 0
	}
}
set activate 0
set wait 0

if {[info exists argv]} {
	foreach arg $argv {
		if {[string compare "restartsupplicant" $arg] == 0 || [string compare "activate" $arg] == 0} {
			set activate 1
		}
		if {[string compare "wait" $arg] == 0 } {
			set wait 1
		}
		
	}
}

if {$activate==1} {
	catch { exec ./init_security.tcl } err

	set connected [activate_security_and_connect 1 $wait]
	puts $connected
	exit
}

####################################################################################

if {[string compare "Activate" [get_cgi_var "name"]] ==0} {
	print_html_head_and_scripts
	#print_debug_info
	puts "<h2>Trying to connect .<BR>Please wait a few seconds."
	flush stdout
	
	set connected [activate_security_and_connect]
	
	puts "</h2><BR><BR>"
	redirect_to "/cgi-bin/link_stats.tcl"
	flush stdout
	
	print_html_end
} else {

	print_html_head_and_scripts
	puts "<H1>Scan Results</H1>"
	if {[is_driver_loaded]==1} {
			
		#print_debug_info	
		do_web_scan		
	} else {
		puts "<p><div id='scan_prompt'><h2>Wireless driver is not loaded.</h2></div></p>"
	}
	print_html_end
}


