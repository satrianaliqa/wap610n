#!/bin/tclsh

#
#
# DHCP Client event handler and entry point
# 
# This script can be used to start/stop the DHCP client, and it handles the differetn DHCP events
# sent by the udhcpc script.
#
# Options:
# startup : start the udhcpc DHCP client with the dhcp.tcl script as it's event handler
# debug   : when used with startup (dhcp.tcl startup debug) will enable logging to /tmp/dhcp.log	
# kill    : kill the udhcpc DHCP client
# bound/renew/ok : DHCP event for successfull ip receive from the DHCP server
# fail/nak/leasefailed/deconfig : DHCP event for unsuccesfull ip receive from the DHCP server
#
#


# Set SET_HOSTNAME to 1 to allow the config_nic to configure local host name.
set SET_HOSTNAME 0
set RESOLV_CONF "/etc/resolv.conf"
set LOGGING 0
set REBOOT 1

#Jacky.Yang 21-Nov-2008
set startupWaitTimer 10
set renewWaitTimer 15
set longerWaitTimer 10

# Source the web_config file that contains platform-dependent paths
if { [file exists /mnt/jffs2/web/web_config.tcl] } { 
	source /mnt/jffs2/web/web_config.tcl
} elseif { [file exists /root/mtlk/web/web_config.tcl] } { 
	source /root/mtlk/web/web_config.tcl
} elseif { [file exists  ../web/web_config.tcl] } {    
        source ../web/web_config.tcl 
} else {
	# TODO: Error handling!
	puts "Missing web configuration file! This is needed for setting paths."
	exit 1
}

set wlan_index 0
set sys_config_file				 "/tmp/sys.conf"
set config_file					 "/tmp/wlan${wlan_index}.conf"


proc update_web_ip {ip_addr} {
	# Update the web with the new IP
	catch {exec echo "current_lan_ip = $ip_addr" > /tmp/updates.ini}
	catch {exec killall -HUP webs}
}

##################################################################
# Jacky.Yang 25-Jun-2009, Leave here.
##################################################################
proc exit_function {} {
	#Jacky.Yang 25-Jun-2009, if wps.c can found /var/retry_dhcp_client then we can re-try renew udhcpc.
	catch {exec echo 1 > /var/retry_dhcp_client}
	exit 0
}


##################################################################
# send a message to all console and telnet sessions connected
##################################################################
proc message_all {message} {
	set terminals ""
	set _terminals ""
	set pts 1
	if {[file exists /dev/pts]} {
		# Star has terminals under /dev/pts folder
		set _terminals [exec ls -1 /dev/pts]
		
	} else {
		# Dongle has terminals under /dev: files ptyp0 etc.
		# Currently, this is unsupported, see following TODO...
		
		### TODO: How to do ls with wildcards under tcl???
		### e.g.   
		###			set _terminals [exec ls -1 /dev/pt*]

		set _terminals ""
		set pts 0
	}
		set _terminals [split $_terminals "\n"]

		if {[file exists /tmp/dhcp-dbg-telnet]!=0} {
			foreach terminal $_terminals {
				if {$pts == 1} {
					lappend terminals "/dev/pts/$terminal"
				} else {
					lappend terminals "$terminal"
				}
			}
		}
	
	if {[file exists /tmp/dhcp-dbg-console]!=0} {
		lappend terminals "/dev/console"
	}

	foreach terminal $terminals {
		exec echo $message >$terminal 
	}
}


##################################################################
# Init the log
##################################################################
proc init_log {} {
	global LOGGING
	if {$LOGGING == 1} {
		catch {exec echo start > /tmp/dhcp.log}
	} else {	
		catch {exec rm /tmp/dhcp.log}
	}
}


##################################################################
# write msg to the /tmp/dhcp.log file
##################################################################
proc log {msg} { 
		if {[file exists "/tmp/dhcp-dbg-file"]!=0} {
		catch {exec echo $msg >> /tmp/dhcp.log}
	}

	message_all "DHCP: $msg"
}


##################################################################
# Load a file and return it's content
##################################################################
proc file_load {filename} {
	set contents ""
	catch {set contents [exec cat $filename]}
	set contents [split $contents "\n"]
	return $contents	
}

##################################################################
# Get the value of the environment variable 'name'
##################################################################
proc get_env_var {name} {
	if {[info exists ::env($name)]!=0} {
		return $::env($name)
	} 
	return ""
}

