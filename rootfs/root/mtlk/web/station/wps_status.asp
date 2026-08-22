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
var updateWPSStatusStopToWait = true;
var stopUpdateWPSStatus = false;
var StopWaitEthernetUP = false;
var onceEvent = true;
var applyParsing = true;
var displayButton = false;
var stopAlert = false;
var successCheck = false;
var waitEthernetUPTag = true;
var ethernetUPTagCount=0;
var changeToCloseField = 0;
var percent=0, checkPercent=0;
//var wps_info, wps_type, getTemp, currentESSID="NULL";
var wps_info, wps_type, getTemp, currentESSID="";
var displayESSID="";
var preStatus;
var ipConfigMethod;
var doRefresh = false;
var langset = "<% getParam(1, "Language"); %>";

var WPS_CURRENT_STAT_IDLE="-1";
var WPS_CURRENT_STAT_SEARCHING="1";
var WPS_CURRENT_STAT_REGISTERING="2";
var WPS_CURRENT_STAT_CONNECTING="3";
var WPS_CURRENT_STAT_ERROR_WALK_TIMEOUT="10";
var WPS_CURRENT_STAT_ERROR_SESSION_TIMEOUT="11";
var WPS_CURRENT_STAT_ERROR_SESSION_OVERLAP="12";
var WPS_CURRENT_STAT_ERROR_INVALID_PIN="13";
var WPS_CURRENT_STAT_CONNECTED="99";

function checkValue() {
	//management.submit();
	return false;
}


function blinkingWaitMSG(displayMsg)
{
	if (stopAlert)
	{
		document.getElementById("writeSuccessButton").innerHTML = "<input type=button value=\"Close\" onclick=\"returnWPS();\">";
	}
	else if ((displayMsg == "true") && !stopAlert)
	{
		document.getElementById("writeSuccessButton").innerHTML = "<font color=#FF0000><!--#tr id=\"w.bws.asp.13\" -->Please wait.....<!--#endtr--></font>";
		setTimeout("blinkingWaitMSG(\"false\")", 2000);
	}
	else
	{
		document.getElementById("writeSuccessButton").innerHTML = "<font color=#FFFFFF><!--#tr id=\"w.bws.asp.13\" -->Please wait.....<!--#endtr--></font>";
		setTimeout("blinkingWaitMSG(\"true\")", 100);
	}
}

function displayCloseButton(tag)
{
	stopAlert = true;
	document.getElementById("waitPad").style.display="none";
	if ((tag != "timeout") && (successCheck == true))
	{
		document.getElementById("writeSuccessButton").innerHTML = "<input type=button value=\"Close\" onclick=\"returnWPS();\">";
	}
	else if ((tag == "timeout") && (successCheck == true))
		document.getElementById("writeSuccessButton").innerHTML = "<input type=button value=\"Close\" onclick=\"returnWPS();\">";
	else if ((tag == "timeout") && (successCheck == false))
	{
		document.getElementById("writeSuccessButton").innerHTML = "<input type=button value=\"Close\" onclick=\"returnWPS();\">";
		//document.getElementById("writeSuccessButton").innerHTML = "<font color=#FF0000><!--#tr id=\"w.bws.js.6\" -->Device can't get current ip address successfully, please try to find it on the AP!<!--#endtr--></font>";
	}
}

function makeCleanWPSTempRequest(url, content) {
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
		alert('Giving up :( Cannot create an XMLHTTP instance');
		return false;
	}
	http_request.onreadystatechange = function () {alertContentsCleanWPSTemp()};
	http_request.open('POST', url, true);
	http_request.send(content);
}

function alertContentsCleanWPSTemp() {
	if (http_request.readyState == 4) {
		if (http_request.status == 200) {
			if (http_request.responseText != "NULL")
				temp = http_request.responseText;
		} else {
			//alert('There was a problem with the request.');
		}
	}
}

function cleanWPSFailTemp(){
		makeCleanWPSTempRequest("/goform/cleanWPSFailTemp", "something");
		doRefresh = true;
}

