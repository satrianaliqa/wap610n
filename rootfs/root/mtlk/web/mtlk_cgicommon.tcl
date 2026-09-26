
#
# Global settings from a common config file
#
set g_web_tmp_folder /tmp
set g_web_wpa_config_folder /mnt/jffs2/etc
set g_web_wpa_app_folder /mnt/jffs2/etc
set g_web_config_folder /mnt/jffs2
set g_web_combined_ver_folder /mnt/jffs2
set g_web_scripts_folder /mnt/jffs2/scripts
set g_web_saved_configs_folder /mnt/jffs/saved_configs
set g_web_proc_root_folder /proc/sys/dev/mtlk/wlan
set g_web_virt_tmp_folder /tmp
set web_config_file "web_config.tcl"
set g_web_remote_mode 0
#
# If config file exists, load it to override the defaults.
#
if {[file exists $web_config_file]}  {
	source $web_config_file
}


##################################################################
# Set value to param_name in the proc file system
##################################################################
proc set_proc_var {value param_name} {
	return [expr ! [catch {exec $::g_web_wpa_app_folder/mtpriv [get_interface] $param_name $value}]	]
}
##################################################################
# Get value from param_name in the proc file system
##################################################################
proc get_proc_var {param_name} {
	if {[catch {set ret [exec $::g_web_wpa_app_folder/mtpriv [get_interface] $param_name]}]} {
		return ""
	}
	return $ret
}

##################################################################
# Check if the driver is loaded
##################################################################
proc is_driver_loaded {} {
	if {[file exists [get_proc_root]]} {
			return 1
		} 
	return 0
}

##################################################################
# Load a file and return the contents
##################################################################
proc file_load {filename} {
	set contents ""
	if {[catch {set contents [exec cat $filename] }]} {
		if {![catch {set fp [open "$filename" r]}]} {
			set contents [read $fp]	
			close $fp
		}
		
	}
	set contents [split $contents "\n"]
	return $contents	
}

##################################################################
# Print debug information (program arguments)
##################################################################
proc debug_print_args {} {
	if {[info exists ::argv]} {
		puts "Arguments:<BR>"
		foreach arg $::argv {
			puts "$arg<BR>"
		}
	}
}

##################################################################
# Print debug information (form query string)
##################################################################
proc debug_print_QUERYSTR {} {
	global QUERY_STR
	puts "QUERY_STR:<BR><i>$QUERY_STR</i><BR>"
}

##################################################################
# print debug information (application environment variables)
##################################################################
proc debug_print_env {} {
	set env_names [split [array names ::env] " "]
	puts "<BR><BR><b><i>Environment Vars</i></b><BR>"
	puts "<b>ENV</b><BR>"
	foreach name $env_names {
		puts "<i>$name = $::env($name)</i><BR>"
	}
	puts "<b>ENV FILE $::g_web_tmp_folder/web_env.tmp</b><BR>"
	set env_file [file_load "$::g_web_tmp_folder/web_env.tmp"]
	set i 0
	foreach env_var $env_file {
		if {[regexp {(^[^ ]+)=(.+)} $env_var dummy ename evalue]>0} {
			puts "$i:$env_var <i>$ename = $evalue</i><BR>"
			incr i
		}
	}
}

##################################################################
# Get the value of the environment variable 'name'
##################################################################
proc get_env_var {name {readFromFile ""}} {
	if {[string compare $readFromFile ""]==0 && [info exists ::env($name)]!=0} {
		return $::env($name)
	} else {
		set env_file [file_load $::g_web_tmp_folder/web_env.tmp]
		foreach env_var $env_file {
			if {[regexp {([^ ]+)=(.+)} $env_var dummy ename evalue]>0} {
				if {[string compare $ename $name]==0} {
					return $evalue
				}
			}
		}
	}
	return ""
}

proc get_interface {} {
	set card_index [get_env_var ASP_wlan_index]
	if {[string compare $card_index ""]==0} {
		set card_index 0
	}
	return "wlan$card_index"
}
##################################################################
# Set the value of the environment variable 'name'
##################################################################
proc set_env_var {name value} {	
	set $::env($name) $value	
}

##################################################################
# Get the proc file system root location
##################################################################
proc get_proc_root {} {
	set card_index [get_env_var ASP_wlan_index]
	if {[string compare $card_index ""]==0} {
		set card_index 0
	}
	return "/proc/sys/dev/mtlk/wlan${card_index}"
}

