#!/bin/tclsh



##################################################################
# This function gets the status of current security and save its 
# parameters accordingly. 
# 1 - Open System
# 2	- WEP SEcurity
# 3 - WPA Personal Security
# 4 - WPA Enterprise Security
##################################################################

# if noMount exists and is not empty, dont perform mount after umount.
# This is incase we come from commit.asp and a reboot will be performed after.

proc save_security {{noMount ""}} {

	# Reading all the needed params.
	set security_mode					[get_parameter NonProcSecurityMode]
	set device_type                     [get_parameter network_type]
	set ESSID                           [get_parameter NonProc_ESSID]
	
	# $device_type == 2 => AP	
	if {$device_type == 2}  {
		set WEP_Authentication          [get_parameter Authentication]
	} 

	debug "Security mode $security_mode"
	# Calling to appropriate function.
	
	#+++ In HNAP spec, 
	#+++ A blank SSID may be passed to indicate that the device should disconnect from any wireless network.
	#+++ So, if user configured device with NULL SSID by HNAP, then, forceWlanDisconnect will be configured to 1.
	#+++ forceWlanDisconnect will be cleared once user configured from GUI or configure None-NULL SSID by HNAP.
	#+++ Ricky Cao
	set forceWlanDisconnect [get_parameter "forceWlanDisconnect"]
	if {$forceWlanDisconnect == 1} {
		exec echo forceWlanDisconnect=1, so clear configuration of wpa_supplicant0.conf > /dev/console
		exec echo > /mnt/jffs2/wpa_supplicant0.conf
		return;
	}
	#--- Ricky Cao

	if {$security_mode == 1 } {
		# security_mode = open
		Activate_Open_System $device_type $ESSID 
		 
	} elseif {$security_mode == 2 } {
		# security_mode = WEP
		set WEP_Length                      [get_parameter NonProc_WepKeyLength]
		set WEP_Used_Key                    [get_parameter WepTxKeyIdx]
		set WEP_Authentication       		[get_parameter NonProc_Authentication]
		set WEP_Key1                        [get_parameter WepKeys_DefaultKey0]
		set WEP_Key2                        [get_parameter WepKeys_DefaultKey1]
		set WEP_Key3                        [get_parameter WepKeys_DefaultKey2]
		set WEP_Key4                        [get_parameter WepKeys_DefaultKey3]
		
		Activate_WEP $device_type $ESSID $WEP_Length $WEP_Authentication $WEP_Used_Key  $WEP_Key1 $WEP_Key2 $WEP_Key3 $WEP_Key4
		
	} elseif {$security_mode == 3 } {
		set WPA_Personal_Mode               [get_parameter NonProc_WPA_Personal_Mode]
		set WPA_Personal_Encapsulation      [get_parameter NonProc_WPA_Personal_Encapsulation]
		set WPA_Personal_PSK                [get_parameter NonProc_WPA_Personal_PSK]
		Activate_Personal_WPA $device_type $ESSID $WPA_Personal_Mode $WPA_Personal_Encapsulation $WPA_Personal_PSK
		
	} elseif {$security_mode == 4 } {
		 # security_mode = WPA/WPA2 Enterprise
		set WPA_Enterpride_Mode             [get_parameter NonProc_WPA_Enterprise_Mode]
		set WPA_Enterpride_Encapsulation    [get_parameter NonProc_WPA_Enterprise_Encapsulation]
		set WPA_Enterpride_Radius_IP        [get_parameter NonProc_WPA_Enterprise_Radius_IP]
		set WPA_Enterpride_Radius_Port      [get_parameter NonProc_WPA_Enterprise_Radius_Port]
		set WPA_Enterpride_Radius_Key       [get_parameter NonProc_WPA_Enterprise_Radius_Key]
		set WPA_Enterpride_Radius_Rekey_Int [get_parameter NonProc_WPA_Enterprise_Radius_ReKey_Interval]
		set WPA_Enterpride_Radius_User      [get_parameter NonProc_WPA_Enterprise_Radius_Username]
		set WPA_Enterpride_Radius_Pass      [get_parameter NonProc_WPA_Enterprise_Radius_Password]
		set WPA_Enterpride_Certificate      [get_parameter NonProc_WPA_Enterprise_Certificate]	 
		Activate_Enterprise_WPA $device_type $ESSID $WPA_Enterpride_Mode $WPA_Enterpride_Encapsulation $WPA_Enterpride_Radius_IP $WPA_Enterpride_Radius_Port $WPA_Enterpride_Radius_Key $WPA_Enterpride_Radius_Rekey_Int $WPA_Enterpride_Radius_User $WPA_Enterpride_Radius_Pass $WPA_Enterpride_Certificate
	} else {
		# security_mode = WEP RADIUS
		set WEP_RADIUS_Length               [get_parameter NonProc_WepRadiusKeyLength]
		set WEP_RADIUS_Used_Key             [get_parameter WepRadiusTxKeyIdx]
		set WEP_RADIUS_Authentication  		[get_parameter WepRadiusAuthentication]
		set WEP_RADIUS_Key1                 [get_parameter WepRadiusKeys_DefaultKey0]
		set WEP_RADIUS_Key2                 [get_parameter WepRadiusKeys_DefaultKey1]
		set WEP_RADIUS_Key3                 [get_parameter WepRadiusKeys_DefaultKey2]
		set WEP_RADIUS_Key4                 [get_parameter WepRadiusKeys_DefaultKey3]
		set WEP_RADIUS_Key                  [get_parameter NonProc_WEP_Radius_Key]
		set WEP_RADIUS_Port                 [get_parameter NonProc_WEP_Radius_Port]
		set WEP_RADIUS_IP                   [get_parameter NonProc_WEP_Radius_IP]
		
		Activate_WEP_RADIUS $device_type $ESSID $WEP_RADIUS_Length $WEP_RADIUS_Authentication $WEP_RADIUS_Used_Key  $WEP_RADIUS_Key1 $WEP_RADIUS_Key2 $WEP_RADIUS_Key3 $WEP_RADIUS_Key4 $WEP_RADIUS_Key $WEP_RADIUS_Port $WEP_RADIUS_IP
	}
	
	# Save the settings to flash - needed on platforms without jffs2 fs
	if {[catch {set ret [exec which config_umount.sh]}] == 0 && $ret != "" } {

		exec config_umount.sh

		# if $noMount is not empty dont perform mount
		if {$noMount == ""} {
			exec config_mount.sh
		}				
	}
}

