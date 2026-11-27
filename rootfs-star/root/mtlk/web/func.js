/* 
 * func.js - Function Define.
 *
 * Copyright (c) U-Media Communications, Inc. All Rights Reserved.
 *
 * $Id: v1.0 12-Dec-2007 Jacky.Yang Exp $
 */
var IPandMask = "<% getCurrectLanIP(); %>";
//var currentIP = "<% getParam(1, 'ip_lan');%>";
var currentIP;
var totalWaitTime=7; //second
var renewTime=500; //millisecond, progress reflash clock.
//var renewClock=100 / ((totalWaitTime*1000)/renewTime);
var renewClock;
var percent=0;
var redirectURL;
var urlPath, oldPercent, waitDHCPInfoValue;
var updateIPAddresStopToWait = true;
var updateWaitDHCPInfoStopToWait = true;
var waitCount=0;

docTemp = IPandMask;
index = docTemp.indexOf(','); //IP
currentIP = docTemp.substr(0, index);

var current_lang = (typeof langset !== "undefined") ? langset : "<% getParam(1, 'Language'); %>";

function makeUpdateIPRequest(url, content) {
	var req = false;
	if (window.XMLHttpRequest) { // Mozilla, Safari,...
		req = new XMLHttpRequest();
		if (req.overrideMimeType) {
			req.overrideMimeType('text/xml');
		}
	} else if (window.ActiveXObject) { // IE
		try {
			req = new ActiveXObject("Msxml2.XMLHTTP");
		} catch (e) {
			try {
				req = new ActiveXObject("Microsoft.XMLHTTP");
			} catch (e) {}
		}
	}
	if (!req) {
		alert("Giving up :( Cannot create an XMLHTTP instance");
		return false;
	}
	req.onreadystatechange = function () {
		alertUpdateIPContents(req);
	};
	req.open('POST', url, true);
	req.send(content);
}

function alertUpdateIPContents(req) {
	if (req && req.readyState == 4) {
		if (req.status == 200) {
			if (req.responseText != "NULL")
				currentIP = req.responseText;
			updateIPAddresStopToWait = true;
		} else {
			updateIPAddresStopToWait = true;
		}
	}
}

function updateIPAddress(){
	if (updateIPAddresStopToWait == true) {
		makeUpdateIPRequest("/goform/updateIPAddress", "something");
		updateIPAddresStopToWait = false;
	}
}

function makeWaitDHCPInfoRequest(url, content) {
	var req = false;
	if (window.XMLHttpRequest) { // Mozilla, Safari,...
		req = new XMLHttpRequest();
		if (req.overrideMimeType) {
			req.overrideMimeType('text/xml');
		}
	} else if (window.ActiveXObject) { // IE
		try {
			req = new ActiveXObject("Msxml2.XMLHTTP");
		} catch (e) {
			try {
				req = new ActiveXObject("Microsoft.XMLHTTP");
			} catch (e) {}
		}
	}
	if (!req) {
		alert("Giving up :( Cannot create an XMLHTTP instance");
		return false;
	}
	req.onreadystatechange = function () {
		alertWaitDHCPInfoContents(req);
	};
	req.open('POST', url, true);
	req.send(content);
}

function alertWaitDHCPInfoContents(req) {
	if (req && req.readyState == 4) {
		if (req.status == 200) {
			waitDHCPInfoValue = req.responseText;
			docTemp = waitDHCPInfoValue;
			index = docTemp.indexOf(','); //IP
			currentIP = docTemp.substr(0, index);
			
			updateWaitDHCPInfoStopToWait = true;
		} else {
			updateWaitDHCPInfoStopToWait = true;
		}
	}
}

function waitDHCPInfo(){
	if (updateWaitDHCPInfoStopToWait == true) {
		waitDHCPInfoValue = "";
		currentIP = "";
		makeWaitDHCPInfoRequest("/goform/waitDHCPInfo", "something");
		updateWaitDHCPInfoStopToWait = false;
	}

	if (waitDHCPInfoValue != "" && currentIP != "") {
		document.getElementById("returnField").innerHTML = "<input type=button value=\"Close\" onclick=\"returnWPS();\">";
		waitCount = 0;
	} else if (waitCount < 60) {
		waitCount++;
		setTimeout("waitDHCPInfo()", 500);
	} else {
		document.getElementById("returnField").innerHTML = "<input type=button value=\"Close\" onclick=\"returnWPS();\">";
		waitCount = 0;
	}
}

function makeRequest(url, content, fieldID, disableFieldID) {
	var req = false;
	if (window.XMLHttpRequest) { // Mozilla, Safari,...
		req = new XMLHttpRequest();
		if (req.overrideMimeType) {
			req.overrideMimeType('text/xml');
		}
	} else if (window.ActiveXObject) { // IE
		try {
			req = new ActiveXObject("Msxml2.XMLHTTP");
		} catch (e) {
			try {
				req = new ActiveXObject("Microsoft.XMLHTTP");
			} catch (e) {}
		}
	}
	if (!req) {
		alert("Giving up :( Cannot create an XMLHTTP instance");
		return false;
	}
	req.onreadystatechange = function () {
		http_request = req;
		if (typeof alertContents === "function") {
			alertContents(fieldID, disableFieldID, req);
		}
	};
	req.open('POST', url, true);
	req.send(content);
}