##################################################################
# Get the proc file system root location
##################################################################
proc get_debug_root {} {
	set card_index [get_env_var ASP_wlan_index]
	if {[string compare $card_index ""]==0} {
		set card_index 0
	}
	return "/proc/net/mtlk/wlan${card_index}"
}
##################################################################
# Check if option_name is on the command line
##################################################################
proc is_option_in_commandline {option_name} {
	foreach arg $::argv {
		if {[string first $option_name $arg] == 0} {
			return $arg
		}
	}
	return ""
}

##################################################################
#	Print a list of debug information items (env, args)
##################################################################
proc print_debug_info {} {
	debug_print_args
	debug_print_env
	debug_print_QUERYSTR
}



##################################################################
#	print a common HTML start paragraph, including the style
##################################################################
proc print_html_start {} {
	puts "content-type: text/html"
	puts ""
	puts ""
	puts "<html>\n\n<head>"
	puts "<meta http-equiv=\"Pragma\" content=\"no-cache\">"
	puts "<meta http-equiv=\"expires\" content=\"Mon, 22 Jul 1999 11:12:01 GMT\">"
	puts "<meta http-equiv=\"CACHE-CONTROL\" CONTENT=\"NO-CACHE\">"
	puts "<title></title>\n<link rel='stylesheet' href='/normal_ws.css' type='text/css'>\n</head>\n<body>\n"
}



##################################################################
#	print a common HTML footer
##################################################################
proc print_html_end {} {
	puts "</body>
</html>
"	
}

##################################################################
#	print a common HTML Comment start symbol
##################################################################
proc print_html_comment_start {} {
	puts "\n<!---\n"
	flush stdout
}

##################################################################
#	print a common HTML Comment ending symbol
##################################################################
proc print_html_comment_end {} {
	puts "\n--->\n"
	flush stdout
}

##################################################################
# Print standard HTML form star tag
##################################################################
proc print_form_start {form_name} {
	puts "<form action=$form_name  method=GET>"
}

##################################################################
# Print standard HTML form end tag
##################################################################
proc print_form_end {} {
	puts "</form>"
}

##################################################################
# Collect CGI information
##################################################################
proc collect_cgi_vars {} {
	global QUERY_STR
	global argv
	
	set QUERY_STR {}

	if [info exists ::env(REQUEST_METHOD)] {
		set request_method $::env(REQUEST_METHOD)
		if {[string compare $request_method "POST"] == 0} {
			if [info exists ::env(CONTENT_TYPE)] {
				set content_type $::env(CONTENT_TYPE)
				if {[string compare $content_type "application/x-www-form-urlencoded"] == 0} {
					if [info exists ::env(CONTENT_TYPE)] {
						set content_length $::env(CONTENT_LENGTH)
						set QUERY_STR [read stdin $content_length]
					}
				}
			}
		} elseif {[string compare $request_method "GET"] == 0} {
			set QUERY_STR $::env(QUERY_STRING)
		} else {
			foreach arg $argv {
				set QUERY_STR "$QUERY_STR&$arg"
			}
		}
	}
	#puts "'$QUERY_STR'"
}

##################################################################
# Decode a given url and remove %xx values 
##################################################################
proc urlDecode {url} {
    regsub -all {\+} $url { } url
    set svar ""
    set snew ""
    set urlPieces [split $url "%"]
    set url ""
    set len [llength $urlPieces]
    for {set i 0} {$i < $len} {incr i} {
        set urlPiece [lindex $urlPieces $i]
        if {$i>0} {
            set urlPiece "%${urlPiece}"
        }
        if {[regexp {%([0-9a-hA-H][0-9a-hA -H])} $urlPiece -> svar]>0} {
            set snew [format "%c" "0x$svar"]
            regsub {%([0-9a-hA-H][0-9a-hA -H])} $urlPiece $snew urlPiece
        }
        set url "${url}${urlPiece}"
    }
    return $url
}

##################################################################
# Get a CGI variable named var_name
##################################################################
proc get_cgi_var {var_name} {
	global QUERY_STR
	set QUERY_PARAMS [split $QUERY_STR "&"]
	foreach param $QUERY_PARAMS {
		set val [string range $param [expr [string length $var_name]+1] [string length $param]]
		if {[string first $var_name $param] == 0} {
			return $val
		}		
	}
	return ""
}

