/* 
 * u-media.js - Common Function Definition.
 *
 * Copyright (c) 2008, U-Media Communications, Inc. All Rights Reserved.
 *
 */
 
var stopToWait = false;
var http_request = false;

function d2b(value)
{
	var num1;
	var num2=null;
	var currnum;
	currnum = 128;

	num1 = eval(value);
	if(num1 >= currnum)
	{
		num2 = "1";
		num1 = num1 - currnum;
		currnum = currnum / 2;
	}
	else
	{
		num2 = "0";
		currnum = currnum / 2;
	}		

	for (p = 1; p <= 7; p++)
	{
		if(num1 >= currnum)
		{
			num2 = num2 + "1";
			num1 = num1 - currnum;
			currnum = currnum / 2;
		}
		else
		{
			num2 = num2 + "0";
			currnum = currnum / 2;
		}
	}
	return num2;
}

function checkIPwithSubnetMask(ip1, ip2, ip3, ip4, subnet1, subnet2, subnet3, subnet4)
{
	var i,j,vi=0,vj=0,zero=0,one=0;
	var ip_b=new Array(d2b(ip1), d2b(ip2), d2b(ip3), d2b(ip4));
	var subnet_b=new Array(d2b(subnet1), d2b(subnet2), d2b(subnet3), d2b(subnet4));

	for(i=0;i<4;i++) {
		for(j=0;j<8;j++) {
			if(subnet_b[i].charAt(j) == "0") {
				vi = i;
				vj = j;
				break;
			}
		}
	}

	for(i=vi;i<4;i++) {
		if (i==vi) {
			for(j=vj;j<8;j++) {
				if(ip_b[i].charAt(j) == "0")
					zero++;
				else if(ip_b[i].charAt(j) == "1")
					one++;
				else 
					return false;
			}
		} else {
			for(j=0;j<8;j++) {
				if(ip_b[i].charAt(j) == "0")
					zero++;
				else if(ip_b[i].charAt(j) == "1")
					one++;
				else 
					return false;
			}
		}
	}

	if (zero == 0 && one != 0)	{
		alert("<!--#tr id="u-media.js.11" -->The Subnet Mask address is illegal.<!--#endtr-->");
		return false;
	} else if (zero != 0 && one == 0) {
		alert("<!--#tr id="u-media.js.11" -->The Subnet Mask address is illegal.<!--#endtr-->");
		return false;
	} else if (zero == 0 && one == 0) {
		return false;
	}

	return true;
}

function checkGatewayIPwithSubnetMask(ip1, ip2, ip3, ip4, subnet1, subnet2, subnet3, subnet4, gw1, gw2, gw3, gw4)
{
	var i,j;
	var a= new Array(d2b(ip1),d2b(ip2),d2b(ip3),d2b(ip4));
	var b= new Array(d2b(subnet1),d2b(subnet2),d2b(subnet3),d2b(subnet4));
	var gw = new Array(d2b(gw1),d2b(gw2),d2b(gw3),d2b(gw4));

	for(i=0;i<4;i++) {
		for(j=0;j<8;j++) {
			if(b[i].charAt(j) == "1")
				if(a[i].charAt(j) != gw[i].charAt(j)) {
					alert("<!--#tr id="u-media.js.12" -->The IP address and gateway are not in the same subnet mask.<!--#endtr-->");
					return false;
				}
		}
	}
	return true;
}