##################################################################
# Get the value of the system configuration variable param_name
##################################################################
proc get_parameter {param_name} {
  global config_file
  global sys_config_file
  
  set params [file_load $sys_config_file]
  foreach param $params {
	if {[regexp {(^[^ ]+)[ ]*=[ ]*(.+)} $param dummy pname pval]>0} {
		if {[string compare $param_name $pname]==0} {
			#debug "(sys.conf) '$param_name=$pval'"
			return $pval
		}
	}
  }
  
  set params [file_load $config_file]
  foreach param $params {
	if {[regexp {(^[^ ]+)[ ]*=[ ]*(.+)} $param dummy pname pval]>0} {
		if {[string compare $param_name $pname]==0} {
			#debug "(wlan0.conf) '$param_name=$pval'"
			return $pval
		}
	}
  }
    
  #puts "'$param_name='"
  return ""
}

##################################################################
# Get the auto_network_type from the configuration
##################################################################
proc get_auto_ap {} {
	set val [get_parameter auto_network_type]
	#log "auto_network_type==$val"
	return $val
}

##################################################################
# Get the ip_config_method from the configuration
##################################################################
proc get_ip_config {} {
	set val [get_parameter ip_config_method]
	#log "ip_config_method==$val"
	return $val
}

##################################################################
# Stop the UPNP daemon
##################################################################
proc stop_upnpd {} {
	log "Stopping UPNPD\n"
	catch {exec killall -SIGINT upnpd}
}

##################################################################
# Start the UPNP daemon
##################################################################
proc start_upnpd {} {
	log "Starting UPNPD\n"
	catch {exec upnpd &}
}

##################################################################
# Restart the UPNP daemon
##################################################################
proc restart_upnpd {} {
	stop_upnpd
	sleep 1
	start_upnpd
}


##################################################################
# Check the wireless connection status
##################################################################
proc get_wls_status {} {
	if {[get_parameter network_type] == "2"} {
		return ""
	}
		
	set status "0"
	
	catch {set wls_status_file [exec cat /tmp/wls_link_stat]}
	catch {regexp {WLSLinksStatus = (.*)} $wls_status_file all status}
	
	if {$status != "0"} {
		log "WLS Link is UP"
		return 1
	} 	
	log "WLS Link is DOWN"
	return 0
}

set NETMASK [get_env_var NETMASK]
set BROADCAST [get_env_var BROADCAST]
set ip [get_env_var ip]
set interface [get_env_var interface]
set domain [get_env_var domain]
set hostname [get_env_var hostname]
set wlan_link_up [get_wls_status]
set auto_ap [get_auto_ap]
set ip_config [get_ip_config]
set current_config [get_env_var hostname]

proc call_generic_event_script {} {
	set external_event "$::g_web_wpa_app_folder/dhcp_event.sh"
	if {[file exists $external_event] == 1} {
		log "call_generic_event_script:Calling $external_event"
		catch {exec $external_event}
	}
}

