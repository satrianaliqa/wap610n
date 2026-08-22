
var FrmElements = new Array();
var varValueHistory = new Array();

function trim(stringToTrim) {
	return stringToTrim.replace(/^\s+|\s+$/g,"");
}

function ltrim(stringToTrim) {
	return stringToTrim.replace(/^\s+/,"");
}

function rtrim(stringToTrim) {
	return stringToTrim.replace(/\s+$/,"");
}

function validateById(FRM,id,focus,alert) {
	var validationErr = "";
	var func = id.replace(/_/g,"");
	func = func.replace(/-/g,"");
	func = func.replace(/\./g,"");
	func = "if(window.vl"+func + ") { validationErr = vl"+func + "(FRM,"+alert+"); }";
	eval(func);
	
	var combo = getFrmElm(FRM,id);
	if(combo) { 
		if(combo.type=="select-one") { 
			var current_selection = combo.options[combo.selectedIndex].text;
			if (current_selection == "Select an option")	{
				validationErr = id;
			}
		}
	}
	
	var elm = returnObjById("td_"+id);
	if (elm) {	if(validationErr=="") elm.className="NOERROR"; else elm.className="ERROR";	}
	
	elm = getFrmElm(FRM,id);
	if(elm) {	if(validationErr=="") elm.className="NOERROR"; else elm.className="ERROR";	}
	if(validationErr!="" && focus) { elm.focus(); }
	return validationErr;
}

function refillComboGeneric(FORM,id,values) {
	var combo = getFrmElm(FORM,id);
	if(!combo) { return; }
	if (values==null) {return; }
	if(combo.type!="select-one") { return; }
	var current_selection = combo.options[combo.selectedIndex].value;
	combo.options.length = 0;
	for (var i = 0; i < values.length; i += 2)
		combo.options[combo.options.length] = new Option(values[i + 1], values[i]);
	
	for (var i = 0; i < combo.options.length; i++)	{
		if (combo.options[i].value == current_selection) {
			combo.selectedIndex = i;
			return;
		}
	}
	combo.options[combo.options.length] = new Option("Select an option", current_selection);
	combo.selectedIndex = combo.options.length - 1;
}


function ToUpperCase ( string ) {
	var	key = string;

	string = "";
	for ( iln = 0 ; iln < key.length ; iln++ ) {
      		ch = key.charAt(iln).toUpperCase();
		string = string + ch;
	}

	return string;
}

function length(STR) {
	return STR.length;
}

function isIP(STR,nozeros) {
	var IPvalue = STR;
	errorString = "";
	theName = "IPaddress";

	var ipPattern = /^(\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3})$/;
	var ipArray = IPvalue.match(ipPattern);

	if (IPvalue == "0.0.0.0") {
		return false;
	} else if (IPvalue == "255.255.255.255") {
		return false;
	}
		
	if (ipArray == null) {
		return false;
	} else {
		for (i = 1; i < 5; i++) {
			thisSegment = ipArray[i];
			if (thisSegment > 255) {
				return false;
			}
			
			if ((i == 0) && (thisSegment > 255)) {
				return false;
			}
		}
		if (nozeros==1 && ipArray[4] ==0) {
			return false;
			}
		
	}
	return true;
}

function isMAC(STR) {
	I=STR;
	I= ToUpperCase ( I );
	for ( iln = 0 ; iln < I.length ; iln++ ) 
	{
    ch = I.charAt(iln).toLowerCase();
		//alert("ch is:["+ ch +"]");
	  if (ch >= '0' && ch <= '9' || ch >= 'a' && ch <= 'f' || ch==':') 	{}
      else 	{
	    return false;
	  }
	}	

	d = parseInt ( I, 16 );

	if ( !(d<256 && d>=0) ) {
		return false;
	} else {
		return true;
	}
}

function isHex(STR) {
	
	var validchars="1234567890ABCDEF";
	var digit;
	
	for(var i=0 ; i<STR.length ; i++)
	{
		digit = STR.charAt(i).toUpperCase();
		if (validchars.indexOf(digit) == -1)
		{
			return false;
		}
	}
	
	return true;
}

function isEmpty(s) {
      return ((s == null) || (s.length == 0))
}