function checkSubnetMask(ip1, ip2, ip3, ip4, subnet1, subnet2, subnet3, subnet4){
	var applyValue = true;

	//subnet mask 1
	if (applyValue && !((ip1 & subnet1) > 128))
		applyValue = false;

	//subnet mask 2
	if (applyValue && subnet1 == 255)
	{
		if (applyValue && !(subnet2 == 0 || subnet2 == 128 || subnet2 == 192 || subnet2 == 224 || subnet2 == 240 || subnet2 == 248 || subnet2 == 252 || subnet2 == 255))
				applyValue = false;
	}
	else if (applyValue && subnet1 != 255)
	{
		if (subnet2 > 0)
			applyValue = false;
	}
	
	//subnet mask 3
	if (applyValue && subnet2 == 255)
	{
		if (applyValue && !(subnet3 == 0 || subnet3 == 128 || subnet3 == 192 || subnet3 == 224 || subnet3 == 240 || subnet3 == 248 || subnet3 == 252 || subnet3 == 255))
			applyValue = false;
	}
	else if (applyValue && subnet2 != 255)
	{
		if (subnet3 > 0)
			applyValue = false;
	}
	
	//subnet mask 4
	if (applyValue && subnet3 == 255)
	{
		if (applyValue && !(subnet4 == 0 || subnet4 == 128 || subnet4 == 192 || subnet4 == 224 || subnet4 == 240 || subnet4 == 248 || subnet4 == 252))
			applyValue = false;
	}
	else if (applyValue && subnet3 != 255)
	{
		if (subnet4 > 0)
			applyValue = false;
	}	
	
	
	if (applyValue)
		return true;
	else
	{
		alert("<!--#tr id="u-media.js.11" -->The Subnet Mask address is illegal.<!--#endtr-->");
		return false;
	}
}

function maskRange(subnet, ip, gw)
{
	var subCount=0, base=0;
	
	switch (subnet*1)	
	{
		case 0:
					return true;
					break;
		
		case 128: 	//2
					subCount = 2;
					base = 128;
					break;
		case 192: 	//4
					subCount = 4;
					base = 64;
					break;
		case 224: 	//8
					subCount = 8;
					base = 32;
					break;
		case 240: 	//16
					subCount = 16;
					base = 16;
					break;
		case 248: 	//32
					subCount = 32;
					base = 8;
					break;
		case 252: 	//64
					subCount = 64;
					base = 4;
					break;
		case 255:
					if (ip == gw)
						return true;
					else
						return false;
					break;
	}
	
	
	for (loopCount=0; loopCount<subCount; loopCount++)
	{
		if (ip >= loopCount*base && gw >= loopCount*base && ip < (loopCount+1)*base && gw < (loopCount+1)*base)
		{
			//alert("maskRange: " + loopCount*base + " ~ " + (loopCount+1)*base);
			
			if (ip >= loopCount*base && ip <= (loopCount+1)*base && gw >= loopCount*base && gw <= (loopCount+1)*base)
				return true;
			else
				return false;
		}
	}
	
	return false;
}

function checkGateway(ip1, ip2, ip3, ip4, subnet1, subnet2, subnet3, subnet4, gw1, gw2, gw3, gw4) {
	var applyValue = true;
	
	if (applyValue && (ip1 ^ subnet1 != subnet1 ^ gw1))
	{
		applyValue = false;
		//alert("adderss1 fail");
	}
	if (applyValue && !maskRange(subnet2, ip2, gw2))
	{
		applyValue = false;
		//alert("adderss2 fail");
	}
	if (applyValue && !maskRange(subnet3, ip3, gw3))
	{
		applyValue = false;
		//alert("adderss3 fail");
	}
	if (applyValue && !maskRange(subnet4, ip4, gw4))
	{
		applyValue = false;
		//alert("adderss4 fail");
	}
	
	if (applyValue)
		return true;
	else
	{
		alert("<!--#tr id="u-media.js.12" -->The IP address and gateway are not in the same subnet mask.<!--#endtr-->");
		return false;
	}
}

function objectDisplay(IDName, value) {
	document.getElementById(IDName).style.display = value;
}

function removeAllOption(selectID) {
	var docTempName, loopCount;
	docTempName = document.getElementById(selectID);
	for (loopCount=docTempName.length; loopCount>=0; loopCount--)
		docTempName.remove(loopCount);
}

