<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/tr/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" lang="en-US" xml:lang="en-US"><!-- InstanceBegin template="/Templates/setup.dwt" codeOutsideHTMLIsLocked="false" -->
<head>
<meta http-equiv="content-type" content="text/html; charset=utf-8" />
<meta http-equiv="cache-control" content="no-cache">
<meta http-equiv="pragma" content="no-cache">
<meta http-equiv="X-UA-Compatible" content="IE=EmulateIE7"/>  
<!-- InstanceBeginEditable name="Page Title" -->
<title>Network Setup</title>
<!-- InstanceEndEditable -->
<link rel="stylesheet" type="text/css" href="../<% getCurrectLangSetting(); %>">
<!-- InstanceBeginEditable name="Include Files" -->
<script type="text/javascript" src="../u-media.js?20090804"></script>
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
function mainDisplay(value) {
	objectDisplay("ipConfigField", value);
}

function displayStaticField(value){
	objectDisplay("mainform", "block");
	objectDisplay("langTitle", "block");
	objectDisplay("langField", "block");
	objectDisplay("langFieldSpace", "block");
	objectDisplay("ipConfigTitle", "block");
	objectDisplay("ipConfigField", "block");
	if (value == "block") {
		objectDisplay("hostNameSeparate", "block");
		objectDisplay("hostNameField", "block");
	}
	else if (value == "none")
	{
		objectDisplay("hostNameSeparate", "block");
		objectDisplay("hostNameField", "block");
	}
	objectDisplay("staticSeparate", value)
	objectDisplay("staticIP", value);
	objectDisplay("staticSubnet", value);
	objectDisplay("staticGateway", value);
	
	objectDisplay("loopBreakSeparate", "block");
	//objectDisplay("loopBreaking", "block");
}

function selectLANMode(lanMode) {
	var networkType = "<% getParam(1, "network_type"); %>";
	if (lanMode == 1)
		displayStaticField("block");
	else
		displayStaticField("none");	
}

function checkValue() {
	var applyValue=true;
	docTemp = document.getElementById("hostName").value;
	//alert(docTemp.value);
	if ((docTemp.indexOf('\'') != -1) || (docTemp.indexOf('\"') != -1))
	{
		alert("<!--#tr id=\"u-media.js.5-1\" -->The <!--#endtr-->" + "<!--#tr id=\"nets.bns.nct.1-3\" -->Host Name:<!--#endtr-->" + docTemp + " " + "<!--#tr id=\"u-media.js.5-2\" --> is illegal value!<!--#endtr-->");
		applyValue = false;
	}
	
	if (document.getElementById("hostName").value.length > 32) {
		alert("<!--#tr id="adm.w.alert.10" -->Host Name length can't bigger than 32 characters!<!--#endtr-->");
		applyValue = false;
	}
	document.getElementById("lanIP").value = document.getElementById("lanIP_1").value + "." + document.getElementById("lanIP_2").value + "." + document.getElementById("lanIP_3").value + "." + document.getElementById("lanIP_4").value;
	if(applyValue && !checkIpAddr(document.getElementById("lanIP"), false, "<!--#tr id=\"nets.bns.check.1\" -->Network IP Address<!--#endtr-->"))
		applyValue = false;
		
	document.getElementById("lanSubnet").value = document.getElementById("lanSubnet_1").value + "." + document.getElementById("lanSubnet_2").value + "." + document.getElementById("lanSubnet_3").value + "." + document.getElementById("lanSubnet_4").value;
	if(applyValue && !checkIpAddr(document.getElementById("lanSubnet"), true, "<!--#tr id=\"nets.bns.check.2\" -->Subnet Mask<!--#endtr-->"))
		applyValue = false;
	
	document.getElementById("gateway").value = document.getElementById("gateway_1").value + "." + document.getElementById("gateway_2").value + "." + document.getElementById("gateway_3").value + "." + document.getElementById("gateway_4").value;
	if(applyValue && !checkIpAddr(document.getElementById("gateway"), false, "<!--#tr id=\"nets.bns.check.3\" -->Default Gateway<!--#endtr-->"))
		applyValue = false;
        	
	//if (applyValue && !checkSubnetMask(document.getElementById("lanIP_1").value, document.getElementById("lanIP_2").value, document.getElementById("lanIP_3").value, document.getElementById("lanIP_4").value, document.getElementById("lanSubnet_1").value, document.getElementById("lanSubnet_2").value, document.getElementById("lanSubnet_3").value, document.getElementById("lanSubnet_4").value))
	if (applyValue && !checkIPwithSubnetMask(document.getElementById("lanIP_1").value, document.getElementById("lanIP_2").value, document.getElementById("lanIP_3").value, document.getElementById("lanIP_4").value, document.getElementById("lanSubnet_1").value, document.getElementById("lanSubnet_2").value, document.getElementById("lanSubnet_3").value, document.getElementById("lanSubnet_4").value))
		applyValue = false;
		
	//if (applyValue && !checkGateway(document.getElementById("lanIP_1").value, document.getElementById("lanIP_2").value, document.getElementById("lanIP_3").value, document.getElementById("lanIP_4").value, document.getElementById("lanSubnet_1").value, document.getElementById("lanSubnet_2").value, document.getElementById("lanSubnet_3").value, document.getElementById("lanSubnet_4").value, document.getElementById("gateway_1").value, document.getElementById("gateway_2").value, document.getElementById("gateway_3").value, document.getElementById("gateway_4").value))
	if (applyValue && !checkGatewayIPwithSubnetMask(document.getElementById("lanIP_1").value, document.getElementById("lanIP_2").value, document.getElementById("lanIP_3").value, document.getElementById("lanIP_4").value, document.getElementById("lanSubnet_1").value, document.getElementById("lanSubnet_2").value, document.getElementById("lanSubnet_3").value, document.getElementById("lanSubnet_4").value, document.getElementById("gateway_1").value, document.getElementById("gateway_2").value, document.getElementById("gateway_3").value, document.getElementById("gateway_4").value))
		applyValue = false;

	if (applyValue) {
		document.getElementById("waitPad").style.display="block";
		document.networkBasic.submit();
	}
}