function cleanWPSTemp(){
		makeCleanWPSTempRequest("/goform/cleanWPSTemp", "something");
		//doRefresh = true;
}

function returnWirelessPage()
{
	//alert("returnWPS: " + currentIP);
	//if(!checkIpAddr(currentIP, false, "Network IP Address"))
	if (currentIP != "")
		window.location = "http://" + currentIP + "/station/wireless_basic.asp";
	else
		window.location = "/station/wireless_basic.asp";
}

function returnWPS()
{
	cleanWPSTemp();
	setTimeout("returnWirelessPage()", 2000);
}

function makeRequestWaitEthernetUP(url, content) {
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
		alert('Giving up :( Cannot create an XMLHTTP instance');
		return false;
	}
	http_request.onreadystatechange = function () {alertContentsWaitEthernetUP()};
	http_request.open('POST', url, true);
	http_request.send(content);
}

function alertContentsWaitEthernetUP() {
	if (http_request.readyState == 4) {
		if (http_request.status == 200) {
			if ((http_request.responseText == "1") || (ipConfigMethod*1 == 1))
			{
				document.getElementById("waitPad").style.display="none";
				successCheck = true;
				setTimeout("displayCloseButton(\"ok\")", 30000);
				waitEthernetUPTag = false;
				StopWaitEthernetUP = true;
				if (ipConfigMethod*1 == 1)
					showField("successField");
			}
			else
				waitEthernetUPTag = true;
		} else {
			//alert('There was a problem with the request.');
		}
	}
}

function waitEthernetUP()
{
	//alert("Into waitEthernetUP()");
	if ((waitEthernetUPTag == true) || StopWaitEthernetUP >= 2)
	{
		makeRequestWaitEthernetUP("/goform/guiWaitEthernetUP", "something");
		waitEthernetUPTag = false;
	}
	else
		StopWaitEthernetUP++;
	
	if (!StopWaitEthernetUP)
		setTimeout("waitEthernetUP()", 500);
}

function showField(tagName)
{
	objectDisplay("titleField", "block");
	objectDisplay("waitStatus", "block");
	objectDisplay("successField", "none");
	objectDisplay("failField", "none");
	objectDisplay("progressField", "none");
	document.getElementById("NonProc_ESSID").innerHTML = displayESSID;

	if (tagName != "")
	{
		objectDisplay("waitStatus", "none");
		//if ((tagName != "progressField") && (changeToCloseField > 1))
		if (tagName != "progressField")
		{
			objectDisplay(tagName, "block");
			
			if (tagName == "successField")
			{
				//document.getElementById("wpsSuccessButton").style.display="none";
				//waitEthernetUP();
				if (onceEvent)
				{
					onceEvent = false;
					//waitEthernetUP();
					//setTimeout("displayCloseButton(\"timeout\")", 70000);
				}
				cleanWPSTemp();
			}
			else if (tagName == "failField")
			{
				updateWPSStatusStopToWait = false;
				stopUpdateWPSStatus = true;
				waitEthernetUPTag = false;
				StopWaitEthernetUP = true;				
				document.getElementById("waitPad").style.display="none";
				cleanWPSFailTemp();
			}
		}
		else if (tagName != "progressField")
		{
			objectDisplay("progressField", "block");
			changeToCloseField++;
		}
		else
			objectDisplay(tagName, "block");
		
		objectDisplay("WPSPIN", "none");
		objectDisplay("WPSPBC", "none");
		if (wps_type == "pin")
			objectDisplay("WPSPIN", "block");
		else if (wps_type == "pbc")
			objectDisplay("WPSPBC", "block");		
	}
}

function timeoutCheck()
{
	setTimeout("displayCloseButton(\"timeout\")", 140000);
}

function easyCheckIpAddr(field)
{
	if (field == "") {
		return false;
	}

	if (isAllNum(field) == 0) {
		return false;
	}
	
	if ((!checkRange(field, 1, 0, 255)) ||
			(!checkRange(field, 2, 0, 255)) ||
			(!checkRange(field, 3, 0, 255)) ||
			(!checkRange(field, 4, 1, 254)))
	{
		return false;
	}

	return true;
}