##################################################################
# Updating all wlan.conf and hosatapd/wpa_supplicant fields 
# according to open system
##################################################################
proc Activate_Open_System {device_type SSID} {
	debug "Activate_Open_System '$device_type' '$SSID'"
	
	if {$device_type == 2}  {
		set proc_err [set_proc_var 0 Authentication]
	}
		
	# Write to the proc for on-the-fly connection
	set proc_err [set_proc_var 0 WepEncryption]
		
	# For station
	if {$device_type == 0 || $device_type == 1}  {
		Update_Supplicant_Conf "OPEN" $SSID "" "" "" "" "" "" "Open_System" "" "" "" "" "" 
	}
		
	# For AP
	if {$device_type == 2}  {
		Update_Hostapd_Conf "OPEN" $SSID "" "" "" "" "" "" "" "" "" "" "" "" "" ""
	}
}

##################################################################
# Updating all wlan.conf and hosatapd/wpa_supplicant fields 
# according to WEP secured system
##################################################################
proc Activate_WEP {device_type SSID WEP_length authentication Used_Key key1 key2 key3 key4} {
	debug "Activate_WEP '$device_type' '$SSID'  $WEP_length $authentication $Used_Key $key1 $key2 $key3 $key4"

	#
	# Write to the proc for on-the-fly connection
	set proc_err [set_proc_var 1 WepEncryption]
	if {$device_type == 2}  {
		set proc_err [set_proc_var $authentication Authentication]
	}
	
	# For station
	if {$device_type == 0 || $device_type == 1}  {
		Update_Supplicant_Conf "WEP" $SSID $authentication $key1 $key2 $key3 $key4 $Used_Key "Wep_System" "" "" "" "" ""
	}
	# For AP
	if {$device_type == 2}  {
		Update_Hostapd_Conf "WEP" $SSID $WEP_length $authentication $Used_Key $key1 $key2 $key3 $key4 "" "" "" "" "" "" ""
	}
}