function setRebootRedirectSrc(srcUrl) {
	var frame = document.getElementById("rebootRedirect");
	if (frame) {
		frame.src = srcUrl;
	}
}

function wait_page()
{	
	var wt = parseInt(totalWaitTime, 10);
	if (isNaN(wt) || wt < 10) wt = 10;
	totalWaitTime = wt;

	if ((Math.round(percent) > 50) && (Math.round(percent)%5) == 0 && redirectURL)
		setRebootRedirectSrc(redirectURL);
	
	if (percent == 0)
	{
		renewClock=100 / ((totalWaitTime*1000)/renewTime);
		objectDisplay("saveField", "none");
		objectDisplay("mainform", "none");
		if(current_lang != "SA"){
			document.getElementById("waitform").innerHTML = "<table bolder=\"0\"><tr><td colspan=\"2\"><font color=red>Processing, Please wait......</font></td></tr><tr><td width=\"95%\"><table align=\"left\" bgcolor=\"#ffffff\" cellpadding=\"0\" cellspacing=\"0\" bordercolor=\"#FFFFFF\" style=\"border-style: solid; border-width: 1px\"><tr><td width=400 align=\"left\"><table id=\"progress\" bgcolor=\"blue\" height=\"25\"><tr><td></td></tr></table></td></tr></table></td><td width=\"95%\"><font color=red><span id=\"progressValue\">&nbsp;</span></font></td></tr></table>";
		}
		else{
			document.getElementById("waitform").innerHTML = "<table bolder=\"0\"><tr><td colspan=\"2\"><font color=red>Processing, Please wait......</font></td></tr><tr><td width=\"95%\"><table align=\"left\" bgcolor=\"#ffffff\" cellpadding=\"0\" cellspacing=\"0\" bordercolor=\"#FFFFFF\" style=\"border-style: solid; border-width: 1px\"><tr><td width=427 align=\"right\"><table id=\"progress\" bgcolor=\"blue\" height=\"25\"><tr><td></td></tr></table></td></tr></table></td><td width=\"95%\" align=\"right\"><font color=red><span id=\"progressValue\">&nbsp;</span></font></td></tr></table>";
		}
	}
	percent+=renewClock;
	if (percent >= 100) {
		percent = 100;
		location.href = redirectURL || "/";
	}
	document.getElementById("progress").style.width = Math.round(percent) + "%";
	document.getElementById("progressValue").innerHTML = Math.round(percent) + "%";
	if (percent != 100)
		window.setTimeout("wait_page()", renewTime);
}

function connecting_wait_page()
{
	var wt = parseInt(totalWaitTime, 10);
	if (isNaN(wt) || wt < 10) wt = 10;
	totalWaitTime = wt;

	if ( (Math.round(percent) >= 20) && ((Math.round(percent)%5) == 0) && (oldPercent != Math.round(percent))) {
		updateIPAddress();
		oldPercent = Math.round(percent);
		setRebootRedirectSrc("http://" + currentIP +"/rebootredirect.asp");
	}
		
	if (percent == 0)
	{
		renewClock=100 / ((totalWaitTime*1000)/renewTime);
		objectDisplay("mainform", "none");
		if(current_lang != "SA"){
			document.getElementById("waitform").innerHTML = "<table bolder=\"0\"><tr><td colspan=\"2\"><font color=red>Processing, Please wait......</font></td></tr><tr><td width=\"95%\"><table align=\"left\" bgcolor=\"#ffffff\" cellpadding=\"0\" cellspacing=\"0\" bordercolor=\"#FFFFFF\" style=\"border-style: solid; border-width: 1px\"><tr><td width=400 align=\"left\"><table id=\"progress\" bgcolor=\"blue\" height=\"25\"><tr><td></td></tr></table></td></tr></table></td><td width=\"95%\"><font color=red><span id=\"progressValue\">&nbsp;</span></font></td></tr></table>";
		}
		else{
			document.getElementById("waitform").innerHTML = "<table bolder=\"0\"><tr><td colspan=\"2\"><font color=red>Processing, Please wait......</font></td></tr><tr><td width=\"95%\"><table align=\"left\" bgcolor=\"#ffffff\" cellpadding=\"0\" cellspacing=\"0\" bordercolor=\"#FFFFFF\" style=\"border-style: solid; border-width: 1px\"><tr><td width=427 align=\"right\"><table id=\"progress\" bgcolor=\"blue\" height=\"25\"><tr><td></td></tr></table></td></tr></table></td><td width=\"95%\" align=\"right\"><font color=red><span id=\"progressValue\">&nbsp;</span></font></td></tr></table>";
		}
	}
	percent+=renewClock;
	if (percent >= 100) {
		percent = 100;
		location.href="http://" + currentIP +"/rebootredirect.asp";
	}
	document.getElementById("progress").style.width = Math.round(percent) + "%";
	document.getElementById("progressValue").innerHTML = Math.round(percent) + "%";
	
	if (percent != 100)
		window.setTimeout("connecting_wait_page()", renewTime);
}