function parsingRespone(msg)
{
	if (doRefresh)
		return;

	docTemp = msg;
	index = docTemp.indexOf(',');
	wps_info = docTemp.substr(0, index);
	/*if (wps_info == "NULL")
		top.location.href = "/station/wireless_basic.asp";*/
	
	docTemp = docTemp.substr(index+1);
	index = docTemp.indexOf(',');
	wps_type = docTemp.substr(0, index);
	
	docTemp = docTemp.substr(index+1);
	index = docTemp.indexOf(',');
	checkPercent = docTemp.substr(0, index);
	
	docTemp = docTemp.substr(index+1);
	index = docTemp.indexOf(',');
	getTemp = docTemp.substr(0, index);
	
	currentESSID = docTemp.substr(index+1);
	//alert("'" + currentESSID + "'");
	//if (currentESSID != "NULL")
	if (currentESSID != "")
		displayESSID = currentESSID; 

	switch(wps_info){
		//case "SEARCHING":
		case WPS_CURRENT_STAT_IDLE:
			showField("progressField");
			break;
		case WPS_CURRENT_STAT_SEARCHING:
			showField("progressField");
			break;
		//case "REGISTERING":
		case WPS_CURRENT_STAT_REGISTERING:
			showField("progressField");
			break;
		//case "CONNECTING":
		case WPS_CURRENT_STAT_CONNECTING:
			if (ipConfigMethod*1 == 0) //dhcp
				showField("progressField");
			else //static
				showField("successField");
			break;
		//case "ERROR_WALK_TIMEOUT":
		case WPS_CURRENT_STAT_ERROR_WALK_TIMEOUT:
			showField("failField");
			break;
		//case "ERROR_SESSION_TIMEOUT":
		case WPS_CURRENT_STAT_ERROR_SESSION_TIMEOUT:
			showField("failField");
			break;
		//case "ERROR_SESSION_OVERLAP":
		case WPS_CURRENT_STAT_ERROR_SESSION_OVERLAP:
			showField("failField");
			break;
		//case "ERROR_INVALID_PIN":
		case WPS_CURRENT_STAT_ERROR_INVALID_PIN:
			showField("failField");
			break;
		//case "CONNECTED":
		case WPS_CURRENT_STAT_CONNECTED:
			showField("successField");
			if (ipConfigMethod*1 == 0) //dhcp
				timeoutCheck();
			break;
	}

	if ((wps_info == "NULL") && (wps_type == "NULL"))
		top.location.href = "/station/wireless_basic.asp";
	graphProgress(wps_info);
	
	/*if (currentESSID != "NULL")
		alert("ipConfigMethod:" + ipConfigMethod + "; currentESSID=" + currentESSID + "; getTemp=" + getTemp);*/
		
	//if (((ipConfigMethod*1 == 1) && (currentESSID != "NULL")) || ((ipConfigMethod*1 == 0) && easyCheckIpAddr(getTemp) && (currentESSID != "NULL")))
	//if (((ipConfigMethod*1 == 1) && (currentESSID != "")) || ((ipConfigMethod*1 == 0) && easyCheckIpAddr(getTemp) && (currentESSID != "")))
	if ((ipConfigMethod*1 == 0) && (getTemp == "dhcp_timeout") && (currentESSID != ""))
	{
		successCheck = true;
		updateWPSStatusStopToWait = false;
		stopUpdateWPSStatus = true;
		setTimeout("displayCloseButton(\"timeout\")", 13000);
	}
	else if ((ipConfigMethod*1 == 0) && easyCheckIpAddr(getTemp) && (currentESSID != ""))
	{
		//alert("ipConfigMethod*1 == 0");
		if (getTemp != "NULL")
			currentIP = getTemp;
		
		updateWPSStatusStopToWait = false;
		stopUpdateWPSStatus = true;
		
		//alert("parsingRespone(): updateWPSStatusStopToWait:" + updateWPSStatusStopToWait + "; stopUpdateWPSStatus=" + stopUpdateWPSStatus);
		waitEthernetUP();
		setTimeout("displayCloseButton(\"timeout\")", 70000);
	}
	else if ((ipConfigMethod*1 == 1) && (currentESSID != ""))
	{
		//alert("ipConfigMethod*1 == 1");
		if (getTemp != "NULL")
			currentIP = getTemp;
		
		updateWPSStatusStopToWait = false;
		stopUpdateWPSStatus = true;
		successCheck = true;
		//displayCloseButton("ok");
		setTimeout("displayCloseButton(\"ok\")", 30000);
		cleanWPSFailTemp();
	}
	else
		updateWPSStatusStopToWait = true;

}