##################################################################
# Updating all wlan.conf and hosatapd/wpa_supplicant fields 
# according to WEP RADIUS secured system
##################################################################
proc Activate_WEP_RADIUS {device_type SSID WEP_length authentication Used_Key key1 key2 key3 key4 radiusKey radiusPort radiusIP} {
	debug "Activate_WEP_RADIUS '$device_type' '$SSID'  $WEP_length $authentication $Used_Key $key1 $key2 $key3 $key4 $radiusKey $radiusPort $radiusIP"

	#
	# Write to the proc for on-the-fly connection
	set proc_err [set_proc_var 1 WepEncryption]
	if {$device_type == 2}  {
		set proc_err [set_proc_var $authentication Authentication]
	}
	
	# For station
	#if {$device_type == 0 || $device_type == 1}  {
	#	Update_Supplicant_Conf "WEP" $SSID $authentication $key1 $key2 $key3 $key4 $Used_Key "Wep_System" "" "" "" "" ""
	#}
	# For AP
	if {$device_type == 2}  {
		Update_Hostapd_Conf "WEP RADIUS" $SSID $WEP_length $authentication $Used_Key $key1 $key2 $key3 $key4 "" "" "" $radiusIP $radiusPort $radiusKey ""
	}
}

##################################################################
# Updating all wlan.conf and hosatapd/wpa_supplicant fields 
# according to WPA personal secured system
##################################################################
proc Activate_Personal_WPA {device_type SSID WPA_mode encapsulation_mode psk} {
	debug "Activate_Personal_WPA '$device_type' '$SSID'  $WPA_mode $encapsulation_mode $psk"

	#
	# Write to the proc for on-the-fly connection
	set proc_err [set_proc_var 0 WepEncryption]
	
	if {$device_type == 2}  {
		set proc_err [set_proc_var 0 Authentication]
	}

	
	# For station
	if {$device_type == 0 || $device_type == 1}  {
		Update_Supplicant_Conf "WPA_PERSONAL" $SSID "" "" "" "" "" "" $psk "" "" "" $WPA_mode $encapsulation_mode
	}
	# For AP
	if {$device_type == 2}  {
		Update_Hostapd_Conf "WPA_PERSONAL" $SSID "" "" "" "" "" "" "" $WPA_mode $encapsulation_mode $psk "" "" "" ""
	}
}
##################################################################
# Updating all wlan.conf and hosatapd/wpa_supplicant fields 
# according to WPA enterprise secured system
##################################################################
proc Activate_Enterprise_WPA {device_type SSID WPA_mode encapsulation_mode IP port server_key interval username user_password certificate} {
	debug "Activate_Enterprise_WPA '$device_type' '$SSID'  $WPA_mode $encapsulation_mode $IP $port $server_key $interval $username $user_password $certificate"
	
	#
	# Write to the proc for on-the-fly connection
	set proc_err [set_proc_var 0 WepEncryption]

	if {$device_type == 2}  {
		set proc_err [set_proc_var 0 Authentication]
	}
		
	# For station
	if {$device_type == 0}  {
		Update_Supplicant_Conf "WPA_ENTERPRISE" $SSID "" "" "" "" "" "" "Enterprise" $username $user_password $certificate "" ""
	} 
	# For AP
	if {$device_type == 2}  {
		Update_Hostapd_Conf "WPA_ENTERPRISE" $SSID "" "" "" "" "" "" "" $WPA_mode $encapsulation_mode "" $IP $port $server_key $interval  
	}
}

