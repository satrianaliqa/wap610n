#!/bin/tclsh

source ini.tcl


set get_proc_root "/proc/sys/dev/mtlk/wlan0"


proc SendCommand {command {rdlim_file ""}} {

	global get_proc_root
	
	set len [format "%03d" [expr "4 + [string length $command]"]]
	set cmd "$len,$command"
	
	# send the command	
	#puts "*** echo $cmd > ${get_proc_root}/ConfigCommand"
	if {[catch {exec echo $cmd > ${get_proc_root}/ConfigCommand}]} {
		puts "Can't write the command to the proc."
		puts "Script terminated."
		exit 1
	}
	
	if {$rdlim_file != ""} {
		puts $rdlim_file "echo $cmd > ${get_proc_root}/ConfigCommand"
	}
	
		
	#read the response
	set response ""
	#puts "*** cat [get_proc_root]/ConfigCommand"
	if {[catch {set response [exec cat ${get_proc_root}/ConfigCommand]}]} {
		puts "Can't read the response to the proc."
		puts "Script terminated."
		exit 1
	}
	
	#analyze the response:
	set ok [regexp {([0-9][0-9][0-9]),([0-9][0-9][0-9]),?(.*)} $response all length resCode resText]
	if { $ok != 1} {
		puts "Wrong response format"
		puts "Command:'$cmd'"
		puts "Response:'$response'"
		exit 1
	}	
		
	if {$resCode != "200"} {
		puts "Return code for $cmd is $resCode - [AnalyzeCode $resCode]"
		exit 1
	}
	return $resText
}

proc AnalyzeCode { GivenCode } {
	set Codes 	 {200  400    500 			501 				502 					504				503 					505 					506}
	set Messages {"OK" "Busy" "Fatal Error" "Invalid Command" 	"Invalid Group Name" 	"Invalid Value" "Invalid property Name" "Read-Only property" 	"Write-Only property"}
	set index [lsearch $Codes $GivenCode]
	if {$index == -1} {
		return "Wrong code '$GivenCode'"
	}
	return [lindex $Messages $index]	
}

proc GetProtocolVersion {} {
	return [SendCommand 0x01]
}

proc SetProperty  {Group Name Value {rdlim_file ""}} {
	#puts "0x02,$Group,$Name,$Value"
	return [SendCommand "0x02,$Group,$Name,$Value" $rdlim_file]
}  
    
proc GetProperty {Group Name} {
	return [SendCommand "0x03,$Group,$Name"]
}      
proc CommitChanges {} {
	return [SendCommand 0x04]
}      
proc CancelChanges	{} {
	return [SendCommand 0x05]

}      

proc Configure80211D {{VendorID ""} {DeviceID ""} {HW_TYPE ""} {HW_REVISION ""}} {

	#we use this sript for each reboot except to restore default 
	set rdlim [open /mnt/jffs2/rdlim.sh w]
	puts $rdlim "#!/bin/sh"

	set HWkey "${VendorID}_${DeviceID}_${HW_TYPE}_${HW_REVISION}"
	puts "# HWKey is $HWkey\n#\n#"
	

	# load the ini file
	if {[catch {set handler [IniOpen rdlim.ini]}]} {
		puts "ERROR: Can't create handler file"
		exit 1
	}

	
	# search HW type
	puts "Searching key $HWkey"

	
	set HwSection [IniGetKey $handler HWTypes $HWkey]
	
	if {$HwSection == ""} {
		exit 1
	}
	

	set out [open "/mnt/jffs2/HW.ini" w]
	puts $out "HW_name=$HwSection"
	puts $out "HW_TYPE=$HW_TYPE"
	if {[catch {set sec [array names ::inihandler "$HwSection*"]}]} {
		puts "ERROR Can't write HW.ini file"
	}
	foreach key $sec {
		set HWitem [lindex [split $key #] 1]
		set item [IniGetKey $handler $HwSection $HWitem]
		puts $out "${HWitem}=${item}"
	}
	close $out
	
	#create link in the tmp
	if {![file exists /tmp/HW.ini]} { 
		eval "exec cp /mnt/jffs2/HW.ini  /tmp/HW.ini"
		eval "exec chmod +x /tmp/HW.ini"
	}

	
	# in this section check for a key '80211Dlimits'
	set LimitList [IniGetKey $handler $HwSection 80211Dlimits]
	if {[catch {set sec [array names ::inihandler "$LimitList*"]}]} {
		puts "ERROR Can't read 80211Dlimits list"
	}
	foreach key $sec {
		set limit [lindex [split $key #] 1]
		set value [IniGetKey $handler $LimitList $limit]
		SetProperty 80211D HWLim $value $rdlim
	}


	if {[catch {set sec [array names ::inihandler "GeneralPenalty*"]}]} {
		puts "ERROR Can't read GeneralPenalty list"
	}
	# hanlde all the TxPenalty frequncies that were not set.
	foreach key $sec {
		set freq [lindex [split $key #] 1]
		set value [IniGetKey $handler GeneralPenalty $freq]
		SetProperty AOCS AocsTxPowerPenalty "${freq},${value}" $rdlim
	}
	
	puts $rdlim "echo RDLIM IS DONE"
	close $rdlim
	exec chmod +x /mnt/jffs2/rdlim.sh

	
	#puts "Committing changes ..."
	CommitChanges
	#puts "Done."

}


set VendorID 0
set DeviceID 0
set HW_TYPE ""
set HW_REVISION ""
set HW_TYPE ""
set HW_REVISION ""
set HW_ID ""
set SubVendorID ""
set SubDeviceID ""

#puts "###################################################################"
#puts "# Start of RDLIM init section"
#puts "###################################################################"


if {[file exists /mnt/jffs2/HW.ini] && [file exists /mnt/jffs2/rdlim.sh]} {

	#create link in the tmp
	if {![file exists /tmp/HW.ini]} { 
		eval "exec cp /mnt/jffs2/HW.ini  /tmp/HW.ini"
		eval "exec chmod +x /tmp/HW.ini"
	}
	
	if {[catch {exec /mnt/jffs2/rdlim.sh}]} {
		puts "ERROR: Can't execute /mnt/jffs2/rdlim.sh"
	}
	CommitChanges
	exit
}

# full rdlim script will run during restore default prossecc only
set HW_TYPE [GetProperty EEPROM HWType]
set HW_REVISION [GetProperty EEPROM HWRevision] 
set HW_ID [GetProperty EEPROM HWID]

#PnP ID (Vendor ID, Device ID, Sub-Vendor ID, Sub-Device ID)
regexp {([^,]*),([^,]*),([^,]*),([^,]*)} $HW_ID all VendorID DeviceID SubVendorID SubDeviceID
#make sure that number of digits is correct
set VendorID [format {0x%04x} $VendorID]
set DeviceID [format {0x%04x} $DeviceID]
set HW_TYPE [format {0x%02x} $HW_TYPE]
set HW_REVISION [format {0x%02x} $HW_REVISION]




puts "# HWID = $HW_ID 	HWtype = $HW_TYPE	HWrevision = $HW_REVISION"
puts "# VendorID=$VendorID DeviceID=$DeviceID SubVendorID=$SubVendorID SubDeviceID=$SubDeviceID"

Configure80211D $VendorID $DeviceID $HW_TYPE $HW_REVISION


#puts "###################################################################"
#puts "# End of RDLIM init section"
#puts "###################################################################"
