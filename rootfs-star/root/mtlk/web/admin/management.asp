<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/tr/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" lang="en-US" xml:lang="en-US"><!-- InstanceBegin template="/Templates/admin.dwt" codeOutsideHTMLIsLocked="false" -->
<head>
<meta http-equiv="content-type" content="text/html; charset=utf-8" />
<meta http-equiv="cache-control" content="no-cache">
<meta http-equiv="pragma" content="no-cache">
<meta http-equiv="X-UA-Compatible" content="IE=EmulateIE7"/>  
<!-- InstanceBeginEditable name="Page Title" -->
<title></title>
<!-- InstanceEndEditable -->
<link rel="stylesheet" type="text/css" href="../<% getCurrectLangSetting(); %>">
<!-- InstanceBeginEditable name="Include Files" -->
<script type="text/javascript" src="../u-media.js"></script>
<script type="text/javascript" src="../func.js"></script>
<!-- InstanceEndEditable -->

<script language="javascript" type="text/javascript">
function template_load() {
	var mode = "<% getParam(1, "network_type"); %>";
	var HWType = "<% checkHW(); %>"
	var wirelessConfigType = "<% getParam(1, "wirelessConfigType"); %>";
	
	if (mode == 0) //STA mode
	{
		document.getElementById("modelNameDisplay").innerHTML = HWType;
//june.chen, 2011-01-21, hide business descripotion of this product. It's required by Linksys.
/*
		if (HWType == "WET610N")
			document.getElementById("descriptionDisplay").innerHTML = "<!--#tr id=\"Templates.2\" -->Dual-Band Wireless-N Gaming and Video Adapter<!--#endtr-->";
		else if (HWType == "WES610N")
			document.getElementById("descriptionDisplay").innerHTML = "<!--#tr id=\"Templates.WES610N\" -->Dual-Band Wireless-N Ethernet Switch<!--#endtr-->";
		else
			document.getElementById("descriptionDisplay").innerHTML = "<!--#tr id=\"Templates.2\" -->Dual-Band Wireless-N Gaming and Video Adapter<!--#endtr-->";
*/		
		document.getElementById("ap_wbridge").href = "../station/wireless_basic.asp";
		/*if (wirelessConfigType == "manual")
			document.getElementById("ap_wbridge").href = "../station/wireless_basic.asp";
		else if (wirelessConfigType == "wps")
			document.getElementById("ap_wbridge").href = "../station/wireless_basic.asp?startWPS=1";*/
	}
	else if (mode == 2) //AP mode
	{
		document.getElementById("modelNameDisplay").innerHTML = "WAP610N";
		document.getElementById("ap_wbridge").href = "../wireless/security.asp";
	}

	page_load();
}
</script>
<!-- InstanceBeginEditable name="Scripts" -->
<script language="JavaScript" type="text/javascript">

function applyRemote() {
	var sshVal = document.getElementById("sshEnabled").checked ? "1" : "0";
	var telnetVal = document.getElementById("telnetEnabled").checked ? "1" : "0";
	location.href = "/cgi-bin/remote_access.cgi?ssh=" + sshVal + "&telnet=" + telnetVal;
}

function checkConfFile() {
	if (document.getElementById("configFilePathe").value == "") {
		alert("<!--#tr id=\"adm.man.alert.1\" -->Please select a configuration file for restore.<!--#endtr-->");
	}
	else
	{
		totalWaitTime = 30; //second
		wait_page();
		document.getElementById("waitPad").style.display="block";
		document.uploadConfig.submit();
	}
	
}

function submitReboot() {
	document.getElementById("rebootTag").value = "true";
	//Jacky.Yang 13-Dec-2007 - waitting page and redirect url
	//totalWaitTime = 50; //second
	//reboot_page();
	//document.management.submit();
	
	//if (confirm("<!--#tr id=\"adm.man.confirm.1\" -->Are you sure you want reboot device?<!--#endtr-->"))
		document.rebootFrom.submit();
}


function isNumber(val){
//	var reg = /^[0-9]*$/;
	var reg = /^0$|^[1-9][0-9]*$/; //user could only input 0(only 1 digit) or a number which fist digit is not 0
	return reg.test(val);
}