##################################################################
# This function writes to /mnt/jffs2/etc/hostapd.conf file
# accoding to the encryption mode.
##################################################################
proc Update_Hostapd_Conf { encryption_mode SSID WEP_length authentication Used_Key WEP_Key1 WEP_Key2 WEP_Key3 WEP_Key4 WPA_mode encapsulation_mode psk RADIUS_IP RADIUS_port RADIUS_key RADIUS_interval} {
	global hostapd_conf_file
	global wlan_index
	debug "Update_Hostapd_Conf '$encryption_mode' '$SSID'  $WEP_length $authentication $Used_Key $WEP_Key1 $WEP_Key2 $WEP_Key3 $WEP_Key4 $WPA_mode $encapsulation_mode $psk $RADIUS_IP $RADIUS_port $RADIUS_key $RADIUS_interval"
	
	exec echo "logger_syslog_level=3"  >  $hostapd_conf_file
	exec echo "interface=wlan$wlan_index"  >>  $hostapd_conf_file
	exec echo "driver=mtlk"      >> $hostapd_conf_file
	exec echo "ssid=$SSID"       >> $hostapd_conf_file
	exec echo "macaddr_acl=0"    >> $hostapd_conf_file

	# find the wireless card count
	set card_count 1
	catch {[set card_count [exec lspci | grep Wireless | wc -l]]}
	set card_count [string trim $card_count]
	set bridging [get_parameter BridgeMode]
	# in case of dual band, in bridge mode set br0 in both cards. 
	if {$bridging > 0 } {
		exec echo "bridge=br0"   >> $hostapd_conf_file
	} else {
		# in route mode set br1 in both cards only if there are two cards.
		if {$card_count>1} {
			exec echo "bridge=br1"   >> $hostapd_conf_file
			}
	}	
	

	if {$encryption_mode == "OPEN"} {
		exec echo "# ------AP is in Plain Text mode------"     >> $hostapd_conf_file
		return
	}

	if {$encryption_mode == "WEP"} {
		exec echo "# ------AP is in WEP mode------"     >> $hostapd_conf_file
		return
	}
	
	if {$encryption_mode == "WEP RADIUS"} {
		
		set RADIUS_port2 [expr $RADIUS_port + 1]
		exec echo "auth_server_addr=$RADIUS_IP"                    >> $hostapd_conf_file
		exec echo "auth_server_port=$RADIUS_port"                  >> $hostapd_conf_file
		exec echo "auth_server_shared_secret=$RADIUS_key"          >> $hostapd_conf_file
		exec echo "acct_server_addr=$RADIUS_IP"                    >> $hostapd_conf_file
		exec echo "acct_server_port=$RADIUS_port2"       		   >> $hostapd_conf_file
		exec echo "acct_server_shared_secret=$RADIUS_key"          >> $hostapd_conf_file
		
		exec echo "# ------AP is in WEP RADIUS mode------"     >> $hostapd_conf_file
		return
	}

	if {$encryption_mode == "WPA_PERSONAL"} {
	
		set groupRekey [get_parameter wpa_group_rekey]
	
		exec echo "wpa=$WPA_mode"          		>> $hostapd_conf_file
		exec echo "wpa_key_mgmt=WPA-PSK"   		>> $hostapd_conf_file
		exec echo "wpa_group_rekey=$groupRekey" >> $hostapd_conf_file
		exec echo "wpa_gmk_rekey=86400"    		>> $hostapd_conf_file

		if {$encapsulation_mode == 0} {
			exec echo "wpa_pairwise=TKIP" >> $hostapd_conf_file
			set encapsulation "TKIP"
		} else {
			if {$encapsulation_mode == 1} {
				exec echo "wpa_pairwise=CCMP" >> $hostapd_conf_file
				set encapsulation "CCMP"
			} else {
				exec echo "wpa_pairwise=TKIP CCMP" >> $hostapd_conf_file
				set encapsulation "TKIP + CCMP"
			}
		}
		
		if {[string length $psk] == 64} {
			exec echo "wpa_psk=$psk" >> $hostapd_conf_file
		} else {		
			exec echo "wpa_passphrase=$psk" >> $hostapd_conf_file
		}
		
		exec echo "# ------AP is in WPA/WPA2 PSK, $encapsulation mode------"     >> $hostapd_conf_file
	}
	if {$encryption_mode == "WPA_ENTERPRISE"} {

		set RADIUS_port2 [expr $RADIUS_port + 1]
		exec echo "ieee8021x=1"	    	   			               >> $hostapd_conf_file
		exec echo "auth_server_addr=$RADIUS_IP"                    >> $hostapd_conf_file
		exec echo "auth_server_port=$RADIUS_port"                  >> $hostapd_conf_file
		exec echo "auth_server_shared_secret=$RADIUS_key"          >> $hostapd_conf_file
		exec echo "acct_server_addr=$RADIUS_IP"                    >> $hostapd_conf_file
		exec echo "acct_server_port=$RADIUS_port2"       		   >> $hostapd_conf_file
		exec echo "acct_server_shared_secret=$RADIUS_key"          >> $hostapd_conf_file
		exec echo "wpa=$WPA_mode"         						   >> $hostapd_conf_file
		exec echo "wpa_key_mgmt=WPA-EAP"   					       >> $hostapd_conf_file
		exec echo "eap_reauth_period=$RADIUS_interval"             >> $hostapd_conf_file

		if {$encapsulation_mode == 0} {
			exec echo "wpa_pairwise=TKIP" >> $hostapd_conf_file
			set encapsulation "TKIP"
		} else {
			if {$encapsulation_mode == 1} {
				exec echo "wpa_pairwise=CCMP" >> $hostapd_conf_file
				set encapsulation "CCMP"
			} else {
				exec echo "wpa_pairwise=TKIP CCMP" >> $hostapd_conf_file
				set encapsulation "TKIP + CCMP"
			}
		}
		exec echo "# ------AP is in WPA/WPA2 Enterprise, $encapsulation mode------"     >> $hostapd_conf_file
	}
}
##################################################################
# This function writes to /mnt/jffs2/etc/wpa_supplicant.conf file
# accoding to the encryption mode.
##################################################################
proc Update_Supplicant_Conf { encryption_mode SSID WEP_Auth WEP_Key1 WEP_Key2 WEP_Key3 WEP_Key4 WEP_Used_Key WPA_Password username user_password certificate WPA_mode encapsulation_mode } {

	global wpa_supplicant_conf_file
	global wpa_passphrase
	global certificate_file

	debug "Update_Supplicant_Conf '$encryption_mode' '$SSID' $WEP_Auth $WEP_Key1 $WEP_Key2 $WEP_Key3 $WEP_Key4 $WEP_Used_Key $WPA_Password $username $user_password $certificate $WPA_mode $encapsulation_mode"

    catch { exec echo "ctrl_interface=/var/run/wpa_supplicant" > $wpa_supplicant_conf_file }
    catch { exec echo "ctrl_interface_group=0" >> $wpa_supplicant_conf_file }
	
	if {[string length $WPA_Password] == 64} {
		catch { exec echo "network=\{" >> $wpa_supplicant_conf_file }
		catch { exec echo "ssid=\"$SSID\"" >> $wpa_supplicant_conf_file }
		catch { exec echo "#psk=\"$WPA_Password\"" >> $wpa_supplicant_conf_file }
		catch { exec echo "psk=$WPA_Password" >> $wpa_supplicant_conf_file }
		catch { exec echo "\}" >> $wpa_supplicant_conf_file }
	} else {
		#Fix can not correct generate wpa_supplicant configuration while ssid is <><>&&
		#Since '>' is reserve character, so it will casused error while it appear in SSID
		#Add double quotation marks to $SSID and call wpa_passphrase with shall script - Ricky CAO on Aug 17 2009
		if {[catch {exec /root/mtlk/web/run_wpa_passphrase_for_update_security.sh >> $wpa_supplicant_conf_file} err]} {
			puts "$err"
			return;
		}
	}
	
	set handler [open $wpa_supplicant_conf_file r+]
	while {[eof $handler] == 0} {
		set line [gets $handler]
		if {[string match "*psk*" $line] == 1} {
			set line [gets $handler]
			if {$encryption_mode == "OPEN"} {
				puts $handler "\tkey_mgmt=NONE"
				puts $handler "\tauth_alg=OPEN"				
				puts $handler "\tscan_ssid=1"	
				puts $handler "\}"
			}
			if {$encryption_mode == "WEP"} {
				puts $handler "\tkey_mgmt=NONE"
				# Writing all the 4 keys.
				for {set j 1} {$j <= 4} {incr j} {
					set WEP_Key WEP_Key$j
					set value_WEP_Key [set $WEP_Key]
					set hexa       [regexp "^0x" $value_WEP_Key]
					set minusOne [expr $j - 1]
					set Key_Conf wep_key$minusOne
					if {$hexa == 1} {
						set hex_WEP_Key [string range $value_WEP_Key 2 end]
						puts $handler "\t$Key_Conf=$hex_WEP_Key"
					} else {
						puts $handler "\t$Key_Conf=\"$value_WEP_Key\""
					}
				}
        		puts $handler "\twep_tx_keyidx=$WEP_Used_Key"
				puts $handler "\tgroup=WEP40 WEP104"				
				if {$WEP_Auth==1} {
					puts $handler "\tauth_alg=OPEN"				
				} elseif {$WEP_Auth==2} {
					puts $handler "\tauth_alg=SHARED"				
				} elseif {$WEP_Auth==3} {
					puts $handler "\tauth_alg=OPEN SHARED"				
				}
				puts $handler "\tscan_ssid=1"
				puts $handler "\}"
			}
			
			if {$encryption_mode == "WPA_PERSONAL"} {
				if {$WPA_mode == 3 && $encapsulation_mode == 2} {
					puts $handler "\tscan_ssid=1"
					puts $handler "\}"	
					break
				}
				if {$WPA_mode == 1} {
					puts $handler "\tproto=WPA"
				} elseif {$WPA_mode == 2} {
					puts $handler "\tproto=RSN"
				}
				if {$encapsulation_mode == 0} {
					puts $handler "\tpairwise=TKIP"
					puts $handler "\tgroup=TKIP"
				} elseif {$encapsulation_mode == 1} {
					puts $handler "\tpairwise=CCMP"
					#Jacky.Yang 28-Aug-2008, Fix security, WAP2-AES, Muticast:AES, Unicast:AES+TKIP, 
					# for support Linksys WPA2 Personal "AES" mode and "TKIP or AES" mode
					#puts $handler "\tgroup=CCMP"
					puts $handler "\tgroup=CCMP TKIP"
				} elseif {$encapsulation_mode == 2} {
					puts $handler "\tpairwise=CCMP TKIP"
					puts $handler "\tgroup=CCMP TKIP"
				}
				puts $handler "\tscan_ssid=1"
				puts $handler "\}"	
			}
				
			if {$encryption_mode == "WPA_ENTERPRISE"} {
				puts $handler "\tkey_mgmt=WPA-EAP"
				puts $handler "\teap=TTLS PEAP TLS"
 				#puts $handler "\tphase2=\"auth=MSCHAPV2 auth=GTC auth=MD5 auth=TLS auth=OTP auth=MSCHAP auth=PAP auth=CHAP\" \"autheap=MSCHAPV2  autheap=GTC  autheap=MD5  autheap=TLS  autheap=OTP\""
				puts $handler "\tphase2=\"auth=MSCHAPV2 auth=GTC\""
				if {$certificate == 1} {
					puts $handler "\tclient_cert=\"$certificate_file\""
				}
				puts $handler "\tidentity=\"$username\""
				puts $handler "\tpassword=\"$user_password\""
				puts $handler "\tauth_alg=OPEN"
				puts $handler "\tscan_ssid=1"
				puts $handler "\}"
			}
		}
	}
	close $handler
}