##################################################################
# call to redirect the client broser to a new page.
##################################################################
proc redirect_to {new_url} {
	puts "<script type=\"text/javascript\">"
	puts "<!--"
	puts "window.location = \"$new_url\""
	puts "//-->"
	puts "</script>"
}

##################################################################
# Print customized HTML header + scripts
##################################################################
proc print_html_head_and_scripts {{refresh 0}} {
	puts "content-type: text/html"
	puts ""
	puts ""
	puts "<html>\n<!- SD  ->\n"
	puts "<head>"
	if {$refresh>0} {
		puts "<meta http-equiv=\"refresh\" content=\"$refresh\" >"
	}
	puts "<meta http-equiv=\"Pragma\" content=\"no-cache\">"
	puts "<meta http-equiv=\"expires\" content=\"Mon, 22 Jul 1999 11:12:01 GMT\">"
	puts "<meta http-equiv=\"CACHE-CONTROL\" CONTENT=\"NO-CACHE\">"
	puts "<title></title>\n<link rel='stylesheet' href='/normal_ws.css' type='text/css'>\n"
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


##################################################################
# Set an ASP var from the TCL. set paramVal of paramName, and force
# immediate update by setting saveNow to 1 (will not appear in the commit
# page, and will be immediately saved to the wlan/sys config file.
##################################################################
proc set_asp_param {paramName paramVal {saveNow 0}} {
	set cgi_vars_file [get_env_var CGI_WEB_VAR_FILE]
	set open_method "w"
			
    if {[file exists $cgi_vars_file] == 1} {
		set open_method "a"
	}
	
	set fout [open $cgi_vars_file $open_method]
	
	#puts "'$open_method' '$cgi_vars_file' '$paramName' '$paramVal'";

	set saveNowText ""
	if {$saveNow==1} {
		set saveNowText "__COMMITNOW__"
	}
	puts $fout "${paramName}=${paramVal}${saveNowText}"
	close $fout  	

}

#
# LINUX SPECIFIC COMMANDS
#
proc mt_exec args {
   set command ""
   foreach arg $args {
      set command "$command $arg"
   }
	exec $command
}

proc mt_print_os_type {} {
	puts "LINUX<br>"
}

proc mt_get_file_lines {filename} {
	return [exec cat $filename]
}

proc mt_fwrite {filename line {appnd 1}} {
	set fp ""
	set op "a"
	if {$appnd!=1} {
		set op "w"
	} 
	set fp [open "$filename" $op]
	puts $fp $line
	close $fp
}

proc mt_get_card_count {} {
	set card_count 1
	catch {[set card_count [exec lspci | grep Wireless | wc -l]]}
	return $card_count
}

proc mt_get_passphrase {SSID WPA_Password filename} {
	if {[catch {exec $wpa_passphrase $SSID $WPA_Password > $wpa_supplicant_conf_file} err]} {
		return err
	}
	return ""
}
##################################################################
# Get the proc file system root location
##################################################################
proc get_proc_root {} {
	set card_index [get_env_var ASP_wlan_index]
	if {[string compare $card_index ""]==0} {
		set card_index 0
	}
	return "/proc/sys/dev/mtlk/wlan${card_index}"
}
#
# END OF LINUX SPECIFIC COMMANDS
#

set RUN_MENU 0

set env(PATH) "/sbin:/bin:/usr/sbin:/usr/bin:/mnt/jffs2/bin"

set g_cgi_vars_file [get_env_var CGI_WEB_VAR_FILE]
if {$g_cgi_vars_file!=""} {
	file delete $g_cgi_vars_file
}

# must call this at the begining of the CGI script, to collect the CGI vars.
collect_cgi_vars


set OS_TYPE [get_env_var OS_TYPE]
set g_web_remote_mode [get_env_var ASP_web_remote_mode]

if {$OS_TYPE == "WIN"} {
	source mtlk_wincommon.tcl
}

# Checks if device is connected to another device (if ap is connected to station or station is connected to ap)
proc isConnected {interface} {

	set status    ""	
	set connected 0
	
	catch {set status [exec cat $::g_web_tmp_folder/wls_link_stat]}	
	if {[regexp {WLSLinksStatus( *)=( *)(.*)} $status all sp1 sp2 value] == 1} {
		if {$value == 1} {
			set connected 1
		}
	}
	
	return $connected
}

