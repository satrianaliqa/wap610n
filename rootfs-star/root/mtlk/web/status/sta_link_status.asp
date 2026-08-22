<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/tr/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" lang="en-US" xml:lang="en-US"><!-- InstanceBegin template="/Templates/status.dwt" codeOutsideHTMLIsLocked="false" -->
<head>
<meta http-equiv="content-type" content="text/html; charset=utf-8" />
<meta http-equiv="cache-control" content="no-cache">
<meta http-equiv="pragma" content="no-cache">
<meta http-equiv="X-UA-Compatible" content="IE=EmulateIE7"/>  
<!-- InstanceBeginEditable name="Page Title" -->
<!--<meta http-equiv='refresh' content='5'>-->
<title>Wireless Network</title>
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
var waitCount=0;


function waitStatus()
{
	var addMsg=".";
	
	waitCount++;
	objectDisplay("waitPad", "block");
	objectDisplay("waitMsg", "block");		
	document.getElementById("waitMsg").innerHTML += addMsg;
	
	if (stopToWait)
	{
		document.getElementById("waitMsg").innerHTML = "";
		objectDisplay("waitPad", "none");
		objectDisplay("waitMsg", "none");
	}
	else if (!stopToWait && (waitCount < 70))
		window.setTimeout("waitStatus()", 500);
	else if (waitCount >= 70)
	{
		document.getElementById("waitMsg").innerHTML = "<!--#tr id=\"status.wait\" -->Please wait <!--#endtr-->";
		waitCount = 0;
		window.setTimeout("waitStatus()", 500);
	}
}

function updateLinkStatus(){
	if (stopToWait == true) {
		makeRequest("/goform/updateLinkStatus", "something", "listLinkStatus", "linkStatusTemp");
		stopToWait = false;
	}
	setTimeout("updateLinkStatus()", 12000);
}

function page_load() {
	fwUpgraceStatus("<% getFWUpgrade(); %>", "<% getCurrectLanIP(); %>");
	wpsStatus("<% getWPSStatus(); %>", "");
	
	document.getElementById("waitMsg").innerHTML = "<!--#tr id=\"status.wait\" -->Please wait <!--#endtr-->";
	waitStatus();
	makeRequest("/goform/checkDHCPMode", "something", "", "");
	makeRequest("/goform/updateLinkStatus", "something", "listLinkStatus", "linkStatusTemp");
	updateLinkStatus();
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
					<!--#tr id="mainmenutitle.4" -->Status<!--#endtr-->
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
						<td width="172" class="mainMenuOptionUpSide"></td>
						<td width="172" class="mainMenuSelectedUpSide"></td>
					</tr>
					<tr>
						<td width="172" class="mainMenuOption"><A href="../network/sta_network.asp"><!--#tr id="mainmenu.2" -->Setup<!--#endtr--></A></td>
						<td width="172" class="mainMenuOption"><A id="ap_wbridge" href=""><!--#tr id="mainmenu.1" -->Wireless<!--#endtr--></A></td>
						<td width="172" class="mainMenuOption"><A href="../admin/management.asp"><!--#tr id="mainmenu.3" -->Administration<!--#endtr--></A></td>
						<td width="172" class="mainMenuSelected"><A href="../status/device_status.asp"><!--#tr id="mainmenu.4" -->Status<!--#endtr--></A></td>
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
									<td class="subMenuOption"><font class="small"><a href="/status/device_status.asp"><!--#tr id="status.submenu.1" -->Bridge<!--#endtr--></a></font></td>
									<td class="subMenuDIV">|</td>
									<td class="subMenuOption"><!--#tr id="status.submenu.2" -->Wireless Network<!--#endtr--></td>
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
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td colspan="2" class="blankContent">
							<iframe class="rebootRedirect" name="rebootRedirect" id="rebootRedirect" frameborder="0" width="1" height="1" scrolling="yes" src="">redirect</iframe>
							<div id="waitform"></div>
						</td>
					</tr>
				</table>
				<span id="listLinkStatus"></span>
				<table id="linkStatusTemp" class="mainTableContent" cellspacing="0">
					<tr>
						<td class="subMenuMainContent" colspan="2"><!--#tr id="status.wn.1" -->Wireless Network<!--#endtr--></td>
						<td colspan="2" class="blankContent"></td>
					</tr>
					<tr>
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td colspan="" class="tdLabel"><!--#tr id="status.wn.1-1" -->Link Status:<!--#endtr--></td>
						<td class="tdContent"><span style="font-weight:bold;" id="waitMsg"></span>
							<span id="linkStatus"></span>
						</td>
					</tr>
					<tr>
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><!--#tr id="status.wn.1-2" -->MAC Address:<!--#endtr--></td>
						<td class="tdContent">
							<span id="wirelessMAC"></span>
						</td>
					</tr>
					<tr>
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><!--#tr id="status.wn.1-3" -->Network Name (SSID):<!--#endtr--></td>
						<td class="tdContent">
							<span id="ESSID"></span>
						</td>
					</tr>
					<tr>
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><!--#tr id="status.wn.1-4" -->BSSID:<!--#endtr--></td>
						<td class="tdContent">
							<span id="BSSID"></span>
						</td>
					</tr>
					<tr>
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><!--#tr id="status.wn.1-5" -->Radio Band:<!--#endtr--></td>
						<td class="tdContent">
							<span id="radioBand"></span>
						</td>
					</tr>
					<tr>
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><!--#tr id="status.wn.1-6" -->Channel Width:<!--#endtr--></td>
						<td class="tdContent">
							<span id="channelWidth"></span>
						</td>
					</tr>
					<tr>
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><!--#tr id="status.wn.1-7" -->Wide Channel:<!--#endtr--></td>
						<td class="tdContent">
							<span id="wideChannel"></span>
						</td>
					</tr>
					<tr>
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><!--#tr id="status.wn.1-8" -->Standard Channel:<!--#endtr--></td>
						<td class="tdContent">
							<span id="channel"></span>
						</td>
					</tr>
					<tr>
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><!--#tr id="status.wn.1-9" -->Bit Rate:<!--#endtr--></td>
						<td class="tdContent">
							<span id="bitRate"></span>
						</td>
					</tr>
					<tr>
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><!--#tr id="status.wn.1-10" -->Signal:<!--#endtr--></td>
						<td class="tdContent">
							<span id="signalLevel"></span>
						</td>
					</tr>
					<!--<tr>
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel">Wireless Channel:</td>
						<td class="tdContent">
							<span id="wirelessChannel"></span>
						</td>
					</tr>-->
					<tr>
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><!--#tr id="status.wn.1-11" -->Security:<!--#endtr--></td>
						<td class="tdContent">
							<span id="securityMode"></span>
						</td>
					</tr>
					<tr>
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"></td>
						<td class="tdContent">
						
						</td>
					</tr>
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
							<!--<a id="divBT1" class="bottonClass" href="javascript:checkValue()">Save Settings</a>
							<a id="divBT0" class="bottonClass" href="javascript:document.location.reload(true)">Cancel Changes</a>-->
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