##################################################################
# Configure the ethernet port with the provided data
##################################################################
proc config_nic {iface ip_addr mask host domain brdcast routing dns} {
	global current_hostname
	set cmd_broadcast ""
	set cmd_netmask ""

	if {$brdcast != ""} {
		set cmd_broadcast "broadcast $brdcast"
	}

	if {$mask != ""} {
		set cmd_netmask "netmask $mask"
	}

	log "config_nic ip:$ip_addr mask:$mask host:$host domain:$domain brdcast:$brdcast routing:$routing dns:$dns"
	catch {exec echo dhcp.tcl: config_nic=>ip:$ip_addr mask:$mask host:$host domain:$domain brdcast:$brdcast routing:$routing dns:$dns > /dev/console}

	stop_upnpd
	catch {exec ifconfig $iface $ip_addr $cmd_broadcast $cmd_netmask}
	start_upnpd
	#shown current ip address
	exec echo "Current IP Address is - $ip_addr" > /dev/console

	# SET route
	# split the routes by '\n'
	if {$routing != ""} {
		log "setting routing info"
		set routing [split $routing " "]
		set _defaults [split [exec route -n] "\n"]
		foreach line $_defaults {
			if {[regexp {^default.*$iface} $line] == 1 || [regexp {^0\.0\.0\.0.*$iface} $line] == 1} {
				#log "deleting default route"
				catch {exec route del default gw 0.0.0.0 dev $iface}
			}
		}

		#log "Setting new default routes"	
		set routes_count [llength $routing]
		#log "default route count : $routes_count"	
		for {set i 0} {$i<$routes_count} {incr i} {
			set line [lindex $routing $i]
			log ">> route add default gw $line dev $iface"
			catch {exec route add default gw $line dev $iface}
		}
	}

	# SET DNS
	# split the DNS by ' '

	# Update resolver configuration file
	set R ""
	if {$domain != ""} {
		log "setting domain $domain"
		set R "domain $domain "	
	}
	#log "DNS:$dns"
	set dns [split $dns " "]
	set dns_count [llength $dns]
	#log "DNS count :$dns_count"
	for {set i 0} {$i<$dns_count} {incr i} {
		set line [lindex $dns $i]
		if {$line != " " && $line != ""} {
			log "Adding dns $line"
			set R "${R} nameserver $line"
		}
	}
	
	log "$::RESOLV_CONF contents : $R"

	if {[file exists /sbin/resolvconf] == 1} {
		log "using /sbin/resolvconf"
		exec echo -n "$R" | resolvconf -a "${iface}.udhcpc" 
	} else {
		log "writing $::RESOLV_CONF"
		exec echo -n "$R" > "$::RESOLV_CONF"
	}
	
	update_web_ip $ip_addr
	
	call_generic_event_script
	restart_upnpd
}


##################################################################
# Send webserver notification of configuration changes
##################################################################
proc web_notify_params_changed {} {
	# Setup the /tmp/params_changed.conf file with the names of parameters that the web should reload
	
	# Setup the /tmp_params_changed.ready file for the web param reload.
}

#######################################################################################
# Wait for Init is done; Needs for synchronize init and dhcp writing into the mnt/jffs
#######################################################################################
proc wait_for_init_done {} {
	for {set i 0} {(![file exists /tmp/init_done]) && ($i != 300)} {incr i} {
			#exec echo "DHCP wait for init_done flag - $i" > /dev/console
			sleep 1
	}
}

##################################################################
# Turn off auto ap mode
##################################################################
proc auto_ap_turn_off {} {

	wait_for_init_done
	
	catch {[exec cat /mnt/jffs2/sys.conf | grep -v auto_network_type > /tmp/t_sys.conf]}
	catch {exec echo "auto_network_type = 0" >> /tmp/t_sys.conf}
	catch {exec mv /tmp/t_sys.conf /mnt/jffs2/sys.conf}
	catch {exec cp /mnt/jffs2/sys.conf /tmp/sys.conf}
	
	exec echo "Turn OFF - AUTO AP MODE" > /dev/console
	
	exec config_umount.sh
	exec config_mount.sh
}

##################################################################
# Return the current IP address
##################################################################
proc get_current_ip {} {
	set ifconfig [exec ifconfig br0]
	set res [regexp {addr:([0-9]+)\.([0-9]+)\.([0-9]+)\.([0-9]+)} $ifconfig dummy p1 p2 p3 p4]
	if {$res==1} {
		#log "get_current_ip=$p1.$p2.$p3.$p4"
		return "$p1.$p2.$p3.$p4"
	}
	return ""
}

# Add for help to detect wireless link status by Ricky Cao on 2008/09/18
proc advance_retry_count {} {
	if {[file exists /tmp/dhcp_client_retry_count] == 1} {
		set retry_count [exec cat /tmp/dhcp_client_retry_count]
		incr retry_count
		# catch {exec echo "Retry count = $retry_count" > /dev/console}
		catch {exec echo $retry_count > /tmp/dhcp_client_retry_count}
	} else {
		# catch {exec echo can not found /tmp/dhcp_client_retry_count, create it with 1 > /dev/console}
		catch {exec echo 1 > /tmp/dhcp_client_retry_count}
	}
}

