<!-- Copyright (c) 2008, U-Media Communications, Inc. All Rights Reserved. -->
<html xmlns="http://www.w3.org/1999/xhtml" lang="en-US" xml:lang="en-US"><!-- InstanceBegin template="/Templates/admin.dwt" codeOutsideHTMLIsLocked="false" -->
<head>
<meta http-equiv="content-type" content="text/html; charset=utf-8" />
<meta http-equiv="cache-control" content="no-cache">
<meta http-equiv="pragma" content="no-cache">
<meta http-equiv="X-UA-Compatible" content="IE=EmulateIE7"/>  
<!-- InstanceBeginEditable name="Page Title" -->
<title>Firmware Upgrade</title>
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
		document.getElementById("modelNameDisplay").innerHTML = "WAP610N"; document.getElementById("descriptionDisplay").innerHTML = "Dual-Band Wireless-N Access Point";
		document.getElementById("ap_wbridge").href = "../wireless/security.asp";
	}

	page_load();
}
</script>
<!-- InstanceBeginEditable name="Scripts" -->
<script language="JavaScript" type="text/javascript">

var burn_state, burn_progress;
var firstTime=true;
var stopProcess=false;
var http_request = false;
var langset = "<% getParam(1, "Language"); %>";

function blinkingWarningMSG(displayMsg)
{
	if (displayMsg == "true")
	{
		//objectDisplay("warningMsgContent", "block");
		document.getElementById("warningMsgContent").innerHTML = "<font color=\"#FF0000\" style=\"font-size:11pt; font-weight:bold;\"><!--#tr id=\"adm.fu.6\" -->Upgrade must NOT be interrupted !!<!--#endtr--></font>";
		//displayMsg = false;
		setTimeout("blinkingWarningMSG(\"false\")", 1000);
	}
	else
	{
		//objectDisplay("warningMsgContent", "none");
		document.getElementById("warningMsgContent").innerHTML = "<font color=\"#FF0000\" style=\"font-size:11pt; font-weight:bold;\">&nbsp;</font>";
		//displayMsg = true;
		setTimeout("blinkingWarningMSG(\"true\")", 100);
	}
}

function makeRequest(url, content) {
	http_request = false;
	if (window.XMLHttpRequest) { // Mozilla, Safari,...
		http_request = new XMLHttpRequest();
		if (http_request.overrideMimeType) {
			http_request.overrideMimeType('text/xml');
		}
	} else if (window.ActiveXObject) { // IE
		try {
			http_request = new ActiveXObject("Msxml2.XMLHTTP");
		} catch (e) {
			try {
			http_request = new ActiveXObject("Microsoft.XMLHTTP");
			} catch (e) {}
		}
	}
	if (!http_request) {
		alert("<!--#tr id=\"adm.fu.alert.1\" -->Giving up :( Cannot create an XMLHTTP instance<!--#endtr-->");
		return false;
	}
	http_request.onreadystatechange = alertContents;
	http_request.open('POST', url, true);
	http_request.send(content);
}

function alertContents() {
	if (http_request.readyState == 4) {
		if (http_request.status == 200) {
			reWirteList( http_request.responseText);
		} else {
			alert("<!--#tr id=\"adm.fu.alert.2\" -->There was a problem with the request.<!--#endtr-->");
		}
	}
}

function reWirteList(str)
{
	stopToWait = true;
	
	docTemp = str;
	index = docTemp.indexOf(',');
	burn_state = docTemp.substr(0, index);
	burn_progress = docTemp.substr(index+1);
	
	if ((burn_progress == -1)) {
		alert("<!--#tr id=\"adm.fu.alert.3\" -->This image file is incorrect.<!--#endtr-->");
		stopProcess = true;
	}
	else if (burn_state*1 == 4) {
		document.getElementById("waitPad").style.display="block";
	}
	else if (burn_state*1 == 5) {
		document.getElementById("waitPad").style.display="block";
		displayAllField("block");
		percent = burn_progress*1;

		document.getElementById("progress").style.backgroundColor = "blue";
		document.getElementById("progress").style.width = Math.round(percent) + "%";
		document.getElementById("progressValue").innerHTML = Math.round(percent) + "%";
	}
	else if ((burn_state*1 == 1) && (burn_progress == 100)) {
		stopProcess = true;
		location.href = "/reboot_page.asp";
	}
}

function updateBurnProgress(){
	if (stopToWait == true) {
		if (!stopProcess) {
			makeRequest("/goform/updateBurnProgress", "something");
			stopToWait = false;
		}
	}
	setTimeout("updateBurnProgress()", 1500);
}

function reloadMe(){

	if(history.back()){
		location.href="admin/upgrade.asp";
	}
}

function displayAllField(value)
{
	objectDisplay("mainform", value);
	objectDisplay("upgradeProgress", value);
	objectDisplay("warningMsg", value);
}

function checkFW()
{
	var applyValue = true;	
	if (document.getElementById("fwFile").value == "")
		applyValue = false;

	if (applyValue) {
		//blinkingWarningMSG("true");
		document.upload_frm.submit();
	}
}

function submitReboot() {
	//alert("jacky - check firefox");
	document.getElementById("rebootTag").value = "true";
	document.management.submit();
}