function page_load() {
	fwUpgraceStatus("<% getFWUpgrade(); %>", "<% getCurrectLanIP(); %>");
	// wpsStatus()
	
	var lang_element = document.getElementById("langSelection");
	var langset = "<% getParam(1, "Language"); %>";
	var lang = (langset=="")? "EN":langset;
	var i;
	for (i=0; i<lang_element.options.length; i++) {
		if (lang == lang_element.options[i].value) {
			lang_element.options.selectedIndex = i;
			break;
		}
	}
	
	var mode = "<% getParam(1, "network_type"); %>";
	var ipConfigMethod = "<% getParam(1, "ip_config_method"); %>";
	var hostName = "<% getParam(1, "HostName"); %>";
	var LanIP = "<% getParam(1, "ip_lan"); %>";
	var subnet = "<% getParam(1, "subnet_lan"); %>";
	var gateway = "<% getParam(1, "default_gw"); %>";
	
	//mainDisplay("block");

	document.getElementById("ipConfigMethod").options[ipConfigMethod].selected = true;
	if (mode == 0) //STA mode
	{		
		selectLANMode(ipConfigMethod*1);
	}
	document.getElementById("hostName").value = hostName;

	docTemp = LanIP;
	index = docTemp.indexOf(".");
	document.getElementById("lanIP_1").value = docTemp.substr(0, index);
	docTemp = docTemp.substr(index+1);
	index = docTemp.indexOf(".");
	document.getElementById("lanIP_2").value = docTemp.substr(0, index);
	docTemp = docTemp.substr(index+1);
	index = docTemp.indexOf(".");
	document.getElementById("lanIP_3").value = docTemp.substr(0, index);
	document.getElementById("lanIP_4").value = docTemp.substr(index+1);
	
	docTemp = subnet;
	index = docTemp.indexOf(".");
	document.getElementById("lanSubnet_1").value = docTemp.substr(0, index);
	docTemp = docTemp.substr(index+1);
	index = docTemp.indexOf(".");
	document.getElementById("lanSubnet_2").value = docTemp.substr(0, index);
	docTemp = docTemp.substr(index+1);
	index = docTemp.indexOf(".");
	document.getElementById("lanSubnet_3").value = docTemp.substr(0, index);
	document.getElementById("lanSubnet_4").value = docTemp.substr(index+1);
	
	docTemp = gateway;
	index = docTemp.indexOf(".");
	document.getElementById("gateway_1").value = docTemp.substr(0, index);
	docTemp = docTemp.substr(index+1);
	index = docTemp.indexOf(".");
	document.getElementById("gateway_2").value = docTemp.substr(0, index);
	docTemp = docTemp.substr(index+1);
	index = docTemp.indexOf(".");
	document.getElementById("gateway_3").value = docTemp.substr(0, index);
	document.getElementById("gateway_4").value = docTemp.substr(index+1);
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
					<!--#tr id="mainmenutitle.2" -->Setup<!--#endtr-->
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
						<td width="172" class="mainMenuSelectedUpSide"></td>
						<td width="172" class="mainMenuOptionUpSide"></td>
						<td width="172" class="mainMenuOptionUpSide"></td>
						<td width="172" class="mainMenuOptionUpSide"></td>
					</tr>
					<tr>
						<td width="172" class="mainMenuSelected"><A href="../network/sta_network.asp"><!--#tr id="mainmenu.2" -->Setup<!--#endtr--></A></td>
						<td width="172" class="mainMenuOption"><A id="ap_wbridge" href=""><!--#tr id="mainmenu.1" -->Wireless<!--#endtr--></A></td>
						<td width="172" class="mainMenuOption"><A href="../admin/management.asp"><!--#tr id="mainmenu.3" -->Administration<!--#endtr--></A></td>
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
									<td class="subMenuOption"><!--#tr id="nets.submenu.1" -->Basic Setup<!--#endtr--></td>
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
	<div id="checkLiveField" style="display:none"></div>
	<table class="mainTable" cellspacing="0">
	<form method="post" name="networkBasic" action="/goform/networkBasic">
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
				<table id="mainform" class="mainTableContent" cellspacing="0">
					<tr id="langTitle">
						<td class="subMenuMainContent" colspan="2"><!--#tr id="adm.man.0" -->Language<!--#endtr--></td>
						<td colspan="2" class="blankContent"></td>
					</tr>
					<tr id="langField">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td colspan="2" class="blankContent">
      <select name="langSelection" id="langSelection">
		<option value="EN" selected="selected">English</option>
		<option value="FR">Fran&#231;ais</option>
		<option value="DE">Deutsch</option>
		<option value="IT">Italiano</option>
		<option value="NL">Nederlands</option>
		<option value="PT">Portugu&#234;s</option>
		<option value="CA">Français Canadien</option>
		<option value="ES">Espa&#241;ol</option>
		<option value="SE">Svenska</option>
		<option value="DK">Dansk</option>
		<option value="SA">العربية</option>
		<option value="PL">polski</option>
		<option value="TR">Türk</option>
      </select>
						</td>
					</tr>
					<tr id="langFieldSpace">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="blankContent" colspan="2"><hr></td>
					</tr>
					<tr id="ipConfigTitle">
						<td class="subMenuMainContent" colspan="2"><!--#tr id="nets.bns.title" -->Network Setup<!--#endtr--></td>
						<td colspan="2" class="blankContent"></td>
					</tr>
					<tr id="ipConfigField">
						<td class="subMenuSubContent"><!--#tr id="nets.bns.nct.1" -->Bridge IP<!--#endtr--></td>
						<td class="subMenuLeftSide"></td>
						<td colspan="2" class="blankContent">
							<select id="ipConfigMethod" name="ipConfigMethod" onChange="selectLANMode(this.value);">
								<option value="0" selected><!--#tr id="nets.bns.nct.1-1" -->Automatic Configuration - DHCP<!--#endtr--></option>
								<option value="1"><!--#tr id="nets.bns.nct.1-2" -->Static IP<!--#endtr--></option>
							</select>
						</td>
					</tr>
					<tr id="hostNameSeparate">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="blankContent" colspan="2"><hr></td>
					</tr>
					<tr id="hostNameField">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><!--#tr id="nets.bns.nct.1-3" -->Host Name:<!--#endtr--></td>
						<td class="tdContent"><input type="text" id="hostName" name="hostName" value=""></td>
					</tr>
					<tr id="staticSeparate">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="blankContent" colspan="2"><hr></td>
					</tr>
					<tr id="staticIP" style="display:none">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><!--#tr id="nets.bns.nct.1-4" -->Static IP Address:<!--#endtr--></td>
						<td class="tdContent"><input type="hidden" id="lanIP" name="lanIP" onChange="" value="">
							<input type="text" id="lanIP_1" name="lanIP_1" size="6" style="max-width:24px; text-align:center" maxlength="3" value="">.<input type="text" id="lanIP_2" name="lanIP_2" size="6" style="max-width:24px; text-align:center" maxlength="3" value="">.<input type="text" id="lanIP_3" name="lanIP_3" size="6" style="max-width:24px; text-align:center" maxlength="3" value="">.<input type="text" id="lanIP_4" name="lanIP_4" size="6" style="max-width:24px; text-align:center" maxlength="3" value="">
						</td>
					</tr>
		        	<tr id="staticSubnet" style="display:none">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><!--#tr id="nets.bns.nct.1-5" -->Subnet Mask:<!--#endtr--></td>
						<td class="tdContent">
							<input type="hidden" id="lanSubnet" name="lanSubnet" onChange="" value="">
							<input type="text" id="lanSubnet_1" name="lanSubnet_1" size="6" style="max-width:24px; text-align:center" maxlength="3" value="">.<input type="text" id="lanSubnet_2" name="lanSubnet_2" size="6" style="max-width:24px; text-align:center" maxlength="3" value="">.<input type="text" id="lanSubnet_3" name="lanSubnet_3" size="6" style="max-width:24px; text-align:center" maxlength="3" value="">.<input type="text" id="lanSubnet_4" name="lanSubnet_4" size="6" style="max-width:24px; text-align:center" maxlength="3" value="">
						</td>
					</tr>
					<tr id="staticGateway" style="display:none">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel"><!--#tr id="nets.bns.nct.1-6" -->Default Gateway:<!--#endtr--></td>
						<td class="tdContent">
							<input type="hidden" id="gateway" name="gateway" onChange="" value="">
							<input type="text" id="gateway_1" name="gateway_1" size="6" style="max-width:24px; text-align:center" maxlength="3" value="">.<input type="text" id="gateway_2" name="gateway_2" size="6" style="max-width:24px; text-align:center" maxlength="3" value="">.<input type="text" id="gateway_3" name="gateway_3" size="6" style="max-width:24px; text-align:center" maxlength="3" value="">.<input type="text" id="gateway_4" name="gateway_4" size="6" style="max-width:24px; text-align:center" maxlength="3" value="">
						</td>
					</tr>
					<tr id="loopBreakSeparate" style="display:none">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="blankContent" colspan="2"><hr></td>
					</tr>
				</table>
				<table class="mainTableContent" cellspacing="0">
					<tr>
            	      <td class="subMenuSubContent"></td>
        	          <td class="subMenuLeftSide"></td>
    	              <td class="blankContent" colspan="2">&nbsp;</td>
	                </tr>
					<tr>
            	      <td class="subMenuSubContent"></td>
        	          <td class="subMenuLeftSide"></td>
    	              <td class="blankContent" colspan="2">&nbsp;</td>
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
						<td class="subMenuSubContent"><img src="../image/goahead_logo.gif" width="120" align="middle">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</td>
						<td class="subMenuLeftSide"></td>
						<td class="blankContent" colspan="2"></td>
						<td class="blankRightSide"></td>
						<!--<td class=footRight rowspan="2"></td>-->
					</tr>
					<tr>
						<td class="subMenuMainContent" colspan="2"></td>
						<td class="footContent" colspan="2">
							<div id="saveField">
							<a class="bottonClass" href="javascript:checkValue()"><!--#tr id="adm.man.save" -->Save Settings<!--#endtr--></a>
							<a class="bottonClass" href="javascript:document.location.reload(true)"><!--#tr id="adm.man.cancel" -->Cancel Changes<!--#endtr--></a>
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
	</form>
	</table>
	<!-- InstanceEndEditable -->
</td></tr></table>
</body>
<!-- InstanceEnd --></html>