function makeRequestWPSStatus(url, content) {
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
		alert('Giving up :( Cannot create an XMLHTTP instance');
		return false;
	}
	http_request.onreadystatechange = function () {alertWaitDHCPInfoContents()};
	http_request.open('POST', url, true);
	http_request.send(content);
}

function alertWaitDHCPInfoContents() {
	if (http_request.readyState == 4) {
		if (http_request.status == 200) {
			if (applyParsing)
				parsingRespone(http_request.responseText);
				
			//updateWPSStatusStopToWait = true;
		} else {
			//alert('There was a problem with the request.');
		}
	}
}

function updateWPSStatus()
{
	//if (currentESSID != "NULL")
	if (currentESSID != "")
	{
		//alert("updateWPSStatus(): updateWPSStatusStopToWait:" + updateWPSStatusStopToWait + "; stopUpdateWPSStatus=" + stopUpdateWPSStatus);
	}
	
	if (updateWPSStatusStopToWait == true) {
		makeRequestWPSStatus("/goform/getCurrentWPSStatus", "something");
		updateWPSStatusStopToWait = false;
	}
	
	/*if (currentESSID != "NULL" && updateWPSStatusStopToWait == false && stopUpdateWPSStatus == false)
		updateWPSStatusStopToWait = true;*/
		
	if (!stopUpdateWPSStatus)
		setTimeout("updateWPSStatus()", 1000);
}

function blinkingWarningMSG(displayMsg, percent)
{
	if (displayMsg == "true")
	{
		document.getElementById("progressValue").innerHTML = Math.round(percent) + "%";
	}
	else
	{
		document.getElementById("progressValue").innerHTML = "";
		setTimeout("blinkingWarningMSG(\"true\", "+ Math.round(percent) +")", 100);
	}
}

function showProgress()
{
	document.getElementById("progress").style.width = Math.round(percent) + "%";
	document.getElementById("progressValue").innerHTML = Math.round(percent) + "%";
	setTimeout("blinkingWarningMSG(\"false\", " + Math.round(percent) +")", 500);
}