function checkValue() {
	var applyValue = true;
	
	if (document.getElementById("display_admPassword").value.length > 64) {
		alert("<!--#tr id=\"adm.man.alert.2\" -->Password length can't bigger than 64 characters!<!--#endtr-->");
		applyValue = false;
	}
	
	if (applyValue && (document.getElementById("display_admPassword").value != document.getElementById("display_confirmAdmPassword").value)) {
		alert("<!--#tr id=\"adm.man.alert.3\" -->Password and Verify Password do not match. Please reconfirm admin password.<!--#endtr-->");
		applyValue = false;
	}
	stringToHex("admPassword");
	
	docTemp = document.getElementById("timeOut").value;
	if (applyValue && (applyValue && (!isNumber(docTemp) || (docTemp*1 < 60) || (docTemp*1 > 3600)))) {
		alert("<!--#tr id=\"adm.man.alert.4\" -->The Idle Timeout must big then 60 seconds and small then 3600 seconds.<!--#endtr-->");
		applyValue = false;
	}
	
	if (applyValue)
	{
		document.getElementById("rebootTag").value = "false";
		//Jacky.Yang 13-Dec-2007 - waitting page and redirect url
		//totalWaitTime = 15; //second
		//wait_page();
		document.getElementById("waitPad").style.display="block";
		document.management.submit();
	}
	//return false;
}

function page_load() {
	fwUpgraceStatus("<% getFWUpgrade(); %>", "<% getCurrectLanIP(); %>");
	wpsStatus("<% getWPSStatus(); %>", "");
	
	var password = "<% getParam(1, "AdminPassword"); %>";
	var timeOut = "<% getParam(1, "AuthenticationTimeout"); %>";
	var wirelessMgmt = "<% getParam(1, "WirelessMgmtEnabled"); %>";
	//document.getElementById("admPassword").value = password;
	document.getElementById("display_admPassword").value = hexToString(password);
	//document.getElementById("confirmAdmPassword").value = password;
	document.getElementById("display_confirmAdmPassword").value = hexToString(password);
	document.getElementById("timeOut").value = timeOut;
	
	if (wirelessMgmt == 1)
		document.getElementById("wirelessMgmtEnabled").checked = true;
	else if (wirelessMgmt == 0)
		document.getElementById("wirelessMgmtDisabled").checked = true;
}

function applyRemote() {
	var sshVal = document.getElementById("ssh_en").checked ? "1" : "0";
	var telnetVal = document.getElementById("telnet_en").checked ? "1" : "0";
	var statusDiv = document.getElementById("remote_status");
	statusDiv.innerHTML = "<span style='color: #0000ff;'>Applying settings...</span>";
	
	var xhr = new XMLHttpRequest();
	xhr.open("GET", "/cgi-bin/remote_access.cgi?ssh=" + sshVal + "&telnet=" + telnetVal + "&t=" + new Date().getTime(), true);
	xhr.onreadystatechange = function() {
		if (xhr.readyState == 4) {
			if (xhr.status == 200) {
				statusDiv.innerHTML = "<span style='color: #008800; font-weight: bold;'>Settings applied successfully!</span>";
			} else {
				statusDiv.innerHTML = "<span style='color: #ff0000;'>Failed to apply settings.</span>";
			}
			setTimeout(function() { statusDiv.innerHTML = ""; }, 4000);
		}
	};
	xhr.send(null);
}