function page_load() {
	objectDisplay("warningMsg", "none");
	blinkingWarningMSG("true");
	if(langset == "SA"){
		document.getElementById("progressTD").style.textAlign = "right";
	}
	stopToWait = true;
	updateBurnProgress();
}
</script>
<!-- InstanceEndEditable -->
<!-- InstanceBeginEditable name="head" -->
<!-- InstanceEndEditable -->
</head>
<body onLoad="template_load();">
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
									<td class="subMenuOption"><font class="small"><a href="management.asp"><!--#tr id="adm.submenu.1" -->Management<!--#endtr--></a></font></td>
									<td class="subMenuDIV">|</td>
									<td class="subMenuOption"><font class="small"><a href="factory_defaults.asp"><!--#tr id="adm.submenu.2" -->Factory Defaults<!--#endtr--></a></font></td>
									<td class="subMenuDIV">|</td>
									<td class="subMenuOption"><!--#tr id="adm.submenu.3" -->Firmware Upgrade<!--#endtr--></td>
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
						<td class="subMenuMainContent" colspan="2"><!--#tr id="adm.fu.title" -->Firmware Upgrade<!--#endtr--></td>
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
					<!--<tbody>-->
					<form method="post" name="upload_frm" enctype="multipart/form-data" action="/goform/upldImage">
					<!--<tr>
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td colspan="2" class="blankContent">
						</td>
					</tr>-->
					<tr>
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="upgradeFirmware"></td>
						<td class="upgradeContent"></td>
					</tr>
					<tr>
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="upgradeFirmware"><!--#tr id="adm.fu.1" -->Please select a file to upgrade the firmware:<!--#endtr--></td>
						<td>
							<input type="hidden" name="page_name" value="admin/upgrade.asp"/>
							<input type="hidden" name="uploadSizeLimit" value="10000">
							<input type="hidden" name="remote_filename" value="/dev/mtdblock2">
							<input type="hidden" name="validate_file_name" value="bootpImage">
							<input type="file" id="fwFile" name="binary">
						</td>
					</tr>
					<tr>
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="upgradeFirmware"></td>
						<td>
							<input type="button" value="<!--#tr id=\"adm.fu.2\" -->Start to Upgrade<!--#endtr-->" onclick="checkFW();" />
						</td>
					</tr>
					<tr>
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="blankContent" colspan="2"><hr></td>
					</tr>
					</form>
					<tr>
						<td class="subMenuSubContent">Backup Firmware</td>
						<td class="subMenuLeftSide"></td>
						<td class="upgradeFirmware">Download NOR Flash Dump:</td>
						<td>
							<input type="button" value="Dump Full Flash (4MB .bin)" onclick="location.href='/cgi-bin/dump_firmware.cgi?type=full';" />
							<input type="button" value="Dump Kernel+RootFS (3.68MB)" onclick="location.href='/cgi-bin/dump_firmware.cgi?type=kernel';" style="margin-left: 5px;" />
							<div style="font-size: 8pt; color: #555; margin-top: 4px;">Directly dumps physical flash partitions to your PC for complete offline recovery and backup.</div>
						</td>
					</tr>
					<tr>
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="blankContent" colspan="2"><hr></td>
					</tr>
					<!--</tbody>-->
				</table>
				<table id="upgradeProgress" class="mainTableContent" cellspacing="0">
					<tr>
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="fwUpgradeLabel"><!--#tr id="adm.fu.3" -->Warning:<!--#endtr--></td>
						<td class="fwUpgradeContent">
							<div style="width:270px;text-align:left;word-wrap:break-word;"><!--#tr id="adm.fu.4"" -->Upgrading firmware may take a few minutes, please don't turn off <!--#endtr--><!--#tr id="adm.fu.5" -->the power or press the reset button.<!--#endtr--></div>
						</td>
					</tr>
					<tr>
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="fwUpgradeLabel"></td>
						<td class="fwUpgradeContent" style="word-break : break-all;">
						</td>
					</tr>
					<tr>
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td colspan="2" class="blankContent" align="center">
							<br>
							<table width="350px" bolder="0">
								<tr>
									<td width="320px">
									<table width="320px" align="left" bgcolor="#ffffff" cellpadding="0" cellspacing="0" bordercolor="#000000" style="border-style: solid; border-width: 1px">
										<tr><td id="progressTD" align="left">
											<table id="progress" bgcolor="white" height="25">
												<tr><td></td></tr>
											</table>
										</td></tr>
									</table>
									</td>
									<td width="20px">
										<span id="progressValue">0%</span>
									</td>
								</tr>
							</table>
						</td>
					</tr>
				</table>
				<table id="warningMsg" class="mainTableContent" cellspacing="0">
					<tr>
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td colspan="2" class="blankContent" align="center" height="50">&nbsp;
							<span id="warningMsgContent"><font color="#FF0000" style="font-size:11pt; font-weight:bold;">Upgrade must NOT be interrupted !!</font></span>
						</td>
					</tr>
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
	</table>	
	<table class="mainTable" cellspacing="0">
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
						</td>
						<td class="footContentRight"></td>
					</tr>
				</table>
			</td>
			<!--<td class="subMenuRightSide"></td>
			<td class="HELP1"><p class="HELP_P"></p></td>-->
			<td class="subMenuRightSide"></td>
			<td class="footRight"></td>
		</tr>
	</table>
	<!-- InstanceEndEditable -->
</td></tr></table>
</body>
<!-- InstanceEnd --></html>
