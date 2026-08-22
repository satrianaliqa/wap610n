<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/tr/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" lang="en-US" xml:lang="en-US"><!-- InstanceBegin template="/Templates/station.dwt" codeOutsideHTMLIsLocked="false" -->
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

var RadarDetect = "<% getParam(1, "11hRadarDetect"); %>";
var AocsRestrictCh = "<% getParam(1, "AocsRestrictCh"); %>";
var AdvancedCoding = "<% getParam(1, "AdvancedCoding"); %>";
var HiddenSSID = "<% getParam(1, "HiddenSSID"); %>";
var ERPProtectionType = "<% getParam(1, "ERPProtectionType"); %>";
var OverlappingBSSProtection = "<% getParam(1, "OverlappingBSSProtection"); %>";
var ProtectionType = "<% getParam(1, "11nProtectionType"); %>";
var APforwarding = "<% getParam(1, "APforwarding"); %>";
var ReliableMulticast = "<% getParam(1, "ReliableMulticast"); %>";
//var STBC = "<% getParam(1, "STBC"); %>";
var PowerSelection = "<% getParam(1, "PowerSelection"); %>";
var Use11QMap = "<% getParam(1, "Use11QMap"); %>";
//u-media(rh001)-Jacky.Yang (Add), 15-Apr-2010, Add new feature for Linksys request.
var EthDownUp = "<% getParam(1, "EthDownUp"); %>";

function checkValue() {
	//Jacky.Yang 13-Dec-2007 - waitting page and redirect url
	//totalWaitTime = 100; //second
	//wait_page();
	document.getElementById("waitPad").style.display="block";	
	document.advancedWireless.submit();
	return false;
}

function stationMode() {
	//document.getElementById("RadarDetect").options[RadarDetect].selected = true;
	objectDisplay("RadarDetectField", "none");
	objectDisplay("AocsRestrictChField", "none");
	document.getElementById("AdvancedCoding").options[AdvancedCoding].selected = true;
	//document.getElementById("HiddenSSID").options[HiddenSSID].selected = true;
	objectDisplay("HiddenSSIDField", "none");
	document.getElementById("ERPProtectionType").options[ERPProtectionType].selected = true;
	//document.getElementById("OverlappingBSSProtection").options[OverlappingBSSProtection].selected = true;
	objectDisplay("OverlappingBSSProtectionField", "none");
	document.getElementById("ProtectionType").options[ProtectionType].selected = true;
	//document.getElementById("APforwarding").options[APforwarding].selected = true;
	objectDisplay("APforwardingField", "none");
	//document.getElementById("ReliableMulticast").options[ReliableMulticast].selected = true;
	objectDisplay("ReliableMulticastField", "none");
	//document.getElementById("STBC").options[STBC].selected = true;
	objectDisplay("STBCField", "none");
	docTemp = document.getElementById("PowerSelection");
	for (loopCount=0; loopCount<docTemp.length; loopCount++)
		if (docTemp.options[loopCount].value == PowerSelection)
			docTemp.options[loopCount].selected = true;

	document.getElementById("Use11QMap").options[Use11QMap].selected = true;
	//u-media(rh001)-Jacky.Yang (Add), 15-Apr-2010, Add new feature for Linksys request.
	document.getElementById("EthDownUp").options[EthDownUp].selected = true;
}

function apMode() {
	objectDisplay("RadarDetectField", "block");
	objectDisplay("AocsRestrictChField", "block");
	objectDisplay("HiddenSSIDField", "block");
	objectDisplay("OverlappingBSSProtectionField", "block");
	objectDisplay("APforwardingField", "block");
	objectDisplay("ReliableMulticastField", "block");
	//objectDisplay("STBCField", "block");
	objectDisplay("STBCField", "none");

	document.getElementById("RadarDetect").options[RadarDetect].selected = true;
	document.getElementById("AocsRestrictCh").value = AocsRestrictCh;
	document.getElementById("AdvancedCoding").options[AdvancedCoding].selected = true;
	document.getElementById("HiddenSSID").options[HiddenSSID].selected = true;
	document.getElementById("ERPProtectionType").options[ERPProtectionType].selected = true;
	document.getElementById("OverlappingBSSProtection").options[OverlappingBSSProtection].selected = true;
	document.getElementById("ProtectionType").options[ProtectionType].selected = true;
	document.getElementById("APforwarding").options[APforwarding].selected = true;
	document.getElementById("ReliableMulticast").options[ReliableMulticast].selected = true;
	//document.getElementById("STBC").options[STBC].selected = true;
	docTemp = document.getElementById("PowerSelection");
	for (loopCount=0; loopCount<docTemp.length; loopCount++)
		if (docTemp.options[loopCount].value == PowerSelection)
			docTemp.options[loopCount].selected = true;
			
	document.getElementById("Use11QMap").options[Use11QMap].selected = true;
}