function graphProgress(wps_info)
{
	var loopCount=0;
	//var IDLE=0, SEARCHING=40, REGISTERING=65, CONNECTING=99, WPS_STATUS_CONNECTED=99, WPS_FAIL=99;
	var IDLE=0, SEARCHING=65, REGISTERING=65, CONNECTING=99, WPS_STATUS_CONNECTED=99, WPS_FAIL=99;
	
	switch(wps_info){
		case WPS_CURRENT_STAT_IDLE:
			if (checkPercent >= 99)
				percent = WPS_FAIL;
			else
			{
				if ((IDLE + (SEARCHING * (checkPercent/100))) <= 99)
					percent = IDLE + (SEARCHING * (checkPercent/100));
				else
					percent = 99;
			}
			showProgress();
			break;
		case WPS_CURRENT_STAT_SEARCHING:
			if (checkPercent >= 99)
				percent = WPS_FAIL;
			else
			{
				if ((IDLE + (SEARCHING * (checkPercent/100))) <= 99)
					percent = IDLE + (SEARCHING * (checkPercent/100));
				else
					percent = 99;
			}
			showProgress();
			break;
		//case "REGISTERING":
		case WPS_CURRENT_STAT_REGISTERING:
			if (checkPercent >= 99)
				percent = WPS_FAIL;
			else
			{
				if ((IDLE + (SEARCHING * (checkPercent/100))) <= 99)
					percent = IDLE + (SEARCHING * (checkPercent/100));
				else
					percent = 99;
			}
			/*if (checkPercent >= 99)
				percent = WPS_FAIL;
			else
			{
				if ((SEARCHING + (REGISTERING*(checkPercent/100))) <= 99)
					percent = SEARCHING + (REGISTERING*(checkPercent/100));
				else
					percent = 99;
			}*/
			showProgress();
			break;
		//case "CONNECTING":
		case WPS_CURRENT_STAT_CONNECTING:
			if (checkPercent >= 99)
				percent = WPS_FAIL;
			else
				if ((REGISTERING + ((CONNECTING-REGISTERING)*(checkPercent/100))) <= 99)
					percent = REGISTERING + ((CONNECTING-REGISTERING)*(checkPercent/100));
				else
					percent = 99;
			showProgress();
			break;
		//case "ERROR_WALK_TIMEOUT":
		case WPS_CURRENT_STAT_ERROR_WALK_TIMEOUT:
			percent = WPS_FAIL;
			showProgress();
			break;
		//case "ERROR_SESSION_TIMEOUT":
		case WPS_CURRENT_STAT_ERROR_SESSION_TIMEOUT:
			percent = WPS_FAIL;
			showProgress();
			break;
		//case "ERROR_SESSION_OVERLAP":
		case WPS_CURRENT_STAT_ERROR_SESSION_OVERLAP:
			percent = WPS_FAIL;
			showProgress();
			break;
		//case "ERROR_INVALID_PIN":
		case WPS_CURRENT_STAT_ERROR_INVALID_PIN:
			percent = WPS_FAIL;
			showProgress();
			break;
		//case "CONNECTED":
		case WPS_CURRENT_STAT_CONNECTED:
			percent = WPS_STATUS_CONNECTED;
			showProgress();
			break;
	}
	//showProgress();
}