if {[info exists ::argv]} {
	# Jacky.Yang 15-Jun-2009, save original ip address
	set origIP [get_current_ip]

	# Parse command line and check arguments
	set action ""
	foreach argv $::argv {

		# Check if startup
		if {$argv == "startup"} {
			set action "startup"
		}

		# Check if kill udhcpc
		# Add by Ricky Cao on 2008/09/18
		# Because udhcpc doesn't renew automatically after change to connect to another AP
		# So we need kill command to kill it, then startup it again and device will get IP from DHCP server
		if {$argv == "kill"} {
			set action "kill"
		}

		# Check for udhcpc successful events
		if {$argv == "bound" || $argv == "renew" || $argv == "ok"} {
			log "udhcpc event : $argv"
			# Add for query dhcp client ip lease status by Ricky Cao on 2008/09/18
			# This is for detect real link status
			# If device is DHCP client and get on bind status, then the wlan link status is UP
			# catch {exec echo ----->$argv > /dev/console}
			catch {exec echo bind > /tmp/dhcp_client_lease_status}
			set action "ok"
		}
	
		# Check for udhcpc error events
		if {$argv == "leasefail" || $argv == "deconfig" || $argv == "nak" || $argv == "fail"} {
			log "udhcpc event : $argv"
			# Add for query dhcp client ip lease status by Ricky Cao on 2008/09/18
			# This is for detect real link status
			# If device is DHCP client and get on unbind status, then the wlan link status is DOWN
			# catch {exec echo ----->$argv > /dev/console}
			catch {exec echo unbind > /tmp/dhcp_client_lease_status}
			
			if {$argv == "leasefail"} {
				advance_retry_count
			}
			set action "fail"
		}

		# Check if debug is enabled
		if {$argv == "debug"} {
			set LOGGING 1
		}

		# Check if noreboot is enabled
		if {$argv == "noreboot"} {
			set REBOOT 0
		}
	}

	#
	# Kill the udhcpc (DHCP client)
	#
	if {[string compare $action "kill"] == 0} {
		# Kill the udhcpc
		# Rebised by Ricky Cao on 2008/09/18
		catch {exec echo 0 > /tmp/dhcp_client_retry_count}
		catch {exec killall udhcpc}
		exit_function
	}

	#
	# start udhcpc (DHCP client)
	#
	if {[string compare $action "startup"] == 0} {
		init_log
		log "action==startup"

		regexp {^.* = (.*)} [exec cat /mnt/jffs2/sys.conf | grep ip_lan] dummy current_ip
		#Jacky.Yang 21-Nov-2008, start - For GUI redirect to correct ip address when we renew ip.
		regexp {^.* = (.*)} [exec cat /mnt/jffs2/sys.conf | grep subnet_lan] dummy current_netmask
		catch {exec echo WET610N will roll-back to default IP and netmask setting, IP: $current_ip, netmask: $current_netmask > /dev/console}
		catch {exec echo $current_ip,$current_netmask > /var/Get_IPInfo_From_DHCP}

		if {[catch {open /var/webCommit r} checkCommit ]} {
			update_web_ip $current_ip
			call_generic_event_script
		} else {
			close $checkCommit
		}
		catch {exec rm -f /var/Get_IPInfo_From_DHCP}
		#Jacky.Yang 21-Nov-2008, End - For GUI redirect to correct ip address when we renew ip.
		#update_web_ip $current_ip
		#call_generic_event_script

		if {$ip_config != 0  || [get_parameter network_mode] == "2"} {
			# if not in auto or already an AP, cancel the auto_ap mode.
			if {[get_parameter auto_network_mode] == 1} {
				log "Disabling auto_network_mode"
				#exec echo "Turn OFF - AUTO AP MODE - Status StartUp" > /dev/console
				auto_ap_turn_off
			}
		} else {
			# start the udhcpc
			log "Starting udhcpc daemon"
			set hostName ""
			catch { set hostName [exec cat /etc/hostname]}
			if {[file exists /etc/udhcpc/dhcp.tcl]} {
				exec udhcpc -i br0 -H $hostName -s /etc/udhcpc/dhcp.tcl -b &
			} else {
				exec udhcpc -i br0 -H $hostName -s ${g_web_combined_ver_folder}/etc/udhcpc/dhcp.tcl -b &
			}
			log "After starting udhcpc daemon"
		}

		log "End of startup handler"
		exit_function
	}
	
	#
	# DHCP Bound/Renew (successfuly got IP)
	#
	if {[string compare $action "ok"] == 0} {
		set reboot_required 0

		# Notify the web with the parameters which were changed, for web reload.
		#log "Notifying web params changed"
		#web_notify_params_changed 

		set currentip [get_current_ip]
		set newip [get_env_var "ip"]
		log "current IP:$currentip / new IP:$newip / auto_ap: [get_auto_ap] "
		catch {exec echo dhcp.tcl: current IP:$currentip / new IP:$newip / auto_ap: [get_auto_ap] > /dev/console}

		# if Auto_AP is FALSE just update the IP address
		if {[get_auto_ap] == "0" } {
			catch {exec echo dhcp.tcl: newip=$newip, currentip=$currentip > /dev/console}
			if {$newip != $currentip} {			
				log "configuring nic to new IP:$newip"
				# Set the new IP Address
				#Jacky.Yang 21-Nov-2008, start - For GUI redirect to correct ip address when we renew ip.
				catch {exec echo WET610N has success get IP and netmask: [get_env_var "ip"], [get_env_var "subnet"] > /dev/console}
				catch {exec echo origIP: $origIP > /dev/console}
				if {[get_env_var "ip"] != ""} {
					catch {exec echo [get_env_var "ip"],[get_env_var "subnet"] > /var/Get_IPInfo_From_DHCP}
					catch {exec echo write ip and netmask [get_env_var "ip"],[get_env_var "subnet"] to /var/Get_IPInfo_From_DHCP > /dev/console}
				}

				catch {exec echo renewWaitTimer=$renewWaitTimer > /dev/console}

				if {[catch {open /var/webCommit r} checkCommit ]} {
				} else {
					close $checkCommit
					for {set loopCount 0} {$loopCount<$renewWaitTimer} {incr loopCount 1} {
						catch {exec echo Renew: waitting timer $loopCount > /dev/console}
						sleep 1
						if {[catch {open /var/success_get_ip r} fileId ]} {
						} else {
							close $fileId
							if {[catch {open /var/assign_get_ip r} checkAssignIP ]} {
							} else {
								close $checkAssignIP
								set loopCount $renewWaitTimer
							}
						}
					}
					#sleep 5
				}
				#sleep 3 #hnap will fial, we can't sleep.
				catch {exec echo start to setting ip and mask. > /dev/console}
				config_nic [get_env_var "interface"] [get_env_var "ip"] [get_env_var "subnet"] [get_env_var "hostname"] [get_env_var "domain"] [get_env_var "broadcast"] [get_env_var "router"] [get_env_var "dns"]
				if {[catch {open /var/webCommit r} checkCommit ]} {
					catch {exec echo dhcp.tcl: (newip != currentip) not write [get_env_var "ip"] to /var/currentIP file > /dev/console}
				} else {
					close $checkCommit
					#catch {exec /root/mtlk/etc/force_ethpc_renew_ip > /dev/console &}
				}
				
				catch {exec /root/mtlk/web/gui_wps_init.sh stop}
				catch {exec echo dhcp.tcl: (newip != currentip) write [get_env_var "ip"] to /var/currentIP file > /dev/console}
				catch {exec echo [get_env_var "ip"] > /var/currentIP}
				#Jacky.Yang 21-Nov-2008, End - For GUI redirect to correct ip address when we renew ip.
			} else {
				if {[catch {open /var/webCommit r} checkCommit ]} {
					catch {exec echo dhcp.tcl: (newip == currentip) not write [get_env_var "ip"] to /var/currentIP file > /dev/console}
				} else {
					close $checkCommit
					#Jacky.Yang 4-Dec-2008, Start - For GUI redirect. renew ip and not change.
					catch {exec echo dhcp.tcl: we didn't get new ip address. Wait 5 seconds and delete all GUI temp files. > /dev/console}
					catch {exec echo WET610N has success get IP and netmask: [get_env_var "ip"], [get_env_var "subnet"] > /dev/console}
					if {[get_env_var "ip"] != ""} {
						catch {exec echo [get_env_var "ip"],[get_env_var "subnet"] > /var/Get_IPInfo_From_DHCP}
						catch {exec echo write ip and netmask [get_env_var "ip"],[get_env_var "subnet"] to /var/Get_IPInfo_From_DHCP > /dev/console}
					}
					
					catch {exec /root/mtlk/web/gui_wps_init.sh stop}
					catch {exec echo dhcp.tcl: (newip == currentip) write [get_env_var "ip"] to /var/currentIP file > /dev/console}
					catch {exec echo [get_env_var "ip"] > /var/currentIP}
					#Jacky.Yang 5-Dec-2008, End - For GUI redirect. renew ip and not change.
				}
				log "IP has not changed."
			}
			
			# Terminate event handling
			#catch {exec rm -f /var/gui_wps_waiting}
			catch {exec rm -f /var/webCommit}
			catch {exec /root/mtlk/etc/umedia_kill_process.sh dhcp_timer.sh}
			exit_function
		}

		if {$wlan_link_up == 1 || [get_parameter network_type] == "2"} {
			# if Wireless link is UP or already an AP (ERROR) : use bcl_util to load auto_ap_turn_off.conf
			log "Disabling auto_network_mode"
			
			#exec echo "Turn OFF - AUTO AP MODE - Status OK" > /dev/console
			
			auto_ap_turn_off
			if {$newip != $currentip} {			
				log "configuring nic to new IP:$newip"
				# Set the new IP Address
				catch {exec echo dhcp.tcl: Set the new IP Address > /dev/console}
				config_nic [get_env_var "interface"] [get_env_var "ip"] [get_env_var "subnet"] [get_env_var "hostname"] [get_env_var "domain"] [get_env_var "broadcast"] [get_env_var "router"] [get_env_var "dns"]
			} else {
				log "IP has not changed."
			}
		} else {
			# if (Wireless link is Down : use mtlk_restore_defaults.sh to load default_ap.conf auto_ap_turn_off.conf)
			log "Converting to AP"
			set AP 1
			
			wait_for_init_done	
			
			exec ${g_web_wpa_app_folder}/mtlk_restore_defaults.sh $AP 
			
			#go to the reboot
			exec killall -SIGINT upnpd
			exec reboot
					
			# Terminate event handling
			exit_function			
		}
	}
	
	#
	# DHCP NAK/Lease FAIL/DeConfig (No IP obtained)
	#
	if {[string compare $action "fail"] == 0} {
		# Jacky.Yang 1-Dec-2008, kill ethernet down/up, if dhcp renew success, we will call it again.
		if {[catch {open /var/webCommit r} checkCommit ]} {
		} else {
			close $checkCommit
			#catch {exec killall -SIGUSR1 force_ethpc_renew_ip}
		}

		set config_changed 0
		set currentip [get_current_ip]
		set static_ip [get_parameter ip_lan]
		log "current IP:$currentip / static IP:$static_ip / ip_config:$ip_config"

		# On failure, don't replace IP if it was already set
		if { $currentip != "" } {
			log "DHCP failed. Keeping current IP: $currentip"
			exit_function
		}

		# if IP_config == Auto : Configure bridge with static info
		if {$ip_config == 0} {
			if { $currentip != $static_ip } {
				log "Configuring $interface to [get_parameter ip_lan]:[get_parameter subnet_lan]"
				set tmp_dns "[get_parameter primary_dns] [get_parameter secondary_dns]"
			
				
				# Configure bridge with static IP information
				# Jacky.Yang 1-Dec-2008, If GUI run wps, don't change ip to default.
				if {[catch {open /var/webCommit r} checkCommit ]} {
					catch {exec echo "We can't find /var/webCommit file, appy [get_parameter ip_lan] [get_parameter subnet_lan]"  > /dev/console}
					config_nic $interface [get_parameter ip_lan] [get_parameter subnet_lan] "" "" "" "" $tmp_dns
				} else {
					close $checkCommit
					catch { set originalIP [exec cat /var/currentIP] }
					catch {exec echo "dhcp.tcl: fail, set to original ip $originalIP"  > /dev/console}
					config_nic $interface $originalIP [get_env_var "subnet"]  "" "" "" "" ""
				}
				
				set config_changed 1
			} else {
				log "Nothing has changed."
			}
		} else {
		
			if {$currentip != "" && $currentip != "0.0.0.0"} {
				# Reset bridge IP Assignment
				catch {exec echo "dhcp.tcl: Reset bridge IP Assignment"  > /dev/console}
				config_nic $interface "0.0.0.0" "0.0.0.0" "" "" "" "" ""
				set config_changed 1
			}
		}
	
		if {$config_changed == 1} {
			# Notify the web with the parameters which were changed, for web reload.
			#log "Notifying web params changed"
			#web_notify_params_changed 
		}
		
		# exit	
		log "Done"
		catch {exec rm -f /var/webCommit}
		exit_function
	}
	
	log "unknown action $action . Aborting"
}