##################################################################
# Activating the process of hostapd/wpa_supplicant. This function is 
# called from the scan function. If the authentication succeeds, 
# the ssid and its password will be inserted to a specific file.
##################################################################
proc Activating_Hostapd_Supplicant {type} {
	global wpa_supplicant_conf_file
	global hostapd_conf_file
	global wpa_supplicant
	global hostapd

	if {$type == "AP"} {
		exec $hostapd -d $hostapd_conf_file > /dev/null 2>/dev/null &
	} else {
		set bridging [get_parameter BridgeMode]
		if {$bridging > 0 } {
			# Check for MAC Cloning
			if {$bridging == 3} {
				#get clone mac from /proc/.../MAC
				set mc [exec cat /tmp/mac_cloning.addr]
				# I don't know why, but with small letters suplicant dosn't work!!!!!!!!!!!!!!!
				set mac_clon_addr [string toupper $mc]
				exec $wpa_supplicant -Dwext -bbr0 -iwlan0 -c $wpa_supplicant_conf_file -p maclone=$mac_clon_addr > /dev/null 2>/dev/null &
			} else {
				exec $wpa_supplicant -Dwext -bbr0 -iwlan0 -c $wpa_supplicant_conf_file > /dev/null 2>/dev/null &
			}
		} else {
			exec $wpa_supplicant -Dwext -iwlan0 -c $wpa_supplicant_conf_file > /dev/null 2>/dev/null &
		}
	}

}



proc Reactivate_Hostapd_Supplicant {type} {
	global wpa_supplicant
	global hostapd
	
	#set application $wpa_supplicant
	# this was changed so we can see also the supplicant that wps is starting , and it may run it from somewere else
	set application "wpa_supplicant"

	if {$type == "AP"} {
		set application $hostapd 
	} 
	
	set ps ""
	catch { set ps [split [exec ps | grep $application] "\n"]}
	set count [llength $ps]
    # the last one is always the grep it self, so ignore it
	incr count -1
	
	if {$count < 1} {
		#puts "Can not locate $application.\n" 
		Activating_Hostapd_Supplicant $type
	} else {
		# if there is more than one supplicant, kill all the others
		for {set j 1} {$j < $count} {incr j} {
			set process_num [lindex [lindex $ps $j] 0]
			catch {exec kill -9 $process_num}
		}
		# send -HUP signal to the supplicant to reread the configuration file.
		set process_num [lindex [lindex $ps 0] 0]
		catch {exec kill -HUP $process_num}
		sleep 4 	
		
	}
}