function page_load() {
	ipConfigMethod = "<% getParam(1, "ip_config_method"); %>"; //0:dhcp, 1: static
	document.getElementById("NonProc_WPS_DevicePIN").innerHTML = "<% getParam(1, "NonProc_WPS_DevicePIN"); %>";
	showField("");
	fwUpgraceStatus("<% getFWUpgrade(); %>", "<% getCurrectLanIP(); %>");
	wpsStatus("<% getWPSStatus(); %>", "wps_status.asp");

	document.getElementById("waitPad").style.display="block";
	
	if(langset == "SA"){
		document.getElementById("progressTD").style.textAlign = "right";	
		document.getElementById("WPSPBC").style.textAlign = "right";
		document.getElementById("WPSPIN").style.textAlign = "right";
		document.getElementById("WPS_fail_id").style.textAlign = "right";
		document.getElementById("WPS_succeed_id").style.textAlign = "right";
	}
	
	updateWPSStatus();
	blinkingWaitMSG();
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
					<!--#tr id="mainmenutitle.1" -->Wireless<!--#endtr-->
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
									<td class="subMenuOption"><!--#tr id="w.submenu.1" -->Basic Wireless Settings<!--#endtr--></td>
									<td class="subMenuDIV">|</td>
									<td class="subMenuOption"><font class="small"><a href="/station/site_survey.asp"><!--#tr id="w.submenu.2" -->Wireless Network Site Survey<!--#endtr--></a></font></td>
									<td class="subMenuDIV">|</td>
									<td class="subMenuOption"><font class="small"><a href="/station/wmm.asp"><!--#tr id="w.submenu.3" -->WMM®<!--#endtr--></a></font></td>
									<td class="subMenuDIV">|</td>
									<td class="subMenuOption"><font class="small"><a href="/station/wireless_advanced.asp"><!--#tr id="w.submenu.4" -->Advanced Wireless Settings<!--#endtr--></a></font></td>
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
	<form method="post" name="" action="/goform/">
		<tr>
			<td class="noSPACE">
				<table class="mainTableContent" cellspacing="0">
					<tr id="titleField">
						<td class="subMenuMainContent" colspan="2"><!--#tr id="w.bws.1-3" -->Wi-Fi Protected Setup<!--#endtr--><sup style=\"font-size: 6pt;\">TM</sup></td>
						<td colspan="2" class="blankContent"></td>
					</tr>
					<tr id="waitStatus" style="display:none">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td colspan="2" class="blankContent">
							<font color="#FF0000"><!--#tr id="w.bws.asp.2" -->Please wait<!--#endtr--> ....</font>
						</td>
					</tr>
					<tr id="progressField" style="display:none">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td colspan="2" class="blankContent">
							<div id="WPSPBC" name="WPSPBC" style="font-size:11pt;width:430px;text-align:left;word-wrap:break-word;"><p><!--#tr id="w.bws.asp.12" -->Searching for your router (or access point).<!--#endtr--></p></p><!--#tr id="w.bws.asp.13" -->Please wait...<!--#endtr--></p><p><!--#tr id="w.bws.asp.14" -->If you haven't clicked on the Wi-Fi Protected Setup button on your router (or access point), please do so now.<!--#endtr--></p></div>
							<div id="WPSPIN" name="WPSPIN" style="font-size:11pt;width:430px;text-align:left;word-wrap:break-word;"><p><!--#tr id="w.bws.asp.15" -->Your bridge's PIN number is:<!--#endtr--><span id="NonProc_WPS_DevicePIN"></span></p><p><!--#tr id="w.bws.asp.16" -->Searching for your router (or access point).<!--#endtr--></p></p><!--#tr id="w.bws.asp.17" -->Please wait...<!--#endtr--></p></div><br />
							<table bolder="0"><tr><td colspan="2"></td></tr><tr><td width="93%"><table align="left" bgcolor="#ffffff" cellpadding="0" cellspacing="0" bordercolor="#FFFFFF" style="border-style: solid; border-width: 1px; border-color:#000000"><tr><td id="progressTD" width=427 align="left"><table id="progress" bgcolor="#00FF00" height="25"><tr><td></td></tr></table></td></tr></table></td><td width="95%"><font color=black><span id="progressValue">&nbsp;</span></font></td></tr></table>
						</td>
					</tr>
					<tr id="successField" style="display:none">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td colspan="2" class="blankContent">
							<table><tr><td><img src=/image/wps_ok.gif></src></td><td style=\"position:relative; top:10px\"><h2><!--#tr id="w.bws.asp.10" -->Congratulations!<!--#endtr--></h2></td></tr><tr><td></td><td style=\"position:relative; top:100px left:100px\"><div id="WPS_succeed_id" style="width:350px;text-align:left;word-wrap:break-word;"><!--#tr id="w.bws.asp.11" -->You had successful connect to SSID:<!--#endtr--><font color=blue><span id="NonProc_ESSID"></span></font>.</div></td></tr></table><br>
							<br><div id="wpsSuccessButton" align="center"><span id="writeSuccessButton"><font color="#FF0000"><!--#tr id="w.bws.asp.4" -->Please wait<!--#endtr-->.....</font></span></div>
						</td>
					</tr>
					<tr id="failField" style="display:none">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td colspan="2" class="blankContent">
							<table width="420px"><tr><td><img src=/image/wps_fail.gif></src></td><td style=\"position:relative; top:10px\"><h2><!--#tr id="w.bws.js.1" -->Connection Failure!<!--#endtr--></h2></td></tr><tr><td></td><td style=\"position:relative; top:100px left:100px\"><div id="WPS_fail_id" style="width:340px;text-align:left;word-wrap:break-word;"><!--#tr id="w.bws.js.2" -->This wireless bridge failed to connect to your router (or a access point).<!--#endtr--> <p><!--#tr id="w.bws.js.3" -->Refer back to your router and try again.<!--#endtr--></div></td></tr></table>
							<br><div align="center"><input type=button value="Close" onclick="returnWPS();"></div>
						</td>
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