function process_page()
{	
	var wt = parseInt(totalWaitTime, 10);
	if (isNaN(wt) || wt < 10) wt = 10;
	totalWaitTime = wt;

	updateIPAddress();
	if ((Math.round(percent) > 70) && (Math.round(percent)%5) == 0)
		setRebootRedirectSrc("http://" + currentIP +"/rebootredirect.asp");
		
	if (percent == 0)
	{
		renewClock=100 / ((totalWaitTime*1000)/renewTime);
		objectDisplay("mainform", "none");
		if(current_lang != "SA"){
			document.getElementById("waitform").innerHTML = "<table bolder=\"0\"><tr><td colspan=\"2\"><font color=red>Processing, Please wait......</font></td></tr><tr><td width=\"95%\"><table align=\"left\" bgcolor=\"#ffffff\" cellpadding=\"0\" cellspacing=\"0\" bordercolor=\"#FFFFFF\" style=\"border-style: solid; border-width: 1px\"><tr><td width=400 align=\"left\"><table id=\"progress\" bgcolor=\"blue\" height=\"25\"><tr><td></td></tr></table></td></tr></table></td><td width=\"95%\"><font color=red><span id=\"progressValue\">&nbsp;</span></font></td></tr></table>";
		}
		else{
			document.getElementById("waitform").innerHTML = "<table bolder=\"0\"><tr><td colspan=\"2\"><font color=red>Processing, Please wait......</font></td></tr><tr><td width=\"95%\"><table align=\"left\" bgcolor=\"#ffffff\" cellpadding=\"0\" cellspacing=\"0\" bordercolor=\"#FFFFFF\" style=\"border-style: solid; border-width: 1px\"><tr><td width=427 align=\"right\"><table id=\"progress\" bgcolor=\"blue\" height=\"25\"><tr><td></td></tr></table></td></tr></table></td><td width=\"95%\" align=\"right\"><font color=red><span id=\"progressValue\">&nbsp;</span></font></td></tr></table>";
		}
	}
	percent+=renewClock;
	if (percent >= 100) {
		percent = 100;
		location.href = (typeof urlPath === "string" && urlPath.length > 0 && currentIP)
			? ("http://" + currentIP + urlPath)
			: redirectURL;
	}
	document.getElementById("progress").style.width = Math.round(percent) + "%";
	document.getElementById("progressValue").innerHTML = Math.round(percent) + "%";
	
	if (percent != 100)
		window.setTimeout("process_page()", renewTime);
}

function reboot_page()
{	
	var wt = parseInt(totalWaitTime, 10);
	if (isNaN(wt) || wt < 10) wt = 10;
	totalWaitTime = wt;

	if ((Math.round(percent) > 70) && (Math.round(percent)%5) == 0)
		setRebootRedirectSrc(redirectURL || ("http://" + currentIP + "/rebootredirect.asp"));
		
	if (percent == 0)
	{
		renewClock=100 / ((totalWaitTime*1000)/renewTime);
		objectDisplay("mainform", "none");
		if(current_lang != "SA"){
			document.getElementById("waitform").innerHTML = "<table bolder=\"0\"><tr><td colspan=\"2\"><font color=red>Rebooting, Please wait......</font></td></tr><tr><td width=\"95%\"><table align=\"left\" bgcolor=\"#ffffff\" cellpadding=\"0\" cellspacing=\"0\" bordercolor=\"#FFFFFF\" style=\"border-style: solid; border-width: 1px\"><tr><td width=400 align=\"left\"><table id=\"progress\" bgcolor=\"blue\" height=\"25\"><tr><td></td></tr></table></td></tr></table></td><td width=\"95%\"><font color=red><span id=\"progressValue\">&nbsp;</span></font></td></tr></table>";
		}
		else{
			document.getElementById("waitform").innerHTML = "<table bolder=\"0\"><tr><td colspan=\"2\"><font color=red>Rebooting, Please wait......</font></td></tr><tr><td width=\"95%\"><table align=\"left\" bgcolor=\"#ffffff\" cellpadding=\"0\" cellspacing=\"0\" bordercolor=\"#FFFFFF\" style=\"border-style: solid; border-width: 1px\"><tr><td width=400 align=\"right\"><table id=\"progress\" bgcolor=\"blue\" height=\"25\"><tr><td></td></tr></table></td></tr></table></td><td width=\"95%\" align=\"right\"><font color=red><span id=\"progressValue\">&nbsp;</span></font></td></tr></table>";
		}
	}
	percent+=renewClock;
	if (percent >= 100) {
		percent = 100;
		location.href = redirectURL;
	}
	document.getElementById("progress").style.width = Math.round(percent) + "%";
	document.getElementById("progressValue").innerHTML = Math.round(percent) + "%";
	
	if (percent != 100)
		window.setTimeout("reboot_page()", renewTime);
}
