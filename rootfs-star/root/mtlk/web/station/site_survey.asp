<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/tr/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" lang="en-US" xml:lang="en-US"><!-- InstanceBegin template="/Templates/station.dwt" codeOutsideHTMLIsLocked="false" -->
<head>
<meta http-equiv="content-type" content="text/html; charset=utf-8" />
<meta http-equiv="cache-control" content="no-cache">
<meta http-equiv="pragma" content="no-cache">
<meta http-equiv="X-UA-Compatible" content="IE=EmulateIE7"/>  
<!-- InstanceBeginEditable name="Page Title" -->
<title>Wireless Network Site Survey</title>
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
/*var securityMode, WEPKey0, WEPKey1, WEPKey2, WEPKey3, defaultWEPKey, WEPKeyLength, WEPAuth;
var WPAPersonalMode, WPAPersonalEncryption, WPAPSK;
var WPAEnterpriseMode, WPAEnterpriseEncryption, WPARadiusIP, WPARadiusPort, WPARadiusKey, RadiusReKeyInterval;*/
var waitCount=0;

function escapeHTML(str) {
	if (!str) return "";
	return String(str).replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;").replace(/"/g, "&quot;").replace(/'/g, "&#39;");
}

function sanitizeSiteSurvey(str) {
	if (!str) return "";
	str = str.replace(/(<td[^>]*\bid=["']?ESSID_\d+["']?[^>]*>\s*<div[^>]*\bid=["']?ssidField["']?[^>]*>)([\s\S]*?)(<\/div>\s*<\/td>)/gi, function(match, openTag, ssid, closeTag) {
		return openTag + escapeHTML(ssid) + closeTag;
	});
	return str.replace(/(<span[^>]*\bid=["']?ESSID_\d+["']?[^>]*>)([\s\S]*?)(<\/span>)/gi, function(match, openTag, ssid, closeTag) {
		return openTag + escapeHTML(ssid) + closeTag;
	});
}

function reWirteList(fieldID, disableFieldID, str) {
	stopToWait = true;
	if (disableFieldID != "" && str != "")
		objectDisplay(disableFieldID, "none");
	if (fieldID != "") {
		if (fieldID == "listSiteSurvey") {
			str = sanitizeSiteSurvey(str);
		}
		document.getElementById(fieldID).innerHTML = str;
	}
}

function updateSiteSurvey(){
	makeRequest("/goform/updateSiteSurvey", "something", "listSiteSurvey", "waitMsg");
	//setTimeout("updateSiteSurvey()", 10000);
}

function scanFieldDisplay(value)
{
	objectDisplay("scanField_1", value);
	objectDisplay("scanField_2", value);
	objectDisplay("scanField_3", value);
	objectDisplay("scanField_4", value);
}

function WEPDisplay(value) {
	objectDisplay("wepPassphraseField", "none");
	objectDisplay("WEPKey0Field", value);
	/*objectDisplay("WEPKey1Field", value);
	objectDisplay("WEPKey2Field", value);
	objectDisplay("WEPKey3Field", value);*/
	objectDisplay("WEPKey1Field", "none");
	objectDisplay("WEPKey2Field", "none");
	objectDisplay("WEPKey3Field", "none");
	objectDisplay("defaultWEPKeyField", value);
	objectDisplay("WEPKeyLengthField", value);
	objectDisplay("WEPAuthField", value);
}

function WPAPersonalDisplay(value) {
	objectDisplay("WPAPersonalEncryptionField", value);
	objectDisplay("WPAPSKField", value);
}

function selectSecurityMode(mode) {
	WEPDisplay("none");
	WPAPersonalDisplay("none");
	
	/*var WEPKey0 = "<% getParam(1, "WepKeys_DefaultKey0"); %>";
	var WEPKey1 = "<% getParam(1, "WepKeys_DefaultKey1"); %>";
	var WEPKey2 = "<% getParam(1, "WepKeys_DefaultKey2"); %>";
	var WEPKey3 = "<% getParam(1, "WepKeys_DefaultKey3"); %>";
	var defaultWEPKey = "<% getParam(1, "WepTxKeyIdx"); %>";
	var WEPKeyLength = "<% getParam(1, "NonProc_WepKeyLength"); %>";
	var WEPAuth = "<% getParam(1, "NonProc_Authentication"); %>";*/

	if (mode == 1) {
	
	}
	else if (mode == 2) {
		WEPDisplay("block");
	}
	else if (mode == 3) {
		WPAPersonalDisplay("block");
	}
	else {
		alert("<!--#tr id=\"w.wnss.alert.1\" -->Can't get mode.<!--#endtr-->");
	}
}

function checkValue() {
	var applyValue = true;
	var checkValue, keyLenCount, defaultWEPKeyValue;
	var hexKey0 = false;
	var hexKey1 = false;
	var hexKey2 = false;
	var hexKey3 = false;

	if (document.getElementById("securityMode").value == 2) {
		var keyLenBits = document.getElementById("WEPKeyLength").value;
		var hexRegex = (keyLenBits == 128) ? /^[0-9a-fA-F]{26}$/ : /^[0-9a-fA-F]{10}$/;
		var rowEl, keyEl, keyVal;
		for (loopCount = 0; loopCount < 4; loopCount++) {
			rowEl = document.getElementById("WEPKey" + loopCount + "Field");
			if (loopCount > 0 && (!rowEl || rowEl.style.display == "none"))
				continue;
			keyEl = document.getElementById("WEPKey" + loopCount);
			keyVal = keyEl ? keyEl.value : "";
			if ((loopCount == 0 && keyVal.length == 0) || (keyVal.length > 0 && !hexRegex.test(keyVal))) {
				if (keyLenBits == 128)
					alert("<!--#tr id=\"w.wnss.alert.3\" -->Please input 26 Hex character of key <!--#endtr-->" + (loopCount + 1));
				else
					alert("<!--#tr id=\"w.wnss.alert.2\" -->Please input 10 Hex character of key <!--#endtr-->" + (loopCount + 1) + "!");
				applyValue = false;
				break;
			}
		}
	}
	
	if (document.getElementById("securityMode").value == 3) {
		if (applyValue && document.getElementById("WPAPersonalEncryption").value == 0)
		{
			if (!confirm("<!--#tr id=\"HT.confirm\" -->WARNING: Your Wireless-N devices will only operate at Wireless-G data rates with the encryption type you have chosen.  Please select WPA2-Personal encryption if you would like to have your Wireless-N devices operating at full data rates.  Click OK to continue with this selection, or Cancel to choose another type of encryption.<!--#endtr-->"))
				applyValue = false;
		}
		
		if (applyValue && !checkWPAPSK(document.getElementById("display_WPAPSK").value)){
			applyValue = false;
		}
	}
	
	if (applyValue) {
		document.getElementById("activeMode").value = "connectToAP";
		document.getElementById("nowMode").value = "stationMode";
		
		//totalWaitTime = 80; //second
		//wait_page();
		document.getElementById("waitPad").style.display="block";
		document.stationScan.submit();
	}
}

function displaySecurityField(value) {
	objectDisplay("securityField", value);
	objectDisplay("ESSIDFeild", value);
	objectDisplay("securityModeField", value);
	objectDisplay("securitySeparate", value);
	WEPDisplay(value);
	WPAPersonalDisplay(value);
	objectDisplay("connectField", value);
}

function setSecurity() {
	var arrCountEl = document.getElementById("arrayCount");
	var arrCount = arrCountEl ? parseInt(arrCountEl.value, 10) : 0;
	if (isNaN(arrCount) || arrCount <= 0)
		arrCount = 0;

	var applyValue = false;
	for (loopCount=0; loopCount<arrCount; loopCount++) {
		var radioEl = document.getElementById("setRadio_"+loopCount);
		if (radioEl && radioEl.checked == true) {
			applyValue = true;
			break;
		}
	}
	if (!applyValue)
		alert("<!--#tr id=\"w.wnss.alert.4\" -->Please select one AP/Router that you want to connect and push Connect again.<!--#endtr-->");

	if (applyValue) {
		scanFieldDisplay("none");
		displaySecurityField("block");
		
		for (loopCount=0; loopCount<arrCount; loopCount++) {
			var selRadio = document.getElementById("setRadio_"+loopCount);
			if (selRadio && selRadio.checked == true) {
				var getESSID = getHTMLContent("ESSID_"+loopCount);
				var getBSSID = document.getElementById("BSSID_"+loopCount).value;
				var getChannel = document.getElementById("channel_"+loopCount).value;
				var getEncryType = document.getElementById("encryType_"+loopCount).value;
			
				document.getElementById("BSSID").value = getBSSID;
				if (getESSID != "") {
					document.getElementById("display_ESSID").readOnly = true;
					document.getElementById("display_ESSID").value = getESSID;
				}
				else {
					document.getElementById("display_ESSID").readOnly = false;
					document.getElementById("rebootNeed").value = 1;
				}
			
				if (getEncryType == "Disabled") {
					document.getElementById("securityMode").value = 1;
					document.getElementById("WPAPersonalMode").value = "";
					document.getElementById("securityModeText").innerHTML = "Disabled";
					selectSecurityMode(1);
				}
				else if (getEncryType == "WEP") {
					document.getElementById("securityMode").value = 2;
					document.getElementById("WPAPersonalMode").value = "";
					document.getElementById("securityModeText").innerHTML = "WEP";
					selectSecurityMode(2);
				}
				else if (getEncryType == "WPA-Personal") {
					document.getElementById("securityMode").value = 3;
					document.getElementById("WPAPersonalMode").value = "1";
					document.getElementById("securityModeText").innerHTML = "WPA Personal";
					selectSecurityMode(3);
					selectWPAMode(1);
					objectDisplay("WPAPersonalEncryptionField", "none");
				}
				else if (getEncryType == "WPA2-Personal") {
					document.getElementById("securityMode").value = 3;
					document.getElementById("WPAPersonalMode").value = "2";
					document.getElementById("securityModeText").innerHTML = "WPA2 Personal";
					selectSecurityMode(3);
					selectWPAMode(2);
					document.getElementById("WPAPersonalEncryption").options[0].selected = true;
				}
				else if (getEncryType == "WPA2-Personal Mixed") {
					document.getElementById("securityMode").value = 3;
					document.getElementById("WPAPersonalMode").value = "3";
					document.getElementById("securityModeText").innerHTML = "WPA2 Personal";
					selectSecurityMode(3);
					selectWPAMode(2);
					document.getElementById("WPAPersonalEncryption").options[1].selected = true;
				}
				else if (getEncryType == "WPA-Enterprise") {
					scanFieldDisplay("block");
					displaySecurityField("none");
					alert("<!--#tr id=\"w.wnss.alert.5\" -->We don't support WPA-Enterprise security mode!<!--#endtr-->");
					return;
				}
				else if (getEncryType == "WPA2-Enterprise") {
					scanFieldDisplay("block");
					displaySecurityField("none");
					alert("<!--#tr id=\"w.wnss.alert.6\" -->We don't support WPA2-Enterprise security mode!<!--#endtr-->");
					return;
				}
				else if (getEncryType == "WPA2-Enterprise Mixed") {
					scanFieldDisplay("block");
					displaySecurityField("none");
					alert("<!--#tr id=\"w.wnss.alert.7\" -->We don't support WPA2-Enterprise security mode!<!--#endtr-->");
					return;
				}
				break;
			}
		}
	}
}

function reScan() {
	document.getElementById("activeMode").value = "reScan";
	//stationScan.submit();
	document.location.reload();
}

function waitStatus()
{
	var addMsg=".";
	
	waitCount++;
	objectDisplay("waitPad", "block");
	objectDisplay("waitMsg", "block");		
	objectDisplay("scanField_3", "none");
	document.getElementById("waitMsg").innerHTML += addMsg;
	
	if (stopToWait)
	{
		document.getElementById("waitMsg").innerHTML = "";
		objectDisplay("waitPad", "none");
		objectDisplay("waitMsg", "none");
		scanFieldDisplay("block");
	}
	else if (!stopToWait && (waitCount < 70))
		window.setTimeout("waitStatus()", 500);
	else if (waitCount >= 70)
	{
		document.getElementById("waitMsg").innerHTML = "<!--#tr id=\"w.bws.asp.2\" -->Please wait <!--#endtr-->";
		waitCount = 0;
		window.setTimeout("waitStatus()", 500);
	}	
}

function page_load() {
	fwUpgraceStatus("<% getFWUpgrade(); %>", "<% getCurrectLanIP(); %>");
	// wpsStatus()
	
	var wirelessConfigType = "<% getParam(1, "wirelessConfigType"); %>";
	
	if (document.getElementById("wpsMode"))
		document.getElementById("wpsMode").href = "/station/wireless_basic.asp";
	if (document.getElementById("wirelessAdv"))
		document.getElementById("wirelessAdv").href = "wireless_advanced.asp?random=" + Math.random();
	
	displaySecurityField("none");
	document.getElementById("waitMsg").innerHTML = "<!--#tr id=\"w.bws.asp.2\" -->Please wait <!--#endtr-->";
	waitStatus();
	updateSiteSurvey();
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
					<!--#tr id=\"mainmenutitle.1\" -->Wireless<!--#endtr-->
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
									<td class="subMenuOption"><font class="small"><a href="/station/wireless_basic.asp">Basic Wireless Settings</a></font></td>
									<td class="subMenuDIV">|</td>
									<td class="subMenuOption"><font class="small"><a href="/station/wps_status.asp">Wi-Fi Protected Setup™</a></font></td>
									<td class="subMenuDIV">|</td>
									<td class="subMenuOption">Wireless Network Site Survey</td>
									<td class="subMenuDIV">|</td>
									<td class="subMenuOption"><font class="small"><a href="/station/wmm.asp">WMM®</a></font></td>
									<td class="subMenuDIV">|</td>
									<td class="subMenuOption"><font class="small"><a id="wirelessAdv" href="/station/wireless_advanced.asp">Advanced Wireless Settings</a></font></td>
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
	<form method="post" name="stationScan" action="/goform/stationScan">
		<tr>
			<td class="noSPACE">
				<input type="hidden" id="nowMode" name="nowMode" value="">
				<input type="hidden" id="activeMode" name="activeMode" value="">
				<input type="hidden" id="securityMode" name="securityMode" value="">
				<input type="hidden" id="WPAPersonalMode" name="WPAPersonalMode" value="">
				<input type="hidden" id="rebootNeed" name="rebootNeed" value="0">
				<input type="hidden" id="BSSID" name="BSSID" value="0">
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
				<table id="mainform" class="mainTableContent" cellspacing="0">
					<tr id="scanField_1">
						<td class="subMenuMainContent" colspan="2"><!--#tr id="w.wnss.1" -->Wireless Network Site Survey<!--#endtr--></td>
						<td colspan="2" class="blankContent"></td>
					</tr>
					<tr id="scanField_2">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td colspan="2" class="blankContent">
							<span style="font-size:14px; font-weight:bold;" id="waitMsg"></span>
							<span id="listSiteSurvey"></span>
						</td>
					</tr>
					<tr id="scanField_3" style="display:none">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td colspan="2" class="blankContent" align="right">
							<input type="button" name="scanButton" value="<!--#tr id=\"w.wnss.1-1\" -->Refresh<!--#endtr-->" onclick="reScan();">&nbsp;&nbsp;&nbsp;
							<input type="button" name="activateButton" value="<!--#tr id=\"w.wnss.1-2\" -->Connect<!--#endtr-->" onclick="setSecurity();">
						</td>
					</tr>
					<tr id="scanField_4" style="display:none">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="blankContent" colspan="2">&nbsp;</td>
					</tr>
					<tr id="securityField">
						<td class="subMenuMainContent" colspan="2"><!--#tr id="w.wnss.2" -->Wireless Security<!--#endtr--></td>
						<td colspan="2" class="blankContent"></td>
					</tr>
					<tr id="ESSIDFeild" style="display:none">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><!--#tr id="w.wnss.2-1" -->Network Name(SSID):<!--#endtr--></td>
						<td class="tdContent">
							<input type="text" id="display_ESSID" name="ESSID" size="33" maxlength="32" value="">
						</td>
					</tr>
					<tr id="securityModeField" style="display:none">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><!--#tr id="w.wnss.2-2" -->Security Mode:<!--#endtr--></td>
						<td class="tdContent">
							<!--We can't use disabled="disabled" to disable this field, becasue the api will can't get this value!!!!-->
							<!--<select id="securityMode" name="securityMode" onchange="selectSecurityMode(this.value)" disabled="disabled">-->
							<!--<select id="securityMode" name="securityMode">
								<option value="1">Open</option>
								<option value="2">WEP</option>
								<option value="3">WPA/WPA2 Personal</option>
								<option value="4">WPA/WPA2 Enterprise</option>
							</select>-->
							<span id="securityModeText"></span>
						</td>
					</tr>
					<tr id="securitySeparate" style="display:none">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="blankContent" colspan="2"><hr></td>
					</tr>
					<tr id="WEPKeyLengthField" style="display:none">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><!--#tr id="w.wnss.2-3" -->Encryption:<!--#endtr--></td>
						<td class="tdContent">
							<select id="WEPKeyLength" name="WEPKeyLength" onchange="selectWEPKeyLength(this.value);">
								<option value="128"><!--#tr id="w.wnss.2-4" -->104 / 128-bit (26 hex digits)<!--#endtr--></option>
								<option value="64" selected><!--#tr id="w.wnss.2-5" -->40 / 64-bit (10 hex digits)<!--#endtr--></option>
							</select>
						</td>
					</tr>
					<tr id="wepPassphraseField" style="display:none">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><!--#tr id="w.wnss.2-6" -->Passphrase:<!--#endtr--></td>
						<td class="tdContent"><input type="text" id="wepPassphrase" name="wepPassphrase" size="32" maxlength="32" value="" onkeyup="Update_WEPKey(this.value);"></td>
					</tr>
					<tr id="WEPKey0Field" style="display:none">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><!--#tr id="w.wnss.2-7" -->Key 1:<!--#endtr--></td>
						<td class="tdContent"><input type="text" id="WEPKey0" name="WEPKey0" size="27" maxlength="10" value=""></td>
					</tr>
					<tr id="WEPKey1Field" style="display:none">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><!--#tr id="w.wnss.2-8" -->Key 2:<!--#endtr--></td>
						<td class="tdContent"><input type="text" id="WEPKey1" name="WEPKey1" size="27" maxlength="10" value=""></td>
					</tr>
					<tr id="WEPKey2Field" style="display:none">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><!--#tr id="w.wnss.2-9" -->Key 3:<!--#endtr--></td>
						<td class="tdContent"><input type="text" id="WEPKey2" name="WEPKey2" size="27" maxlength="10" value=""></td>
					</tr>
					<tr id="WEPKey3Field" style="display:none">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><!--#tr id="w.wnss.2-10" -->Key 4:<!--#endtr--></td>
						<td class="tdContent"><input type="text" id="WEPKey3" name="WEPKey3" size="27" maxlength="10" value=""></td>
					</tr>
					<tr id="defaultWEPKeyField" style="display:none">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><!--#tr id="w.wnss.2-11" -->Tx Key:<!--#endtr--></td>
						<td class="tdContent"><input type="hidden" id="defaultWEPKey" name="defaultWEPKey" value="0">
							<!--#tr id="w.wnss.2-12" -->Key 1<!--#endtr-->
							<!--<select id="defaultWEPKey" name="defaultWEPKey" onchange="">
								<option value="0" selected>Key 1</option>
								<option value="1">Key 2</option>
								<option value="2">Key 3</option>
								<option value="3">Key 4</option>
							</select>-->
						</td>
					</tr>
					<tr id="WEPAuthField" style="display:none">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><!--#tr id="w.wnss.2-13" -->Authentication:<!--#endtr--></td>
						<td class="tdContent">
							<select id="WEPAuth" name="WEPAuth" onchange="">
								<option value="1"><!--#tr id="w.wnss.2-14" -->Open<!--#endtr--></option>
								<option value="2"><!--#tr id="w.wnss.2-15" -->Shared<!--#endtr--></option>
								<option value="3" selected><!--#tr id="w.wnss.2-16" -->Auto<!--#endtr--></option>
							</select>						</td>
					</tr>
					<tr id="WPAPersonalEncryptionField" style="display:none">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><!--#tr id="w.wnss.2-17" -->Encryption:<!--#endtr--></td>
						<td class="tdContent">
							<select id="WPAPersonalEncryption" name="WPAPersonalEncryption" onchange="">
								<option value="0"><!--#tr id="w.wnss.2-18" -->TKIP<!--#endtr--></option>
								<option value="1"><!--#tr id="w.wnss.2-19" -->AES<!--#endtr--></option>
								<option value="2" selected><!--#tr id="w.wnss.2-20" -->TKIP or AES<!--#endtr--></option>
							</select>
						</td>
					</tr>
					<tr id="WPAPSKField" style="display:none">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><!--#tr id="w.wnss.2-21" -->Passphrase:<!--#endtr--></td>
						<td class="tdContent">
							<!--<input type="text" id="WPAPSK" name="WPAPSK" size="48" maxlength="64" value="">-->
							<input type="hidden" id="submit_WPAPSK" name="submit_WPAPSK" value="">
							<input type="text" id="display_WPAPSK" name="WPAPSK" size="48" maxlength="64" value="">
						</td>
					</tr>
					<tr id="connectField" style="display:none">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td colspan="2" class="blankContent" align="right">
							<input type="button" name="scanButton" value="<!--#tr id=\"w.wnss.2-22\" -->Refresh<!--#endtr-->" onclick="reScan();">&nbsp;&nbsp;&nbsp;
							<input type="button" name="connectButton" value="<!--#tr id=\"w.wnss.2-23\" -->Connect<!--#endtr-->" onclick="checkValue();">
						</td>
					</tr>
					<!--<tr>
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="blankContent" colspan="2">&nbsp;</td>
					</tr>-->
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
							<!--<a class="bottonClass" href="javascript:setSecurity()">Save Settings</a>
							<a class="bottonClass" href="javascript:document.location.reload(true)">Cancel Changes</a>-->
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