function isDigit(num) {
	if (num.length>1){return false;}
	var string="1234567890";
	if (string.indexOf(num)!=-1){return true;}
	return false;
}

function isInteger (STR) {
      var i;

      if (isEmpty(STR)) return false;

      for (i = 0; i < STR.length; i++) {
         var c = STR.charAt(i);

         if (!isDigit(c)) return false;
      }

      return true;
}

function isInRange(NUM,MIN,MAX) {	
	if (NUM>=MIN && NUM<=MAX)
		return true;
		
	return false;
}

function returnObjById( id ) {
	if (document.getElementById)
		var returnVar = document.getElementById(id);
	else if (document.all)
		var returnVar = document.all[id];
	else if (document.layers)
		var returnVar = document.layers[id];
	return returnVar;
}

function CreateFrmElementHash(form) {
	var els = form.elements;
	for(i=0; i<els.length; i++){
		FrmElements[els[i].id] = i;
		varValueHistory[els[i].id] = els[i].value;
	}
}

function getFrmElm( form, id ) {
	var els = form.elements;
	
	if (window.FrmElements[id]) {
		return els[FrmElements[id]];
		}
	
	for(i=0; i<els.length; i++){
		if (els[i].id==id)
		{
				FrmElements[id] = i;
				return els[i];
		}
	}
	return null;
}

function IsNumeric(sText) {
   sText = trim(sText);
   var ValidChars = "0123456789.";
   var IsNumber=true;
   var Char;
   
   if (sText.length==0) return false;
 
   for (i = 0; i < sText.length && IsNumber == true; i++) { 
      Char = sText.charAt(i); 
      if (ValidChars.indexOf(Char) == -1) {
			IsNumber = false;
      }
   }
   return IsNumber;
   
}

function getFrmVal(form,id) {
	var element = getFrmElm(form,id);
	if (element) {
		if (IsNumeric(element.value)){
			return Number(element.value);
		}
		else {
			return element.value;
		}
	} else return 0;
}

function isWepPassword(STR,wepLength) {
	var len = 0;
	var hexa = 0;
	
	hexa = (STR.indexOf("0x")==0 || STR.indexOf("0X")==0);
    
	if (hexa)
		STR = STR.substring(2);
		

	len = STR.length;
	wepLength = parseInt(wepLength);
	if (hexa){
		if ((wepLength==64 && len==10) || (wepLength==128 && len==26))
			return 1;
	} else	{
		if ((wepLength == 64 && len == 5) || (wepLength == 128 && len == 13))
			return 1;
	}
	
	return 0;
	
}

function IsWPSChecksumOK(STR) {
	var pin = 0;
	var accum = 0;

	// make sure the WPS PIN is all numeric and integer
	if (!isInteger(STR))
		return false;

	pin = STR;

	// make sure the pin is 8 digit long.
	if (pin<10000000 || pin>99999999)
		return false;
	
	// calculate the checksum
	accum += 3 * ((parseInt(pin / 10000000)) % 10); 
	accum += 1 * ((parseInt(pin / 1000000)) % 10); 
	accum += 3 * ((parseInt(pin / 100000)) % 10); 
	accum += 1 * ((parseInt(pin / 10000)) % 10); 
	accum += 3 * ((parseInt(pin / 1000)) % 10); 
	accum += 1 * ((parseInt(pin / 100)) % 10); 
	accum += 3 * ((parseInt(pin / 10)) % 10); 
	accum += 1 * ((parseInt(pin / 1)) % 10); 
	
	// check the checksum
	if (0 != (accum % 10))
		return false;
		
	// if all conditions apply, this checksum is OK !
	return true;
}