function addNewOption(selectID, optionID, optionName, optionValue, optionText) {
	var docTempName, newElement;
	docTempName = document.getElementById(selectID);
	newElement = document.createElement("option");
	newElement.id = optionID;
	newElement.name = optionName;
	newElement.value = optionValue;
	newElement.text = optionText;
	try {
		docTempName.add(newElement, null); //standards compliant
	}
	catch(ex) {
		docTempName.add(newElement); //IE only
	}
}


function stringToHex(tagName)
{
	var resultStr="";
	var docTemp;
	docTemp = document.getElementById("display_"+tagName).value;
	
	for (loopCount=0; loopCount<docTemp.length; loopCount++){
		if (docTemp.substr(loopCount, 1) == '\\'){
			if (docTemp.substr(loopCount+1, 1) == '"'){
				resultStr = resultStr + docTemp.substr(loopCount+1, 1);
				loopCount++;
			}
			else
				resultStr = resultStr + docTemp.substr(loopCount, 1);
		}
		else
			resultStr = resultStr + docTemp.substr(loopCount, 1);
	}
	document.getElementById("submit_"+tagName).value = resultStr;
}


function hexToString(string)
{
	var loopCount=0;
	var origSSID="";
	var tempChar;
	
	for (loopCount=0; loopCount<string.length; loopCount=loopCount+2) {
		tempChar = String.fromCharCode(parseInt(string.substr(loopCount, 2), 16));
		if (tempChar == '"')
			origSSID = origSSID + "\"";
		else
			origSSID = origSSID + tempChar;
	}
	return origSSID;
}

function translateSubmitToHex(tagName)
{
	var resultStr="";
	var docTemp;
	docTemp = document.getElementById("display_"+tagName).value;
	
	for (loopCount=0; loopCount<docTemp.length; loopCount++){
		if (docTemp.substr(loopCount, 1) == '\\'){
			if (docTemp.substr(loopCount+1, 1) == '"'){
				resultStr = resultStr + docTemp.substr(loopCount+1, 1);
				loopCount++;
			}
			else
				resultStr = resultStr + docTemp.substr(loopCount, 1);
		}
		else
			resultStr = resultStr + docTemp.substr(loopCount, 1);
	}
	document.getElementById("submit_"+tagName).value = resultStr;
	
	/*if (document.getElementById("submit_"+tagName).value.length < 1) {
		alert("The SSID length can't less than 1 character!");
		return false;
	}*/
	if ((tagName == "ESSID") && (document.getElementById("submit_"+tagName).value.length > 32)) {
		alert("<!--#tr id=\"u-media.js.1\" -->The SSID length can't big than 32 character!<!--#endtr-->");
		return false;
	}
	return true;
}

function transSSID(SSID)
{
	var loopCount=0;
	var origSSID="";
	var tempChar;
	
	for (loopCount=0; loopCount<SSID.length; loopCount=loopCount+2) {
		tempChar = String.fromCharCode(parseInt(SSID.substr(loopCount, 2), 16));
		if (tempChar == '"')
			origSSID = origSSID + "\"";
		else
			origSSID = origSSID + tempChar;
	}
	return origSSID;
}

function getHTMLContent(IDName) {
	if(window.ActiveXObject) { // IE
        return document.getElementById("ESSID_"+loopCount).innerText;
    } else if (window.XMLHttpRequest) { // Mozilla, Safari,...
        return document.getElementById("ESSID_"+loopCount).textContent;
    }
}

function checkWPAPSK(value) {
	var pskValue;
	if (value.length == 64) {
		for (loopCount=0; loopCount<64; loopCount++)
		{
			pskValue = value.substr(loopCount, 1);
			if (!(parseInt(pskValue, 16) >= 0) && !(parseInt(pskValue, 16) <= 15)) {
				alert("<!--#tr id=\"u-media.js.7\" -->Please input 64 Hex character of pre-shared key!<!--#endtr-->");
				return false;
			}
		}
		return true;
	}
	else if (value.length < 8) {
		alert("<!--#tr id=\"u-media.js.8\" -->The passphrase key must between 8 and 63 ASCII characters!<!--#endtr-->");
		return false;
	}
	
	if (!translateSubmitToHex("WPAPSK")){
		return false;
	}
	return true;
}