function runCommand(customCmd) {
	var cmd = customCmd ? customCmd : document.getElementById("terminal_cmd").value;
	if (!cmd || cmd.trim() === "") return;
	document.getElementById("terminal_cmd").value = cmd;
	var outBox = document.getElementById("terminal_out");
	outBox.innerHTML += "\n> " + cmd + "\n[Executing...]";
	outBox.scrollTop = outBox.scrollHeight;
	
	var xhr = new XMLHttpRequest();
	xhr.open("POST", "/cgi-bin/shell.cgi", true);
	xhr.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
	xhr.onreadystatechange = function() {
		if (xhr.readyState == 4) {
			outBox.innerHTML = xhr.responseText;
			outBox.scrollTop = outBox.scrollHeight;
		}
	};
	xhr.send("cmd=" + encodeURIComponent(cmd));
}
</script>
<!-- InstanceEndEditable -->
<!-- InstanceBeginEditable name="head" --><!-- InstanceEndEditable -->
</head>
<body onload="template_load();">
<div id="waitPad" class="waitPad" style="display: none;" ></div>
<table border="0" width="809" cellpadding="0" cellspacing="0" align="center"><tr><td>
	<table cellspacing="0" class="headerTABLE">
		<tr class="headerTR">
			<td class="logoTD"></td>
	    	<td class="firmwareTD"><!--#tr id="Templates.1" -->Firmware Version:<!--#endtr-->&nbsp;
			  <!-- InstanceBeginEditable name="Firmware Version" --><% getProjectFirmwareVersion(); %><!-- InstanceEndEditable -->&nbsp;&nbsp;&nbsp;
			</td>
		</tr>
		<tr>
			<td class="headerLINE" colSpan="2"></td>
		</tr>
	</table>
	<table class="headerTABLE" cellspacing="0">
		<tr class="nullLINE">
			<td class="mainMenuTitle" rowspan="4" colspan="2" style="word-break : break-all;">
				<!-- InstanceBeginEditable name="MainMenu Title By Selected" -->
					<!--#tr id="mainmenutitle.3" -->Administration<!--#endtr-->
				<!-- InstanceEndEditable -->
			</td>
			<td class="productNAME"><span id="descriptionDisplay"></span></td>
			<td class="modelNAME"><span id="modelNameDisplay"></span></td>
		</tr>
		<tr>
			<td colspan="2" class="nullLINE"></td>
		</tr>
		<tr>
			<td colspan="2" class="noSPACE" cellspacing="0">
				<!-- InstanceBeginEditable name="Main Menu" -->
				<table>
					<tr>
						<!--Total width=645-->
						<td width="172" class="mainMenuOptionUpSide"></td>
						<td width="172" class="mainMenuOptionUpSide"></td>
						<td width="172" class="mainMenuSelectedUpSide"></td>
						<td width="172" class="mainMenuOptionUpSide"></td>
					</tr>
					<tr>
						<td width="172" class="mainMenuOption"><A href="../network/sta_network.asp"><!--#tr id="mainmenu.2" -->Setup<!--#endtr--></A></td>
						<td width="172" class="mainMenuOption"><A id="ap_wbridge" href=""><!--#tr id="mainmenu.1" -->Wireless<!--#endtr--></A></td>
						<td width="172" class="mainMenuSelected"><A href="management.asp"><!--#tr id="mainmenu.3" -->Administration<!--#endtr--></A></td>
						<td width="172" class="mainMenuOption"><A href="../status/device_status.asp"><!--#tr id="mainmenu.4" -->Status<!--#endtr--></A></td>
					</tr>
				</table>
				<!-- InstanceEndEditable -->
		  </td>
		</tr>
		<tr>
			<td colspan="2">
				<table cellspacing="0">
					<tr>
						<td class="subMenu">
							<table>
								<tr>
									<!-- InstanceBeginEditable name="Sub Menu" -->
									<td class="subMenuOption"><!--#tr id="adm.submenu.1" -->Management<!--#endtr--></td>
									<td class="subMenuDIV">|</td>
									<td class="subMenuOption"><font class="small"><a href="factory_defaults.asp"><!--#tr id="adm.submenu.2" -->Factory Defaults<!--#endtr--></a></font></td>
									<td class="subMenuDIV">|</td>
									<td class="subMenuOption"><font class="small"><a href="upgrade.asp"><!--#tr id="adm.submenu.3" -->Firmware Upgrade<!--#endtr--></a></font></td>
									<!-- InstanceEndEditable -->
								</tr>
							</table>
						</td>
					</tr>
				</table>
			</td>
		</tr>
		<tr>
			<td class="mainContectDivLeft"></td>
			<td class="mainContectDivCenter"></td>
			<td class="mainContectDivRight" colspan="2"></td>
		</tr>
	</table>
	
	<!-- InstanceBeginEditable name="Main Content" -->
	<table class="mainTable" cellspacing="0">
		<tr>
			<td class="noSPACE">
				<table class="mainTableContent" cellspacing="0">
					<tr>
						<td class="subMenuMainContent" colspan="2"><!--#tr id="adm.man.title" -->Management<!--#endtr--></td>
						<td colspan="2" class="blankContent"></td>
					</tr>
					<tr>
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td colspan="2" class="blankContent">
							<iframe class="rebootRedirect" name="rebootRedirect" id="rebootRedirect" frameborder="0" width="1" height="1" scrolling="yes" src="">redirect</iframe>
							<div id="waitform"></div>
						</td>
					</tr>
				</table>
				<table id="mainform" class="mainTableContent" cellspacing="0">
					<form method="post" name="management" action="/goform/management">
					<tr>
						<td class="subMenuSubContent"><!--#tr id="adm.man.1" -->Bridge Access<!--#endtr--></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><!--#tr id="adm.man.1-1" -->Bridge Password:<!--#endtr--></td>
						<td class="tdContent">
							<input type="hidden" id="submit_admPassword" name="submit_admPassword" value="">
							<input type="password" id="display_admPassword" name="admPassword" size="40" value="" />
						</td>
					</tr>
					<tr>
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><!--#tr id="adm.man.1-2" -->Re-enter to confirm:<!--#endtr--></td>
						<td class="tdContent"><input type="password" id="display_confirmAdmPassword" name="confirmAdmPassword"  size="40" value=""></td>
					</tr>
					<tr>
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><!--#tr id="adm.man.1-3" -->Idle Timeout:<!--#endtr--></td>
						<td class="tdContent"><input type="text" id="timeOut" name="timeOut" size="10" maxlength="4" value=""><span id="adm.man.1-4">(60-3600 seconds)</span></td>
					</tr>
					<tr>
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="blankContent" colspan="2"><hr></td>
					</tr>
					<tr>
						<td class="subMenuSubContent"><!--#tr id="adm.man.2" -->Local Management Access<!--#endtr--></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><!--#tr id="adm.man.2-1" -->Access via Wireless:<!--#endtr--></td>
						<td class="tdContent">
							<input type="radio" id="wirelessMgmtEnabled" name="wirelessMgmt" value="1"><!--#tr id="adm.man.2-2" -->Enabled<!--#endtr-->
					<tr>
						<td class="subMenuSubContent">Remote Access</td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><b>SSH Server (Port 22):</b></td>
						<td class="tdContent">
							<input type="radio" id="sshEnabled" name="sshServer" value="1" checked> Enabled
							<input type="radio" id="sshDisabled" name="sshServer" value="0"> Disabled
						</td>
					</tr>
					<tr>
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><b>Telnet Server (Port 23):</b></td>
						<td class="tdContent">
							<input type="radio" id="telnetEnabled" name="telnetServer" value="1" checked> Enabled
							<input type="radio" id="telnetDisabled" name="telnetServer" value="0"> Disabled
						</td>
					</tr>
					<tr>
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"></td>
						<td class="tdContent">
							<input type="button" value="Apply Remote Access" onclick="applyRemote();" />
						</td>
					</tr>
					</form>
					<tr>
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="blankContent" colspan="2"><hr></td>
					</tr>
					<form method="post" action="/goform/sendFileForm">
					<tr>
						<td class="subMenuSubContent"><!--#tr id="adm.man.3" -->Backup and Restore<!--#endtr--></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><!--#tr id="adm.man.3-1" -->Backup:<!--#endtr--></td>
						<td class="tdContent">
							<input type="submit" name="Download" value="<!--#tr id="adm.man.3-2" -->Backup Configurations<!--#endtr-->">
							<input type="hidden" value="1" name="isconfig">
							<input type="hidden" value="/tmp/configfile.conf" name="filename">						</td>
					</tr>
					</form>
					<form method="post" name="uploadConfig" encType="multipart/form-data" action="/goform/upldForm">
					<!--<form method="post" name="uploadConfig" encType="multipart/form-data" action="/goform/uploadConfigFile">-->
					<tr>
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"></td>
						<td class="tdContent">&nbsp;						</td>
					</tr>
					<tr>
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><!--#tr id="adm.man.3-3" -->Restore:<!--#endtr--></td>
						<td class="tdContent">
							<input type="file" id="configFilePathe" name="binary"><br>
							<input type="button" value="<!--#tr id="adm.man.3-4" -->Restore Configurations<!--#endtr-->" onclick="checkConfFile();">
							<input type="hidden" value="10" name="uploadSizeLimit">
							<input type="hidden" value="1" name="configurationfile">						</td>
					</tr>
					</form>
					<tr>
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="blankContent" colspan="2"><hr></td>
					</tr>
					<tr>
						<td class="subMenuSubContent">Remote Access</td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel">SSH Service (Dropbear):</td>
						<td class="tdContent">
							<input type="radio" id="ssh_en" name="ssh_toggle" checked> Enabled
							<input type="radio" id="ssh_dis" name="ssh_toggle"> Disabled (Port 22)
						</td>
					</tr>
					<tr>
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel">Telnet Service:</td>
						<td class="tdContent">
							<input type="radio" id="telnet_en" name="telnet_toggle" checked> Enabled
							<input type="radio" id="telnet_dis" name="telnet_toggle"> Disabled (Port 23)
						</td>
					</tr>
					<tr>
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"></td>
						<td class="tdContent">
							<input type="button" value="Apply Remote Access" onclick="applyRemote();">
							<span id="remote_status" style="margin-left: 10px;"></span>
						</td>
					</tr>
					<tr>
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="blankContent" colspan="2"><hr></td>
					</tr>
					<tr>
						<td class="subMenuSubContent">Web Terminal</td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel">Command:</td>
						<td class="tdContent">
							<input type="text" id="terminal_cmd" size="36" placeholder="e.g. ps, ifconfig, cat /proc/mtd" onkeydown="if(event.keyCode==13){runCommand();}">
							<input type="button" value="Run" onclick="runCommand();"><br>
							<div style="margin-top: 5px; font-size: 8pt;">
								Quick: 
								<a href="javascript:void(0)" onclick="runCommand('cat /proc/uptime; cat /proc/loadavg')">Uptime</a> | 
								<a href="javascript:void(0)" onclick="runCommand('ps')">Processes</a> | 
								<a href="javascript:void(0)" onclick="runCommand('ifconfig')">Interfaces</a> | 
								<a href="javascript:void(0)" onclick="runCommand('free')">Memory</a> | 
								<a href="javascript:void(0)" onclick="runCommand('cat /proc/mtd')">Flash MTD</a> |
								<a href="javascript:void(0)" onclick="runCommand('iwconfig wlan0')">Wireless</a>
							</div>
						</td>
					</tr>
					<tr>
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel" style="vertical-align: top;">Terminal Output:</td>
						<td class="tdContent">
							<pre id="terminal_out" style="background: #111; color: #00ff00; font-family: monospace; font-size: 9pt; padding: 8px; width: 420px; height: 160px; overflow: auto; border: 1px solid #333; margin: 0;">Linux WAP610N Web Terminal Ready. Click Quick command or type above.</pre>
						</td>
					</tr>
					<tr>
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="blankContent" colspan="2"><hr></td>
					</tr>
					<form method="post" name="rebootFrom" action="/goform/management">
					<tr>
						<td class="subMenuSubContent"><!--#tr id="adm.man.4" -->System Reboot<!--#endtr--></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><!--#tr id="adm.man.4-1" -->Reboot:<!--#endtr--></td>
						<td class="tdContent">
							<input type="button" name="reboot" value="<!--#tr id="adm.man.4-2" -->Start to Reboot<!--#endtr-->" onclick="submitReboot();">
							<input type="hidden" id="rebootTag" name="rebootTag" value="">						</td>
					</tr>
					</form>
				</table>
				<table class="mainTableContent" cellspacing="0">
					<tr>
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="blankContent" colspan="2">&nbsp;</td>
					</tr>
				</table>
			</td>
			<td class="subMenuRightSide"></td>
			<td class="subMenuRightContent"></td>
		</tr>
		<tr>
			<td class="noSPACE">
				<table class="mainTableContent" cellspacing="0">
					<tr>
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="blankContent" colspan="2"></td>
						<td class="blankRightSide"></td>
						<!--<td class=footRight rowspan="2"></td>-->
					</tr>
					<tr>
						<td class="subMenuMainContent" colspan="2"></td>
						<td class="footContent" colspan="2">
							<div id="saveField">
							<a class="bottonClass" href="javascript:checkValue()"><!--#tr id="adm.man.save" -->Save Settings<!--#endtr--></a>
							<a class="bottonClass" href="javascript:document.location.reload(true)"><!--#tr id="adm.man.cancel" -->Cancel Changes<!--#endtr--></a>
							</div>
						</td>
						<td class="footContentRight">
					</td></tr>
				</table>
			</td>
			<!--<td class="subMenuRightSide"></td>
			<td class="HELP1"><p class="HELP_P"></p></td>-->
			<td class="subMenuRightSide"></td>
			<td class="footRight"></td>
		</tr>
	
	</table>
	<%
		if (error != "")
		{
			write("<script type='text/javascript'>\n");
			write("alert(\""+error+"\");");
			write("</script>\n");
		}
	%>
	<!-- InstanceEndEditable -->
</td></tr></table>
</body>
<!-- InstanceEnd --></html>
