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

var current_lang = "<% getParam(1, "Language"); %>";

function makeUpdateIPRequest(url, content) {
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
		alert("<!--#tr id=\"u-media.js.6\" -->Giving up :( Cannot create an XMLHTTP instance<!--#endtr-->");
		return false;
	}
	http_request.onreadystatechange = function () {alertUpdateIPContents()};
	http_request.open('POST', url, true);
	http_request.send(content);
}

function alertUpdateIPContents() {
	if (http_request.readyState == 4) {
		if (http_request.status == 200) {
			/*IPandMask = http_request.responseText;
			docTemp = IPandMask;
			index = docTemp.indexOf(','); //IP
			currentIP = docTemp.substr(0, index);*/
			if (http_request.responseText != "NULL")
				currentIP = http_request.responseText;
			updateIPAddresStopToWait = true;
			//alert("alertUpdateIPContents:" + currentIP);
			//alert(currentIP);
		} else {
			//alert('There was a problem with the request.');
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
		alert("<!--#tr id=\"u-media.js.6\" -->Giving up :( Cannot create an XMLHTTP instance<!--#endtr-->");
		return false;
	}
	http_request.onreadystatechange = function () {alertWaitDHCPInfoContents()};
	http_request.open('POST', url, true);
	http_request.send(content);
}

function alertWaitDHCPInfoContents() {
	if (http_request.readyState == 4) {
		if (http_request.status == 200) {
			waitDHCPInfoValue = http_request.responseText;
			docTemp = waitDHCPInfoValue;
			index = docTemp.indexOf(','); //IP
			currentIP = docTemp.substr(0, index);
			
			updateWaitDHCPInfoStopToWait = true;
			//alert("alertUpdateIPContents:" + currentIP);
			//alert(currentIP);
		} else {
			//alert('There was a problem with the request.');
		}
	}
}

function waitDHCPInfo(){
	//alert("waitDHCPInfo");
	//alert(waitDHCPInfoValue);
	if (updateWaitDHCPInfoStopToWait == true) {
		waitDHCPInfoValue="";
		currentIP="";
		//updateIPAddress();
		makeWaitDHCPInfoRequest("/goform/waitDHCPInfo", "something");
		updateWaitDHCPInfoStopToWait = false;
	}
	
	//if ((waitDHCPInfoValue != "") && (waitCount < 20))
		document.getElementById("returnField").innerHTML = "<input type=button value=\"Close\" onclick=\"returnWPS();\">";
		setTimeout("waitDHCPInfo()", 500);
	//if ((waitDHCPInfoValue != 1) && (waitCount < 50))
	//alert("waitDHCPInfoValue: " + waitDHCPInfoValue);
	/*if ((waitDHCPInfoValue == ""))
	{
		waitCount++;
		setTimeout("waitDHCPInfo()", 500);
	}
	else
	{
		
		document.getElementById("returnField").innerHTML = "<input type=button value=\"Close\" onclick=\"returnWPS();\">";
	}*/
}

function wait_page()
{	
	/*if (percent > 80) {
			rebootRedirect.location.href="http://" + currentIP +"/rebootredirect.asp";
	}*/
	
	if ((Math.round(percent) > 50) && (Math.round(percent)%5) == 0)
		rebootRedirect.location.href=redirectURL;
	
	if (percent == 0)
	{
		renewClock=100 / ((totalWaitTime*1000)/renewTime);
		//redirectURL = ".." + location.pathname;
		//document.getElementById("mainform").style.visibility = "none";
		objectDisplay("saveField", "none");
		objectDisplay("mainform", "none");
		if(current_lang != "SA"){
			document.getElementById("waitform").innerHTML = "<table bolder=\"0\"><tr><td colspan=\"2\"><font color=red><!--#tr id=\"func.js.1\" -->Processing, Please wait......<!--#endtr--></font></td></tr><tr><td width=\"95%\"><table align=\"left\" bgcolor=\"#ffffff\" cellpadding=\"0\" cellspacing=\"0\" bordercolor=\"#FFFFFF\" style=\"border-style: solid; border-width: 1px\"><tr><td width=400 align=\"left\"><table id=\"progress\" bgcolor=\"blue\" height=\"25\"><tr><td></td></tr></table></td></tr></table></td><td width=\"95%\"><font color=red><span id=\"progressValue\">&nbsp;</span></font></td></tr></table>";
		}
		else{
			document.getElementById("waitform").innerHTML = "<table bolder=\"0\"><tr><td colspan=\"2\"><font color=red><!--#tr id=\"func.js.1\" -->Processing, Please wait......<!--#endtr--></font></td></tr><tr><td width=\"95%\"><table align=\"left\" bgcolor=\"#ffffff\" cellpadding=\"0\" cellspacing=\"0\" bordercolor=\"#FFFFFF\" style=\"border-style: solid; border-width: 1px\"><tr><td width=427 align=\"right\"><table id=\"progress\" bgcolor=\"blue\" height=\"25\"><tr><td></td></tr></table></td></tr></table></td><td width=\"95%\" align=\"right\"><font color=red><span id=\"progressValue\">&nbsp;</span></font></td></tr></table>";
		}
	}
	percent+=renewClock;
	if (percent >= 100) {
		percent = 100;
		location.href = redirectURL;
		//return;
	}
	document.getElementById("progress").style.width = Math.round(percent) + "%";
	document.getElementById("progressValue").innerHTML = Math.round(percent) + "%";
	if (percent != 100)
		window.setTimeout("wait_page()", renewTime);
}

function connecting_wait_page()
{
	//alert(currentIP + ", percent=" + percent);
	//if ( ((Math.round(percent) > 10) && (Math.round(percent)%2) == 0) && ((Math.round(percent) < 30) && ((Math.round(percent)%2) == 0)) )
	if ( (Math.round(percent) >= 20) && ((Math.round(percent)%5) == 0) && (oldPercent != (Math.round(percent)%5))) {
		updateIPAddress();
		oldPercent = Math.round(percent)%5;
		//alert("connecting_wait_page: " + currentIP);
		rebootRedirect.location.href="http://" + currentIP +"/rebootredirect.asp";
	}
		
	/*if ((Math.round(percent) > 60) && ((Math.round(percent)%5) == 0))
		rebootRedirect.location.href="http://" + currentIP +"/rebootredirect.asp";*/
		
	if (percent == 0)
	{
		renewClock=100 / ((totalWaitTime*1000)/renewTime);
		//redirectURL = ".." + location.pathname;
		//document.getElementById("mainform").style.visibility = "hidden";
		objectDisplay("mainform", "none");
		if(current_lang != "SA"){
			document.getElementById("waitform").innerHTML = "<table bolder=\"0\"><tr><td colspan=\"2\"><font color=red><!--#tr id=\"func.js.1\" -->Processing, Please wait......<!--#endtr--></font></td></tr><tr><td width=\"95%\"><table align=\"left\" bgcolor=\"#ffffff\" cellpadding=\"0\" cellspacing=\"0\" bordercolor=\"#FFFFFF\" style=\"border-style: solid; border-width: 1px\"><tr><td width=400 align=\"left\"><table id=\"progress\" bgcolor=\"blue\" height=\"25\"><tr><td></td></tr></table></td></tr></table></td><td width=\"95%\"><font color=red><span id=\"progressValue\">&nbsp;</span></font></td></tr></table>";
		}
		else{
			document.getElementById("waitform").innerHTML = "<table bolder=\"0\"><tr><td colspan=\"2\"><font color=red><!--#tr id=\"func.js.1\" -->Processing, Please wait......<!--#endtr--></font></td></tr><tr><td width=\"95%\"><table align=\"left\" bgcolor=\"#ffffff\" cellpadding=\"0\" cellspacing=\"0\" bordercolor=\"#FFFFFF\" style=\"border-style: solid; border-width: 1px\"><tr><td width=427 align=\"right\"><table id=\"progress\" bgcolor=\"blue\" height=\"25\"><tr><td></td></tr></table></td></tr></table></td><td width=\"95%\" align=\"right\"><font color=red><span id=\"progressValue\">&nbsp;</span></font></td></tr></table>";
		}
	}
	percent+=renewClock;
	if (percent >= 100) {
		percent = 100;
		//location.href = redirectURL;
		location.href="http://" + currentIP +"/rebootredirect.asp";
		//return;
	}
	document.getElementById("progress").style.width = Math.round(percent) + "%";
	document.getElementById("progressValue").innerHTML = Math.round(percent) + "%";
	
	if (percent != 100)
		window.setTimeout("connecting_wait_page()", renewTime);
}


function process_page()
{	
	//if ((percent > 30))
	updateIPAddress();
	if ((Math.round(percent) > 70) && (Math.round(percent)%5) == 0)
		rebootRedirect.location.href="http://" + currentIP +"/rebootredirect.asp";
		
	if (percent == 0)
	{
		renewClock=100 / ((totalWaitTime*1000)/renewTime);
		//redirectURL = ".." + location.pathname;
		//document.getElementById("mainform").style.visibility = "hidden";
		objectDisplay("mainform", "none");
		if(current_lang != "SA"){
			document.getElementById("waitform").innerHTML = "<table bolder=\"0\"><tr><td colspan=\"2\"><font color=red><!--#tr id=\"func.js.1\" -->Processing, Please wait......<!--#endtr--></font></td></tr><tr><td width=\"95%\"><table align=\"left\" bgcolor=\"#ffffff\" cellpadding=\"0\" cellspacing=\"0\" bordercolor=\"#FFFFFF\" style=\"border-style: solid; border-width: 1px\"><tr><td width=400 align=\"left\"><table id=\"progress\" bgcolor=\"blue\" height=\"25\"><tr><td></td></tr></table></td></tr></table></td><td width=\"95%\"><font color=red><span id=\"progressValue\">&nbsp;</span></font></td></tr></table>";
		}
		else{
			document.getElementById("waitform").innerHTML = "<table bolder=\"0\"><tr><td colspan=\"2\"><font color=red><!--#tr id=\"func.js.1\" -->Processing, Please wait......<!--#endtr--></font></td></tr><tr><td width=\"95%\"><table align=\"left\" bgcolor=\"#ffffff\" cellpadding=\"0\" cellspacing=\"0\" bordercolor=\"#FFFFFF\" style=\"border-style: solid; border-width: 1px\"><tr><td width=427 align=\"right\"><table id=\"progress\" bgcolor=\"blue\" height=\"25\"><tr><td></td></tr></table></td></tr></table></td><td width=\"95%\" align=\"right\"><font color=red><span id=\"progressValue\">&nbsp;</span></font></td></tr></table>";
		}
	}
	percent+=renewClock;
	if (percent >= 100) {
		percent = 100;
		location.href = redirectURL;
		//return;
	}
	document.getElementById("progress").style.width = Math.round(percent) + "%";
	document.getElementById("progressValue").innerHTML = Math.round(percent) + "%";
	
	if (percent != 100)
		window.setTimeout("process_page()", renewTime);
}

function reboot_page()
{	
	//if ((percent > 30))
	if ((Math.round(percent) > 70) && (Math.round(percent)%5) == 0)
		rebootRedirect.location.href="http://" + currentIP +"/rebootredirect.asp";
		
	if (percent == 0)
	{
		renewClock=100 / ((totalWaitTime*1000)/renewTime);
		//redirectURL = ".." + location.pathname;
		//document.getElementById("mainform").style.visibility = "hidden";
		objectDisplay("mainform", "none");
		if(current_lang != "SA"){
			document.getElementById("waitform").innerHTML = "<table bolder=\"0\"><tr><td colspan=\"2\"><font color=red><!--#tr id=\"func.js.2\" -->Rebooting, Please wait......<!--#endtr--></font></td></tr><tr><td width=\"95%\"><table align=\"left\" bgcolor=\"#ffffff\" cellpadding=\"0\" cellspacing=\"0\" bordercolor=\"#FFFFFF\" style=\"border-style: solid; border-width: 1px\"><tr><td width=400 align=\"left\"><table id=\"progress\" bgcolor=\"blue\" height=\"25\"><tr><td></td></tr></table></td></tr></table></td><td width=\"95%\"><font color=red><span id=\"progressValue\">&nbsp;</span></font></td></tr></table>";
		}
		else{
			document.getElementById("waitform").innerHTML = "<table bolder=\"0\"><tr><td colspan=\"2\"><font color=red><!--#tr id=\"func.js.2\" -->Rebooting, Please wait......<!--#endtr--></font></td></tr><tr><td width=\"95%\"><table align=\"left\" bgcolor=\"#ffffff\" cellpadding=\"0\" cellspacing=\"0\" bordercolor=\"#FFFFFF\" style=\"border-style: solid; border-width: 1px\"><tr><td width=400 align=\"right\"><table id=\"progress\" bgcolor=\"blue\" height=\"25\"><tr><td></td></tr></table></td></tr></table></td><td width=\"95%\" align=\"right\"><font color=red><span id=\"progressValue\">&nbsp;</span></font></td></tr></table>";
		}
	}
	percent+=renewClock;
	if (percent >= 100) {
		percent = 100;
		location.href = redirectURL;
		//return;
	}
	document.getElementById("progress").style.width = Math.round(percent) + "%";
	document.getElementById("progressValue").innerHTML = Math.round(percent) + "%";
	
	if (percent != 100)
		window.setTimeout("reboot_page()", renewTime);
}