function selectWEPKeyLength(keyLength) {
	if (keyLength == 64) {
		for (loopCount=0; loopCount<4; loopCount++) {
			docTemp = document.getElementById("WEPKey"+loopCount);
			//docTemp.size = 11;
			docTemp.maxLength = 10;
			
			docTemp = document.getElementById("WEPKey"+loopCount).value;
			if (docTemp.length > 10)
				document.getElementById("WEPKey"+loopCount).value = docTemp.substr(0,10);
		}
	}
	else if (keyLength == 128) {
		for (loopCount=0; loopCount<4; loopCount++) {
			docTemp = document.getElementById("WEPKey"+loopCount);
			//docTemp.size = 27;
			docTemp.maxLength = 26;
		}
	}
	if (document.getElementById("wepPassphrase").value != "")
			Update_WEPKey(document.getElementById("wepPassphrase").value);
}

function selectWPAMode(mode) {
	if (mode*1 == 1) //WPA
	{
		//document.getElementById("WPAPersonalMode").options[0].selected = true;
		removeAllOption("WPAPersonalEncryption");
		addNewOption("WPAPersonalEncryption", "", "", "0", "TKIP");
	}
	else if ((mode*1 == 2) || (mode*1 == 3)) //WPA2, WPA2 Mixed
	{
		//document.getElementById("WPAPersonalMode").options[1].selected = true;
		removeAllOption("WPAPersonalEncryption");
		addNewOption("WPAPersonalEncryption", "", "", "1", "AES");
		addNewOption("WPAPersonalEncryption", "", "", "2", "TKIP or AES");
		/*if (mode == 2)
			docTemp = document.getElementById("WPAPersonalEncryption").options[0];
		else if (mode == 3)
			docTemp = document.getElementById("WPAPersonalEncryption").options[1];
		docTemp.selected = true;*/
	}
}

function Update_WEPKey(passphrase)
{	
	var wepkey = new Array(4);
	var pp = passphrase;
	var key_len = document.getElementById("WEPKeyLength").value;
	
	if (pp=='') {
		//key.value='';
		/*key0.value='';
		key1.value='';
		key2.value='';
		key3.value='';*/
	} else {
		var l=pp.length;
		if(key_len == 128){
			var seed="";
			for (i=0;i<64;i++) seed+=pp.charAt(i%l);
			wepkey[0]=wepkey[1]=wepkey[2]=wepkey[3]=MD5(seed).slice(0,26);
		}else{
			var rand=0;
			for (i=0;i<l;i++) rand^=(pp.charCodeAt(i)<<((i%4)*8));
			
			for(i=0; i<4; i++){
				var s="";
				for (j=0;j<5;j++) {
					rand*=0x343fd;
					rand+=0x269ec3;
					rand&=0xffffffff;
					s+=ToHex(rand>>16);
				}
				wepkey[i]=s;
			}
		}
		document.getElementById("WEPKey0").value = wepkey[0];
		document.getElementById("WEPKey1").value = wepkey[1];
		document.getElementById("WEPKey2").value = wepkey[2];
		document.getElementById("WEPKey3").value = wepkey[3];	
	}
}
function atoi(str, num)
{
    i=1;
    if(num != 1 ){
        while (i != num && str.length != 0){
            if(str.charAt(0) == '.'){
                i++;
            }
            str = str.substring(1);
        }
        if(i != num )
            return -1;
    }

    for(i=0; i<str.length; i++){
        if(str.charAt(i) == '.'){
            str = str.substring(0, i);
            break;
        }
    }
    if(str.length == 0)
        return -1;
    return parseInt(str, 10);
}

function isAllNum(str)
{
	for (var i=0; i<str.length; i++){
	    if((str.charAt(i) >= '0' && str.charAt(i) <= '9') || (str.charAt(i) == '.' ))
			continue;
		return 0;
	}
	return 1;
}