function getRegulatoryDomain(countryID) {
	var db = new Array("48","GP","48","GF","48","IR","48","IS","48","HU","48","VA","48","GW","48","IE","48","GG","48","IM","48","GR","48","GI","48","DE","48","GE","48","GM","48","GA","48","AF","48","GN","48","KW","48","LT","48","LI","48","LY","48","LR","48","LS","48","LB","48","IQ","48","KG","50","FR","48","KI","48","KE","48","KZ","48","JO","48","JE","48","IT","48","IL","48","LV","48","AM","48","PF","48","BA","48","BJ","48","BE","48","BY","48","BH","48","BI","48","AT","48","KH","48","AQ","48","AO","48","AD","48","AS","48","DZ","48","AL","48","AX","48","AZ","48","CZ","48","FI","48","FO","48","ET","48","EE","48","ER","48","GQ","48","BG","48","DK","48","MK","48","CY","48","HR","48","CG","48","KM","48","TD","48","CF","48","CM","48","EG","48","GS","48","LU","48","SY","48","CH","48","SE","48","SZ","48","SJ","48","TZ","48","ES","48","TG","48","ZA","48","SO","48","SI","48","SK","48","SL","48","RS","48","SN","48","SD","48","AE","48","ZM","48","CD","48","YE","48","EH","48","WF","48","VA","48","TJ","48","GB","48","SM","48","UA","48","UG","48","TM","48","TR","48","TN","48","TT","48","TO","48","UZ","48","MU","48","MZ","48","MA","48","MS","48","ME","48","MN","48","MC","48","SA","48","YT","48","NL","48","MR","48","MQ","48","MT","48","ML","48","MW","48","MG","48","ZW","48","MD","48","PG","48","MO","48","SH","48","RW","48","RU","48","RO","48","RE","48","QA","48","MM","48","PL","48","NA","48","PK","48","OM","48","NO","48","NG","48","NE","48","ST","48","PT","64","CD","80","AU","64","JP","80","ID","80","IN","80","HK","64","HM","64","GU","64","GH","64","TF","64","FJ","64","DJ","64","KR","64","CK","64","LA","64","CC","64","CX","64","CN","64","CV","64","BF","64","BN","64","IO","64","BV","64","BW","64","BT","64","BD","64","CI","64","PW","64","VU","64","TV","64","TK","64","TL","64","TH","64","TW","64","LK","64","SB","80","SG","64","SC","64","WS","64","KP","80","PH","64","VN","64","MP","64","NF","64","NU","64","NZ","64","NC","64","NP","64","NR","64","FM","64","MH","64","MV","80","MY","64","PN","16","KY","16","GD","16","GL","16","FK","16","SV","16","EC","16","DO","16","DM","16","CU","16","CR","16","AI","16","CL","16","HT","32","CA","16","BR","16","BO","16","BM","16","BZ","16","BB","16","BS","16","AW","16","AR","16","AG","16","CO","16","BL","16","VG","16","VE","16","UY","16","UM","16","US","16","TC","16","SR","16","VC","16","PM","16","MF","16","GT","16","KN","16","GY","16","PR","16","PE","16","PY","16","PA","16","NI","16","AN","16","MX","16","JM","16","HN","16","VI","16","LC");
	for (var i = 0; i <  db.length; i += 2) {
		if (db[i+1]==countryID)
			return db[i];
	}
	return 0;
}

function hideElement (elementId) {
	var element;
	if (document.all)
		element = document.all[elementId];
	else if (document.getElementById)
		element = document.getElementById(elementId);
	if (element && element.style)
		element.style.display = 'none';
}

function showElement (elementId) {
	var element;
	if (document.all)
		element = document.all[elementId];
	else if (document.getElementById)
		element = document.getElementById(elementId);
	if (element && element.style)
		element.style.display = '';
}

function toggleElement (elementId) {
	var element;
	if (document.all)
		element = document.all[elementId];
	else if (document.getElementById)
		element = document.getElementById(elementId);
	if (element && element.style) {
		if (element.style.display == '')
			hideElement(elementId);
		else
			showElement(elementId);
	}
}

function loopTD(FRM) {
	TDs=document.getElementsByTagName('td');
	for(i=0;i<TDs.length;i++)	{
		TDs[i].className = "ERROR";
	}
 }

// Show confirm alert only if there are changed parameters.
// str = the string that will be shown in the confirm alert.
// location = what page to load in case the confirmation is true.

function confirmCmd(str,location)
 {
	var paramsChangedCount = getParamChangesCount();
	if(paramsChangedCount > 0)	
	{
		var res = confirm(str);
		if(!res)
		{
			return;
		}
	}			
	window.location = location;						
 }

 