function page_load() {
	fwUpgraceStatus("<% getFWUpgrade(); %>", "<% getCurrectLanIP(); %>");
	wpsStatus("<% getWPSStatus(); %>", "");
	
	var mode = "<% getParam(1, "network_type"); %>";
	var wirelessConfigType = "<% getParam(1, "wirelessConfigType"); %>";
	
	document.getElementById("wpsMode").href = "/station/wireless_basic.asp";
	/*if (wirelessConfigType == "manual")
		document.getElementById("wpsMode").href = "/station/wireless_basic.asp";
	else if (wirelessConfigType == "wps")
		document.getElementById("wpsMode").href = "/station/wireless_basic.asp?startWPS=1";*/
		
	if (mode == 0) //STA mode
	{
		stationMode();
		document.getElementById("nowMode").value = "stationMode";
	}
	else if (mode == 2) //AP mode
	{
		apMode();
		document.getElementById("nowMode").value = "apMode";
	}
}
</script>
<!-- InstanceEndEditable -->
<!-- InstanceBeginEditable name="head" --><!-- InstanceEndEditable -->
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
					<!--#tr id="mainmenutitle.5" -->Advanced<!--#endtr-->
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
						<td width="172" class="mainMenuSelectedUpSide"></td>
						<td width="172" class="mainMenuOptionUpSide"></td>
						<td width="172" class="mainMenuOptionUpSide"></td>
					</tr>
					<tr>
						<td width="172" class="mainMenuOption"><a href="../network/sta_network.asp"><!--#tr id="mainmenu.2" -->Setup<!--#endtr--></a></td>
						<td width="172" class="mainMenuSelected"><a id="ap_wbridge" href=""><!--#tr id="mainmenu.1" -->Wireless<!--#endtr--></a></td>
						<td width="172" class="mainMenuOption"><a href="../admin/management.asp"><!--#tr id="mainmenu.3" -->Administration<!--#endtr--></a></td>
						<td width="172" class="mainMenuOption"><a href="../status/device_status.asp"><!--#tr id="mainmenu.4" -->Status<!--#endtr--></a></td>
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
									<td class="subMenuOption"><font class="small"><a id="wpsMode" href="/station/wireless_basic.asp"><!--#tr id="w.submenu.1" -->Basic Wireless Settings<!--#endtr--></a></font></td>
									<td class="subMenuDIV">|</td>
									<td class="subMenuOption"><font class="small"><a href="/station/site_survey.asp"><!--#tr id="w.submenu.2" -->Wireless Network Site Survey<!--#endtr--></a></font></td>
									<td class="subMenuDIV">|</td>
									<td class="subMenuOption"><font class="small"><a href="/station/wmm.asp"><!--#tr id="w.submenu.3" -->WMM®<!--#endtr--></a></font></td>
									<td class="subMenuDIV">|</td>
									<td class="subMenuOption"><font class="small"><!--#tr id="w.submenu.4" -->Advanced Wireless Settings<!--#endtr--></font></td>
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
	<form method="post" name="advancedWireless" action="/goform/advancedWireless">
		<tr>
			<td class="noSPACE">
				<input type="hidden" id="nowMode" name="nowMode" value="">
				<table class="mainTableContent" cellspacing="0">
					<tr>
						<td class="subMenuMainContent" colspan="2"><!--#tr id="w.aws.title" -->Advanced Wireless Settings<!--#endtr--></td>
						<td colspan="2" class="blankContent"></td>
					</tr>
					<!--<tr>
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="blankContent" colspan="2"><hr /></td>
					</tr>-->
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
					<tr id="RadarDetectField">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><!--#tr id="w.aws.1" -->Use Radar Detect:<!--#endtr--></td>
						<td class="tdContent">
							<select id="RadarDetect" name="RadarDetect">
								<option value="0" selected><!--#tr id="w.aws.2" -->No<!--#endtr--></option>
								<option value="1"><!--#tr id="w.aws.3" -->Yes<!--#endtr--></option>
							</select>
						</td>
					</tr>
					<tr id="AocsRestrictChField">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><!--#tr id="w.aws.4" -->List of AOCS restricted channels:<!--#endtr--></td>
						<td class="tdContent">
							<input id="AocsRestrictCh" name="AocsRestrictCh" value="None">
						</td>
					</tr>
					<tr>
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><!--#tr id="w.aws.5" -->Use LDPC:<!--#endtr--></td>
						<td class="tdContent">
							<select id="AdvancedCoding" name="AdvancedCoding">
								<option value="0"><!--#tr id="w.aws.6" -->No<!--#endtr--></option>
								<option value="1" selected><!--#tr id="w.aws.7" -->Yes<!--#endtr--></option>
							</select>
						</td>
					</tr>
					<tr id="HiddenSSIDField">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><!--#tr id="w.aws.8" -->Use Hidden SSID:<!--#endtr--></td>
						<td class="tdContent">
							<select id="HiddenSSID" name="HiddenSSID">
								<option value="0" selected><!--#tr id="w.aws.9" -->No<!--#endtr--></option>
								<option value="1"><!--#tr id="w.aws.10" -->Yes<!--#endtr--></option>
							</select>
						</td>
					</tr>
					<tr>
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><!--#tr id="w.aws.11" -->ERP Protection Type:<!--#endtr--></td>
						<td class="tdContent">
							<select id="ERPProtectionType" name="ERPProtectionType">
								<option value="0"><!--#tr id="w.aws.12" -->None<!--#endtr--></option>
								<option value="1" selected><!--#tr id="w.aws.13" -->RTS/CTS<!--#endtr--></option>
								<option value="2"><!--#tr id="w.aws.14" -->CTS2Self<!--#endtr--></option>
							</select>
						</td>
					</tr>
					<tr id="OverlappingBSSProtectionField">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><!--#tr id="w.aws.15" -->Use Overlapping BSS Protection:<!--#endtr--></td>
						<td class="tdContent">
							<select id="OverlappingBSSProtection" name="OverlappingBSSProtection">
								<option value="0"><!--#tr id="w.aws.16" -->No<!--#endtr--></option>
								<option value="1" selected><!--#tr id="w.aws.17" -->Yes<!--#endtr--></option>
							</select>
						</td>
					</tr>
					<tr>
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><!--#tr id="w.aws.18" -->11n Protection Type:<!--#endtr--></td>
						<td class="tdContent">
							<select id="ProtectionType" name="ProtectionType">
								<option value="0"><!--#tr id="w.aws.19" -->None<!--#endtr--></option>
								<option value="1" selected><!--#tr id="w.aws.20" -->RTS/CTS<!--#endtr--></option>
								<option value="2"><!--#tr id="w.aws.21" -->CTS2Self<!--#endtr--></option>
							</select>
						</td>
					</tr>
					<tr id="APforwardingField">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><!--#tr id="w.aws.22" -->Enable AP forwarding:<!--#endtr--></td>
						<td class="tdContent">
							<select id="APforwarding" name="APforwarding">
								<option value="0"><!--#tr id="w.aws.23" -->No<!--#endtr--></option>
								<option value="1" selected><!--#tr id="w.aws.24" -->Yes<!--#endtr--></option>
							</select>
						</td>
					</tr>
					<tr id="ReliableMulticastField">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><!--#tr id="w.aws.25" -->Use reliable multicast:<!--#endtr--></td>
						<td class="tdContent">
							<select id="ReliableMulticast" name="ReliableMulticast">
								<option value="0"><!--#tr id="w.aws.26" -->No<!--#endtr--></option>
								<option value="1" selected><!--#tr id="w.aws.27" -->Yes<!--#endtr--></option>
							</select>
						</td>
					</tr>
					<tr id="STBCField">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><!--#tr id="w.aws.28" -->Use STBC:<!--#endtr--></td>
						<td class="tdContent">
							<select id="STBC" name="STBC">
								<option value="0" selected><!--#tr id="w.aws.29" -->No<!--#endtr--></option>
								<option value="1"><!--#tr id="w.aws.30" -->Yes<!--#endtr--></option>
							</select>
						</td>
					</tr>
					<tr>
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><!--#tr id="w.aws.31" -->Percentage of Maximal Transmit Power:<!--#endtr--></td>
						<td class="tdContent">
							<select id="PowerSelection" name="PowerSelection">
								<option value="0" selected>100%</option>
								<option value="-3">50%</option>
								<option value="-6">25%</option>
								<option value="-9">12%</option>
							</select>
						</td>
					</tr>
					<tr id="Use11QMapField">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><!--#tr id="w.aws.32" -->QoS Classification Type:<!--#endtr--></td>
						<td class="tdContent">
							<select id="Use11QMap" name="Use11QMap">
								<option value="0" selected>802.1D</option>
								<option value="1">802.1Q</option>
							</select>
						</td>
					</tr>
					<!--u-media(rh001)-Jacky.Yang (Add), 15-Apr-2010, Add new feature for Linksys request.-->
					<tr id="Re-EstablishEthernet">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><!--#tr id="w.aws.38" -->Re-Establish Ethernet Connection When Initiating Wireless Connection:<!--#endtr--></td>
						<td class="tdContent">
							<select id="EthDownUp" name="EthDownUp">
								<option value="0"><!--#tr id="w.aws.2" -->No<!--#endtr--></option>
								<option value="1" selected><!--#tr id="w.aws.3" -->Yes<!--#endtr--></option>
							</select>
						</td>
					</tr>
				</table>
				<table class="mainTableContent" cellspacing="0">
					<tr>
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="blankContent" colspan="2">&nbsp;</td>
					</tr>
				</table>			</td>
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
							<a class="bottonClass" href="javascript:checkValue()"><!--#tr id="w.aws.save" -->Save Settings<!--#endtr--></a>
							<a class="bottonClass" href="javascript:document.location.reload(true)"><!--#tr id="w.aws.save" -->Cancel Changes<!--#endtr--></a>
							</div>
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
	</form>
	</table>	
	<!-- InstanceEndEditable -->
</td></tr></table>
</body>
<!-- InstanceEnd --></html>
