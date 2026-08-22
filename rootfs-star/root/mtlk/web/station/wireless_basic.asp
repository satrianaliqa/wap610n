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
<script type="text/javascript" src="../func.js"></script>
<script type="text/javascript" src="../u-media.js"></script>
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
var LanIP = "<% getParam(1, "ip_lan");%>";
var updateStatusCount=0;
var redirectUrlPath;

function basicDisplay(value) {
	//objectDisplay("configTypeField", value);
	//objectDisplay("configTypeSeparate", value);
	objectDisplay("freqBand", value);
	objectDisplay("ESSIDFeild", value);
	objectDisplay("securityModeField", value);
	objectDisplay("RFSetSeparate", value);
	objectDisplay("manualTitle", value);
	objectDisplay("securitySeparate", value);
}

function WPSDisplay(value) {
	//objectDisplay("configTypeField", value);
	//objectDisplay("configTypeSeparate", value);
	objectDisplay("WPSTitleField", value);
	objectDisplay("WPSContentField", value);
	objectDisplay("WPSSapceField1", value);
	objectDisplay("WPSAPPBCField", value);
	objectDisplay("WPSORField1", value);
	objectDisplay("WPSSapceField2", value);
	objectDisplay("WPSAPPINField", value);
	objectDisplay("WPSSeparate", value);
	objectDisplay("WPSStatusField", value);
}