function checkRange(str, num, min, max)
{
	d = atoi(str, num);
	if (d > max || d < min)
		return false;
	return true;
}

function checkMaskRange(str)
{
	m = new Array(255,254,252,248,240,224,192,128,0);

	for(i=0;i<9;i++)
		if (str == m[i])
			return true;

	return false;
}

function checkIpAddr(field, ismask, msg)
{
	if (field.value == "") {
		alert("<!--#tr id=\"u-media.js.3-1\" -->Error. <!--#endtr-->" + msg + "<!--#tr id=\"u-media.js.3-2\" --> is empty!<!--#endtr-->");
		//field.value = field.defaultValue;
		//field.focus();
		return false;
	}

	if (isAllNum(field.value) == 0) {
		alert("<!--#tr id=\"u-media.js.4-1\" -->The <!--#endtr-->" + msg + "<!--#tr id=\"u-media.js.4-2\" --> should be a [0-9] number.<!--#endtr-->");
		//field.value = field.defaultValue;
		//field.focus();
		return false;
	}

	v = new Array(atoi(field.value, 1), atoi(field.value, 2), atoi(field.value, 3), atoi(field.value, 4) );
	if (ismask) {
//		if ((!checkRange(field.value, 1, 0, 256)) ||
//				(!checkRange(field.value, 2, 0, 256)) ||
//				(!checkRange(field.value, 3, 0, 256)) ||
//				(!checkRange(field.value, 4, 0, 256)))
		if (!checkMaskRange(v[0]) || !checkMaskRange(v[1]) || !checkMaskRange(v[2]) || !checkMaskRange(v[3]) ||
			(v[0] < 255 && v[1]+v[2]+v[3] > 0) ||
			(v[0] == 255 && v[1] < 255 && v[2]+v[3] > 0) ||
			(v[0] == 255 && v[1] == 255 && v[2] < 255 && v[3] > 0) ||
			field.value == "0.0.0.0" || field.value == "255.255.255.254" || field.value == "255.255.255.255")
		{
			alert("The " + msg + " is illegal value!");
			//alert("<!--#tr id=\"u-media.js.5-1\" -->The <!--#endtr-->" + msg + "<!--#tr id=\"u-media.js.5-2\" --> is illegal value!<!--#endtr-->");
			//field.value = field.defaultValue;
			//field.focus();
			return false;
		}
	}
	else {
		if ((!checkRange(field.value, 1, 0, 255)) ||
				(!checkRange(field.value, 2, 0, 255)) ||
				(!checkRange(field.value, 3, 0, 255)) ||
				(!checkRange(field.value, 4, 1, 254)))
		{
			alert("The " + msg + " is illegal value!");
			//field.value = field.defaultValue;
			//field.focus();
			return false;
		}
	}
	return true;
}

function makeRequest(url, content, fieldID, disableFieldID) {
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
	http_request.onreadystatechange = function () {alertContents(fieldID, disableFieldID)};
	http_request.open('POST', url, true);
	http_request.send(content);
}

function alertContents(fieldID, disableFieldID) {
	if (http_request.readyState == 4) {
		if (http_request.status == 200) {
			reWirteList(fieldID, disableFieldID, http_request.responseText);
		} else {
			//alert('There was a problem with the request.');
		}
	}
}

function reWirteList(fieldID, disableFieldID, str)
{
	stopToWait = true;
	
	if (disableFieldID != "" && str != "")
		objectDisplay(disableFieldID, "none")
	if (fieldID != "")
		document.getElementById(fieldID).innerHTML = str;
}

function fwUpgraceStatus(status, IPandMask)
{
	docTemp = IPandMask;
	index = docTemp.indexOf(','); //IP
	currentIP = docTemp.substr(0, index);
	
	if (status*1 == 1)
		top.location.href = "http://" + currentIP + "/admin/upgrade.asp";
}

function wpsStatus(status, mainPageName)
{
	if (status*1 == 1)
	{
		if (mainPageName != "wps_status.asp")
			top.location.href = "/station/wps_status.asp";
	}
}