function mainDisplay(value) {
	objectDisplay("titleField", value);
	objectDisplay("outputField", value);
	objectDisplay("configTypeField", value);
	objectDisplay("configTypeSeparate", value);
	objectDisplay("freqBand", value);
	objectDisplay("ESSIDFeild", value);
	objectDisplay("securityModeField", value);
	objectDisplay("RFSetSeparate", value);
	objectDisplay("manualTitle", value);
	objectDisplay("securitySeparate", value);
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

function changeImage(position)
{
	if (position == "moveIn")
		document.getElementById("wpsPBCImage").src = "../image/WFA_WPS_Mark_Solo1.gif";
	else if (position == "moveOut")
		document.getElementById("wpsPBCImage").src = "../image/WFA_WPS_Mark_Solo.gif";
	else if (position == "moveDown")
	{
		document.getElementById("wpsPBCImage").src = "../image/wps_nonselectable.gif";
		checkWPS("getPBC");
	}
}

function selectConfigType(control)
{
	if (control =="submit")
	{
		if (document.getElementById("selectManual").checked == true)
			document.getElementById("wirelessConfigType").value = "manual";
		else if (document.getElementById("selectWPS").checked == true)
			document.getElementById("wirelessConfigType").value = "wps";
			
		document.wirelessTypeSet.submit();
	}
	else
	{
		WEPDisplay("none");
		WPAPersonalDisplay("none");
		
		if (document.getElementById("selectManual").checked == true)
		{
			document.getElementById("wirelessType").innerHTML = "<!--#tr id=\"w.bws.1\" -->Basic Wireless Settings<!--#endtr-->";
			//document.getElementById("wirelessType").innerHTML = _("w.bws.1");
			document.getElementById("wirelessConfigType").value = "manual";
			WPSDisplay("none");
			basicDisplay("block");
		
		}
		else if (document.getElementById("selectWPS").checked == true)
		{
			document.getElementById("wirelessType").innerHTML = "<!--#tr id=\"w.bws.1-3\" -->Wi-Fi Protected Setup<!--#endtr-->" + "<sup style=\"font-size: 6pt;\">TM</sup>";
			//document.getElementById("wirelessType").innerHTML = _("w.bws.1-3") + "<sup style=\"font-size: 6pt;\">TM</sup>";
			document.getElementById("wirelessConfigType").value = "wps";
			basicDisplay("none");
			WPSDisplay("block");
		}
	}	
}

function selectSecurityMode(mode) {
	WEPDisplay("none");
	WPAPersonalDisplay("none");

	var selectedIndex = document.getElementById("securityMode").selectedIndex;
	var modeName = document.getElementById("securityMode").options[selectedIndex].id;

	if (mode*1 == 1) {
	
	}
	else if (mode*1 == 2) {
		WEPDisplay("block");
	}
	else if (mode*1 == 3) {
		WPAPersonalDisplay("block");
		if (modeName == "wpa")
		{
			selectWPAMode(1);
			objectDisplay("WPAPersonalEncryptionField", "none");
		}
		else if (modeName == "wpa2")
		{
			selectWPAMode(2);
		}
	}
}

function selectRFBand() {
	var channelList;
	var value = document.getElementById("frequencyBand").value;
	var origIndex = document.getElementById("channel").selectedIndex;
	//alert("jacky - origIndex:"+origIndex);
	removeAllOption("channel");

	if (value == "0") {
		if ((document.getElementById("channelBounding").value == 0)) {
			//channelList = pvChannel52FCCnoCB;
			channelList = pvChannel52FCCnoCB;
		}
		else if ((document.getElementById("channelBounding").value == 1) && (document.getElementById("upperLower").value == 0)) {
			//channelList = pvChannel52FCCCBU;
			channelList = pvChannel52FCCCBU;
		}
		else if ((document.getElementById("channelBounding").value == 1) && (document.getElementById("upperLower").value == 1)) {
			//channelList = pvChannel52FCCCBL;
			channelList = pvChannel52FCCCBL;
		}
	
		for (loopCount=0; loopCount<channelList.length; loopCount=loopCount+2)
		{
			addNewOption("channel", channelList[loopCount], channelList[loopCount], channelList[loopCount], channelList[loopCount+1]);
		}
		if (origIndex != -1)
			document.getElementById("channel").options[origIndex].selected = true;
	}
	else if (value == "1") {
		if ((document.getElementById("channelBounding").value == 0)) {
			//channelList = pvChannel24ETCInoCB;
			channelList = pvChannel24FCCnoCB;
		}
		else if ((document.getElementById("channelBounding").value == 1) && (document.getElementById("upperLower").value == 0)) {
			//channelList = pvChannel24ETCICBU;
			channelList = pvChannel24FCCCBU;
		}
		else if ((document.getElementById("channelBounding").value == 1) && (document.getElementById("upperLower").value == 1)) {
			//channelList = pvChannel24ETCICBL;
			channelList = pvChannel24FCCCBL;
		}
	
		for (loopCount=0; loopCount<channelList.length; loopCount=loopCount+2)
		{
			addNewOption("channel", channelList[loopCount], channelList[loopCount], channelList[loopCount], channelList[loopCount+1]);
		}
		if (origIndex != -1)
			document.getElementById("channel").options[origIndex].selected = true;
	}
	else if (value == "both") {
		//alert("both");
	}
}

function selectMode(mode){
	//selectRFBand(mode);
	if (mode*1 == 0) //STA mode
	{
		WEPDisplay("block");
		objectDisplay("WPAPSKField", "block");
		
		var wpsEnabled = "<% getParam(1, "NonProc_WPS_ActivateWPS"); %>";
		var securityMode = "<% getParam(1, "NonProcSecurityMode"); %>";
		var WEPKey0 = "<% getParam(1, "WepKeys_DefaultKey0"); %>";
		var WEPKey1 = "<% getParam(1, "WepKeys_DefaultKey1"); %>";
		var WEPKey2 = "<% getParam(1, "WepKeys_DefaultKey2"); %>";
		var WEPKey3 = "<% getParam(1, "WepKeys_DefaultKey3"); %>";
		var defaultWEPKey = "<% getParam(1, "WepTxKeyIdx"); %>";
		var WEPKeyLength = "<% getParam(1, "NonProc_WepKeyLength"); %>";
		var WEPAuth = "<% getParam(1, "NonProc_Authentication"); %>";
		var WPAPersonalMode = "<% getParam(1, "NonProc_WPA_Personal_Mode"); %>";
		var WPAPSK = "<% getParam(1, "NonProc_WPA_Personal_PSK"); %>";
		
		/*docTemp = document.getElementById("securityMode");
		for (loopCount=0; loopCount<docTemp.length; loopCount++)
		{
			if (docTemp.options[loopCount].value == securityMode) {
				docTemp.options[loopCount].selected = true;
			}
		}*/
		//selectSecurityMode(securityMode*1);
		
		if (securityMode*1 != 3) {
			docTemp = document.getElementById("securityMode");
			for (loopCount=0; loopCount<docTemp.length; loopCount++)
			{
				if (docTemp.options[loopCount].value == securityMode) {
					docTemp.options[loopCount].selected = true;
				}
			}
			selectSecurityMode(securityMode*1);
		}
		else {
			if (WPAPersonalMode == 1) //WPA
			{
				document.getElementById("securityMode").options[2].selected = true;
				selectSecurityMode(securityMode*1);
				document.getElementById("WPAPersonalEncryption").options[0].selected = true;
			}
			else if (WPAPersonalMode == 2) //WPA2
			{
				document.getElementById("securityMode").options[3].selected = true;
				selectSecurityMode(securityMode*1);
				document.getElementById("WPAPersonalEncryption").options[0].selected = true;
			}
			else if (WPAPersonalMode == 3) //WPA2 Mixed
			{
				document.getElementById("securityMode").options[3].selected = true;
				selectSecurityMode(securityMode*1);
				document.getElementById("WPAPersonalEncryption").options[1].selected = true;
			}
		}
		//alert(document.getElementById("securityMode").selectedIndex);
		
		if (securityMode == 2) //WEP
		{
			if (WEPKeyLength == 128)
				document.getElementById("WEPKeyLength").options[0].selected = true;
			else if (WEPKeyLength == 64)
				document.getElementById("WEPKeyLength").options[1].selected = true;
			selectWEPKeyLength(WEPKeyLength*1);
			
			if (WEPKey0.substr(0,2) == "0x")
				document.getElementById("WEPKey0").value = WEPKey0.substr(2);
			else
				document.getElementById("WEPKey0").value = WEPKey0;
		
			if (WEPKey1.substr(0,2) == "0x")
				document.getElementById("WEPKey1").value = WEPKey1.substr(2);
			else
				document.getElementById("WEPKey1").value = WEPKey1;
		
			if (WEPKey2.substr(0,2) == "0x")
				document.getElementById("WEPKey2").value = WEPKey2.substr(2);
			else
				document.getElementById("WEPKey2").value = WEPKey2;
		
			if (WEPKey3.substr(0,2) == "0x")
				document.getElementById("WEPKey3").value = WEPKey3.substr(2);
			else
				document.getElementById("WEPKey3").value = WEPKey3;
			
			//document.getElementById("defaultWEPKey").options[defaultWEPKey].selected = true;
			for (loopCount=0; loopCount<3; loopCount++)
				if (document.getElementById("WEPAuth").options[loopCount].value*1 == WEPAuth*1)
					document.getElementById("WEPAuth").options[loopCount].selected = true;
			
		}
		else if (securityMode == 3) //WPA
		{
			//document.getElementById("WPAPSK").value = WPAPSK;
			document.getElementById("display_WPAPSK").value = transSSID(WPAPSK);
		}
	}
}

function selectBonding(HTEnable, changeRF) {
	if (HTEnable == 0)
		objectDisplay("offsetField", "none");
	else
		objectDisplay("offsetField", "block");
	if (changeRF == true)
		selectRFBand();
}

function checkValue() {
	var applyValue = true;
	var checkValue, keyLenCount, defaultWEPKeyValue;
	var hexKey0 = false;
	var hexKey1 = false;
	var hexKey2 = false;
	var hexKey3 = false;
		
	if (!translateSubmitToHex("ESSID")){
		applyValue = false;
	}
	
	if (applyValue && (document.getElementById("securityMode").value == 2)) {
		//defaultWEPKeyValue = document.getElementById("defaultWEPKey").value * 1;
		defaultWEPKeyValue = 0; //always use key 1
		if (document.getElementById("WEPKeyLength").value == 64) {
			for (loopCount=0; loopCount<4; loopCount++) {
				applyValue = true;
				docTemp = document.getElementById("WEPKey"+loopCount).value;

				if ((docTemp.length != 0) /*|| (document.getElementById("WEPKey"+defaultWEPKeyValue).value == 0)*/) {
					/*if (docTemp.length == 5)	//ASCII
					{
						applyValue = false;
					}*/
					//else if (docTemp.length == 10)	//HEX
					if (docTemp.length == 10)	//HEX
					{

						for (keyLenCount=0; keyLenCount<10; keyLenCount++)
						{

							checkValue = docTemp.substr(keyLenCount, 1);
							if (!(parseInt(checkValue, 16) >= 0) && !(parseInt(checkValue, 16) <= 15)) {

								if (document.getElementById("WEPKey"+defaultWEPKeyValue).value == 0) {
									alert("<!--#tr id=\"w.bws.alert.1\" -->Please input 10 Hex character of key <!--#endtr-->" + (defaultWEPKeyValue+1) + "!");
									//alert(_("w.bws.alert.1") + (defaultWEPKeyValue+1) + "!");
								} else {
									alert("<!--#tr id=\"w.bws.alert.1\" -->Please input 10 Hex character of key <!--#endtr-->" + (loopCount+1) + "!");
									//alert(_("w.bws.alert.1") + (loopCount+1) + "!");
								}
								applyValue = false;
								loopCount = 5;
								keyLenCount = 10;
							}
						}
						if (applyValue) {
							switch(loopCount) {
								case 0: hexKey0 = true;
										break;
								case 1: hexKey1 = true;
										break;
								case 2: hexKey2 = true;
										break;
								case 3: hexKey3 = true;
										break;
							}
						}
					}
					else
					{
						//alert("Please input 5 ascii characters or 10 hex digits for WEP Key " +?(loopCount+1) + "!");
						if (document.getElementById("WEPKey"+defaultWEPKeyValue).value == 0) {
							alert("<!--#tr id=\"w.bws.alert.1\" -->Please input 10 Hex character of key <!--#endtr-->" + (defaultWEPKeyValue+1) + "!");
							//alert(_("w.bws.alert.1") + (defaultWEPKeyValue+1) + "!");
						} else {
							alert("<!--#tr id=\"w.bws.alert.1\" -->Please input 10 Hex character of key <!--#endtr-->" + (loopCount+1) + "!");
							//alert(_("w.bws.alert.1") + (loopCount+1) + "!");
						}
						applyValue = false;
						loopCount = 5;
					}
				}
			}
		}
		else if (applyValue && (document.getElementById("WEPKeyLength").value == 128)) {
			for (loopCount=0; loopCount<4; loopCount++) {
				applyValue = true;
				docTemp = document.getElementById("WEPKey"+loopCount).value;
				
				if ((docTemp.length != 0) /*|| (document.getElementById("WEPKey"+defaultWEPKeyValue).value == 0)*/) {
					/*if (docTemp.length == 13)	//ASCII
					{
						applyValue = false;
					}*/
					//else if (docTemp.length == 26)	//HEX
					if (docTemp.length == 26)	//HEX
					{
						for (keyLenCount=0; keyLenCount<26; keyLenCount++)
						{
							checkValue = docTemp.substr(keyLenCount, 1);
							if (!(parseInt(checkValue, 16) >= 0) && !(parseInt(checkValue, 16) <= 15)) {
								if (document.getElementById("WEPKey"+defaultWEPKeyValue).value == 0) {
									alert("<!--#tr id=\"w.bws.alert.2\" -->Please input 26 Hex character of key <!--#endtr-->" + (defaultWEPKeyValue+1));
									//alert(_("w.bws.alert.2") + (defaultWEPKeyValue+1));
								} else {
									alert("<!--#tr id=\"w.bws.alert.2\" -->Please input 26 Hex character of key <!--#endtr-->" + (loopCount+1));
									//alert(_("w.bws.alert.2") + (loopCount+1));
								}
								applyValue = false;
								loopCount = 13;
								keyLenCount = 26;
							}
						}
						if (applyValue) {
							switch(loopCount) {
								case 0: hexKey0 = true;
										break;
								case 1: hexKey1 = true;
										break;
								case 2: hexKey2 = true;
										break;
								case 3: hexKey3 = true;
										break;
							}
						}
					}
					else
					{
						//alert("Please input 13 ascii characters or 26 hex digits for WEP Key " +?(loopCount+1) + "!");
						if (document.getElementById("WEPKey"+defaultWEPKeyValue).value == 0) {
							alert("<!--#tr id=\"w.bws.alert.2\" -->Please input 26 Hex character of key <!--#endtr-->" + (defaultWEPKeyValue+1));
							//alert(_("w.bws.alert.2") + (defaultWEPKeyValue+1));
						} else {
							alert("<!--#tr id=\"w.bws.alert.2\" -->Please input 26 Hex character of key <!--#endtr-->" + (loopCount+1));
							//alert(_("w.bws.alert.2") + (loopCount+1));
						}
						applyValue = false;
						loopCount = 13;
					}
				}
			}
		}
	}
	
	if (applyValue && (document.getElementById("securityMode").value == 3)) {
		if (applyValue && document.getElementById("WPAPersonalEncryption").value == 0)
		{
			if (!confirm("<!--#tr id=\"HT.confirm\" -->WARNING: Your Wireless-N devices will only operate at Wireless-G data rates with the encryption type you have chosen.? Please select WPA2-Personal encryption if you would like to have your Wireless-N devices operating at full data rates.? Click OK to continue with this selection, or Cancel to choose another type of encryption.<!--#endtr-->"))
				applyValue = false;
		}
	
		if (applyValue && !checkWPAPSK(document.getElementById("display_WPAPSK").value)){
			applyValue = false;
		}
		if (applyValue)
		{
			docTemp = document.getElementById("display_WPAPSK").value;
			if (checkWPAPSK(docTemp.length == 64))
				document.getElementById("wpaToHex").value = "hex";
		}
	}

	if (applyValue) {		
		//totalWaitTime = 40; //second
		//wait_page();
		document.getElementById("waitPad").style.display="block";
		//top.location.href = "http://169.254.1.250/wait_page.asp";
		document.modeOption.submit();
	}
}

//function returnWPS(currentIP)
function returnWPS()
{
	//alert("returnWPS: " + currentIP);
	//if(!checkIpAddr(currentIP, false, "Network IP Address"))
	if (currentIP != "")
		window.location = "http://" + currentIP + "/station/wireless_basic.asp";
	else
		window.location = "/station/wireless_basic.asp";
}

function submitReset() {
	window.location = "/station/wireless_basic.asp?startWPS=1&action_type=reset";
	//Jacky.Yang 19-Aug-2008, that will crash.
	//objectDisplay("resetField", "none");
	objectDisplay("waitPad", "block");
}

function checkWPS(funcName) {
	if (funcName == "getPIN") {
		//document.getElementById("networkType").value = "STA";
		document.startWPS.networkType.value = "STA";
		//document.getElementById("nowMode").value = "getPin";
		document.startWPS.nowMode.value = "getPin";
		document.getElementById("waitPad").style.display="block";
	}
	else if (funcName == "getPBC") {
		//document.getElementById("networkType").value = "STA";
		document.startWPS.networkType.value = "STA";
		//document.getElementById("nowMode").value = "getPBC";
		document.startWPS.nowMode.value = "getPBC";
		document.getElementById("waitPad").style.display="block";
	}
	document.startWPS.submit();
}

function waitStatus()
{	
	if (stopToWait)
	{
		//if (document.getElementById("listWPSStatus").innerHTML == "Connected")
		/*if (document.getElementById("listWPSStatus").innerHTML.indexOf("Connected") != -1)
		{
			//objectDisplay("WPSStatusESSID", "block");
			//objectDisplay("WPSStatusSecurity", "block");
		}*/
		
	}
	else
	{
		window.setTimeout("waitStatus()", 1000);
		//objectDisplay("WPSStatusESSID", "none");
		//objectDisplay("WPSStatusSecurity", "none");
	}

}

function updateWPSStatus(){
	makeRequest("/goform/updateWPSStatus", "something", "listWPSStatus", "listWPSTemp");
}

function page_load() {
	fwUpgraceStatus("<% getFWUpgrade(); %>", "<% getCurrectLanIP(); %>");
	wpsStatus("<% getWPSStatus(); %>", "");
	
	var mode = "<% getParam(1, "network_type"); %>";
	var wirelessConfigType = "<% getParam(1, "wirelessConfigType"); %>";
	var FrequencyBand = "<% getParam(1, "FrequencyBand"); %>";
	var country = "<% getParam(1, "Country"); %>";
	var channel = "<% getParam(1, "Channel");%>";
	var channelBonding = "<% getParam(1, "ChannelBonding");%>";
	var upperLower = "<% getParam(1, "UpperLowerChannelBonding");%>";
	var ESSID = "<% getParam(1, "NonProc_ESSID");%>";
	var securityMode = "<% getParam(1, "NonProcSecurityMode"); %>";
	
	mainDisplay("block");
	WEPDisplay("none");
	WPAPersonalDisplay("none");
	selectConfigType();
	
	if (wirelessConfigType == "manual")
	{
		document.getElementById("selectManual").checked = true;
		document.getElementById("wirelessType").innerHTML = "<!--#tr id=\"w.bws.1\" -->Basic Wireless Settings<!--#endtr-->";
		//document.getElementById("wirelessType").innerHTML = _("w.bws.1");
		document.getElementById("wirelessConfigType").value = "manual";
		WPSDisplay("none");
		basicDisplay("block");

		if (mode*1 == 0 || mode*1 == 2) // Both STA and AP mode (WAP610N)
		{
			document.modeOption.nowMode.value = (mode*1 == 2) ? "apMode" : "stationMode";
			selectMode(mode*1);
			removeAllOption("frequencyBand");
			addNewOption("frequencyBand", "", "", "0", "5GHz");
			addNewOption("frequencyBand", "", "", "1", "2.4GHz");
			addNewOption("frequencyBand", "", "", "2", "Both");
			if (FrequencyBand && document.getElementById("frequencyBand").options[FrequencyBand])
				document.getElementById("frequencyBand").options[FrequencyBand].selected = true;
		}
		document.getElementById("display_ESSID").value = transSSID(ESSID);
	}
	else
	{
		//WPS
		waitStatus();
		updateWPSStatus();
		var WPAPersonalMode = "<% getParam(1, "NonProc_WPA_Personal_Mode"); %>";
		var pinCode = "<% getParam(1, "NonProc_WPS_DevicePIN"); %>";

		document.getElementById("selectWPS").checked = true;
		document.getElementById("wirelessType").innerHTML = "<!--#tr id=\"w.bws.1-3\" -->Wi-Fi Protected Setup<!--#endtr-->" + "<sup style=\"font-size: 6pt;\">TM</sup>";
		//document.getElementById("wirelessType").innerHTML = _("w.bws.1-3") + "<sup style=\"font-size: 6pt;\">TM</sup>";
		document.getElementById("wirelessConfigType").value = "wps";
		basicDisplay("none");
		WPSDisplay("block");

		objectDisplay("saveField", "none");
	
		document.getElementById("WPSDevicePIN").innerHTML = pinCode;
		//document.getElementById("WPSStatusESSID").innerHTML = transSSID(ESSID);		
		/*if (securityMode == 1)
			document.getElementById("WPSStatusSecurity").innerHTML = "<!--#tr id=\"w.bws.2-2\" -->Disabled<!--#endtr-->";
			//document.getElementById("WPSStatusSecurity").innerHTML = _("w.bws.2-2");
		else if (securityMode == 2)
			document.getElementById("WPSStatusSecurity").innerHTML = "<!--#tr id=\"w.bws.2-3\" -->WEP<!--#endtr-->";
			//document.getElementById("WPSStatusSecurity").innerHTML = _("w.bws.2-3");
		else if (securityMode == 3)
		{
			if (WPAPersonalMode == 1)
				document.getElementById("WPSStatusSecurity").innerHTML = "<!--#tr id=\"w.bws.2-4\" -->WPA Personal<!--#endtr-->";
				//document.getElementById("WPSStatusSecurity").innerHTML = _("w.bws.2-4");
			else if (WPAPersonalMode == 2 || WPAPersonalMode == 3)
				document.getElementById("WPSStatusSecurity").innerHTML = "<!--#tr id=\"w.bws.2-5\" -->WPA2 Personal<!--#endtr-->";
				//document.getElementById("WPSStatusSecurity").innerHTML = _("w.bws.2-5");
		}*/
	}
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
									<td class="subMenuOption">Basic Wireless Settings</td>
									<td class="subMenuDIV">|</td>
									<td class="subMenuOption"><font class="small"><a href="/station/wps_status.asp">Wi-Fi Protected Setup™</a></font></td>
									<td class="subMenuDIV">|</td>
									<td class="subMenuOption"><font class="small"><a href="/station/site_survey.asp">Wireless Network Site Survey</a></font></td>
									<td class="subMenuDIV">|</td>
									<td class="subMenuOption"><font class="small"><a href="/station/wmm.asp">WMM®</a></font></td>
									<td class="subMenuDIV">|</td>
									<td class="subMenuOption"><font class="small"><a href="/station/wireless_advanced.asp">Advanced Wireless Settings</a></font></td>
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
					<tr id="titleField">
						<td id="wirelessType" class="subMenuMainContent" colspan="2"><!--#tr id="w.bws.1" -->Basic Wireless Settings<!--#endtr--></td>
						<td colspan="2" class="blankContent"></td>
					</tr>
					<tr id="outputField">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td colspan="2" class="blankContent">
							<iframe class="rebootRedirect" name="rebootRedirect" id="rebootRedirect" frameborder="0" width="1" height="1" scrolling="yes" src="">redirect</iframe>
							<div id="waitform"></div>
						</td>
					</tr>
				</table>
				<table id="mainform" class="mainTableContent" cellspacing="0">
					<form method="post" name="wirelessTypeSet" action="/goform/wirelessTypeSet">
					<tr id="configTypeField">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><!--#tr id="w.bws.1-1" -->Configuration View:<!--#endtr--></td>
						<td class="tdContent">
							<input type="hidden" id="wirelessConfigType" name="wirelessConfigType">
							<input type="radio" id="selectManual" name="configType" onclick="selectConfigType('submit');"><!--#tr id="w.bws.1-2" -->Manual<!--#endtr-->
							<input type="radio" id="selectWPS"  name="configType" onclick="selectConfigType('submit');"><!--#tr id="w.bws.1-3" -->Wi-Fi Protected Setup<!--#endtr--><sup style="font-size: 6pt;">TM</sup>
						</td>
					</tr>
					</form>
					<tr id="configTypeSeparate">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="blankContent" colspan="2"><hr></td>
					</tr>
					<form method="post" name="modeOption" action="/goform/wirelessBasic">
					<tr id="freqBand">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><!--#tr id="w.bws.1-4" -->Frequency Band:<!--#endtr--></td>
						<td class="tdContent">
							<input type="hidden" id="nowMode" name="nowMode" value="">
							<input type="hidden" id="wpaToHex" name="wpaToHex" value="">
							<select id="frequencyBand" name="frequencyBand" onchange="selectRFBand();">
								<option id="5GHz" value="0">5 GHz</option>
								<option id="2.4GHz" value="1">2.4 GHz</option>
								<option id="both" value="both">Both</option>
							</select>						</td>
					</tr>
					<tr id="ESSIDFeild">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><!--#tr id="w.bws.1-8" -->Network Name (SSID):<!--#endtr--></td>
						<td class="tdContent">
							<input type="hidden" id="submit_ESSID" name="submit_ESSID" value="">
							<input type="text" id="display_ESSID" name="ESSID" size="33" maxlength="32" value="">
						</td>
					</tr>
					<tr id="RFSetSeparate">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="blankContent" colspan="2"><hr></td>
					</tr>
					<tr id="manualTitle">
						<td class="subMenuMainContent" colspan="2"><!--#tr id="w.bws.2" -->Wireless Security<!--#endtr--></td>
						<td colspan="2" class="blankContent"></td>
					</tr>
					<tr id="securityModeField">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><!--#tr id="w.bws.2-1" -->Security Mode:<!--#endtr--></td>
						<td class="tdContent">
						<select id="securityMode" name="securityMode" onchange="selectSecurityMode(this.value)">
                          <option id="open" value="1" selected> <!--#tr id="w.bws.2-2" -->Disabled<!--#endtr--></option>
                          <option id="wep" value="2"><!--#tr id="w.bws.2-3" -->WEP<!--#endtr--></option>
                          <option id="wpa" value="3"><!--#tr id="w.bws.2-4" -->WPA Personal<!--#endtr--></option>
                          <option id="wpa2" value="3"><!--#tr id="w.bws.2-5" -->WPA2 Personal<!--#endtr--></option>
                        </select></td>
					</tr>
					<tr id="securitySeparate" style="display:none">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="blankContent" colspan="2"><hr></td>
					</tr>
					<tr id="WEPKeyLengthField" style="display:none">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><!--#tr id="w.bws.2-6" -->Encryption:<!--#endtr--></td>
						<td class="tdContent">
							<select id="WEPKeyLength" name="WEPKeyLength" onchange="selectWEPKeyLength(this.value);">
								<option value="128"><!--#tr id="w.bws.2-7" -->104 / 128-bit (26 hex digits)<!--#endtr--></option>
								<option value="64" selected><!--#tr id="w.bws.2-8" -->40 / 64-bit (10 hex digits)<!--#endtr--></option>
							</select>						</td>
					</tr>
					<tr id="wepPassphraseField" style="display:none">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><!--#tr id="w.bws.2-9" -->Passphrase:<!--#endtr--></td>
						<td class="tdContent"><input type="text" id="wepPassphrase" name="wepPassphrase" size="32" maxlength="32" value="" onkeyup="Update_WEPKey(this.value);"></td>
					</tr>
					<tr id="WEPKey0Field" style="display:none">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><!--#tr id="w.bws.2-10" -->Key 1:<!--#endtr--></td>
						<td class="tdContent"><input type="text" id="WEPKey0" name="WEPKey0" size="27" maxlength="10" value="" /></td>
					</tr>
					<tr id="WEPKey1Field" style="display:none">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><!--#tr id="w.bws.2-11" -->Key 2:<!--#endtr--></td>
						<td class="tdContent"><input type="text" id="WEPKey1" name="WEPKey1" size="27" maxlength="10" value=""></td>
					</tr>
					<tr id="WEPKey2Field" style="display:none">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><!--#tr id="w.bws.2-12" -->Key 3:<!--#endtr--></td>
						<td class="tdContent"><input type="text" id="WEPKey2" name="WEPKey2" size="27" maxlength="10" value=""></td>
					</tr>
					<tr id="WEPKey3Field" style="display:none">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><!--#tr id="w.bws.2-13" -->Key 4:<!--#endtr--></td>
						<td class="tdContent"><input type="text" id="WEPKey3" name="WEPKey3" size="27" maxlength="10" value=""></td>
					</tr>
					<tr id="defaultWEPKeyField" style="display:none">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><!--#tr id="w.bws.2-14" -->Tx Key:<!--#endtr--></td>
						<td class="tdContent"><input type="hidden" id="defaultWEPKey" name="defaultWEPKey" value="0">
							<!--#tr id="w.bws.2-15" -->Key 1<!--#endtr-->
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
						<td class="tdLabel"><!--#tr id="w.bws.2-16" -->Authentication:<!--#endtr--></td>
						<td class="tdContent">
							<select id="WEPAuth" name="WEPAuth" onchange="">
								<option value="1"><!--#tr id="w.bws.2-17" -->Open<!--#endtr--></option>
								<option value="2"><!--#tr id="w.bws.2-18" -->Shared<!--#endtr--></option>
								<option value="3" selected><!--#tr id="w.bws.2-19" -->Auto<!--#endtr--></option>
							</select>						</td>
					</tr>
					<tr id="WPAPersonalEncryptionField">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><!--#tr id="w.bws.2-20" -->Encryption:<!--#endtr--></td>
						<td class="tdContent">
							<select id="WPAPersonalEncryption" name="WPAPersonalEncryption" onchange="">
								<option value="0"><!--#tr id="w.bws.2-21" -->TKIP<!--#endtr--></option>
								<option value="1"><!--#tr id="w.bws.2-22" -->AES<!--#endtr--></option>
								<option value="2" selected><!--#tr id="w.bws.2-23" -->TKIP or AES<!--#endtr--></option>
							</select>						</td>
					</tr>
					<tr id="WPAPSKField">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><!--#tr id="w.bws.2-24" -->Passphrase:<!--#endtr--></td>
						<td class="tdContent">
							<!--<input type="text" id="WPAPSK" name="WPAPSK" size="48" maxlength="64" value="">-->
							<input type="hidden" id="submit_WPAPSK" name="submit_WPAPSK" value="">
							<input type="text" id="display_WPAPSK" name="WPAPSK" size="48" maxlength="64" value="">
						</td>
					</tr>
					</form>
					<!-- WPS -->
					<form method="get" name="startWPS" action="/goform/startWPS">
					<tr id="WPSTitleField" style="display:none;">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="WPSTitle" colSpan="2">
							<input type="hidden" id="nowMode" name="nowMode" value="">
							<input type="hidden" id="networkType" name="networkType" value="">
							<!--#tr id="w.bws.2-25" -->Wi-Fi Protected Setup<!--#endtr--><sup style="font-size: 8pt;">TM</sup>
						</td>
					</tr>
					<tr id="WPSContentField" style="display:none;">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="blankContent" colSpan="2">
							<!--#tr id="w.bws.2-26" -->Use one of the following methods if your router (or access point) supports Wi-Fi Protected Setup:<!--#endtr-->
						</td>
					</tr>
					<tr id="WPSSapceField1" style="display:none;">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="blankContent" colSpan="2">&nbsp;</td>
					</tr>
					<tr id="WPSAPPBCField" style="display:none;">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="blankContent" colSpan="2">
							<table border="0"><tr>
								<td width="250px">
								<!--#tr id="w.bws.2-27" -->1. If your router has a Wi-Fi Protected Setup button, click or press that button, and then click the button on the right.<!--#endtr-->
								</td>
								<td style="padding-left:10px">
									<img id="wpsPBCImage" src="../image/WFA_WPS_Mark_Solo.gif" onmousemove="changeImage('moveIn');" onmouseout="changeImage('moveOut');" onmousedown="changeImage('moveDown');">
								</td>
							</tr></table>
						</td>
					</tr>
					<tr id="WPSORField1" style="display:none;">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="WPSOR" colSpan="2"><!--#tr id="w.bws.2-28" -->OR<!--#endtr--></td>
					</tr>
					<tr id="WPSSapceField2" style="display:none;">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="blankContent" colSpan="2">&nbsp;</td>
					</tr>
					<tr id="WPSAPPINField" style="display:none;">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="blankContent" colSpan="2">
						<!--#tr id="w.bws.2-29" -->2. If your router asks for the client device's PIN number, enter this number<!--#endtr-->&nbsp;<strong><span id="WPSDevicePIN"></span></strong>&nbsp;<!--#tr id="w.bws.2-30" -->in your router and then click<!--#endtr-->&nbsp;
						<input type="button" id="Get_configured_via_pin" name="Get_configured_via_pin" value="OK" onclick="checkWPS('getPIN');" /></td>
					</tr>
					<tr id="WPSSeparate">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="blankContent" colspan="2"><hr></td>
					</tr>
					</form>
				</table>
				<table id="WPSStatusField" class="mainTableContent" cellspacing="0" style="display:none">
					<tr>
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td colspan="2">
							<span id="listWPSStatus"></span>
							<table id="listWPSTemp">
								<tr><td class="WPSStatus"><!--#tr id="w.bws.2-31" -->Link Status:<!--#endtr--></td>
								<td><!--#tr id="w.bws.2-32" -->Disconnected<!--#endtr--></td></tr>
								<tr><td class="WPSStatus"><!--#tr id="w.bws.2-33" -->Network Name (SSID):<!--#endtr--></td>
								<td><span id="WPSStatusESSID"></span></td></tr>
								<tr><td class="WPSStatus"><!--#tr id="w.bws.2-34" -->Security:<!--#endtr--></td>
								<td><span id="WPSStatusSecurity"></span></td></tr>
							</table>
						</td>
					</tr>
				</table>
				<table id="wpsStatus" class="mainTableContent" cellspacing="0">
					<tr>
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td colspan="2" class="blankContent">
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
							<a class="bottonClass" href="javascript:checkValue()"><!--#tr id="w.bws.save" -->Save Settings<!--#endtr--></a>
							<a class="bottonClass" href="javascript:document.location.reload(true)"><!--#tr id="w.bws.cancel" -->Cancel Changes<!--#endtr--></a>
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
	<!-- InstanceEndEditable -->
</td></tr></table>
</body>
<!-- InstanceEnd --></html>
