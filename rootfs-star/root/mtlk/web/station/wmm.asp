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
		document.getElementById("modelNameDisplay").innerHTML = "WAP610N"; document.getElementById("descriptionDisplay").innerHTML = "Dual-Band Wireless-N Access Point";
		document.getElementById("ap_wbridge").href = "../wireless/security.asp";
	}

	page_load();
}
</script>
<!-- InstanceBeginEditable name="Scripts" -->
<script language="JavaScript" type="text/javascript">

var AC_BE_UseAggregation = "<% getParam(1, "AC_BE_UseAggregation"); %>";
var AC_BE_AcceptAggregation = "<% getParam(1, "AC_BE_AcceptAggregation"); %>";
var AC_BE_MaxNumOfPackets = "<% getParam(1, "AC_BE_MaxNumOfPackets"); %>";
var AC_BE_MaxNumOfBytes = "<% getParam(1, "AC_BE_MaxNumOfBytes"); %>";
var AC_BE_TimeoutInterval = "<% getParam(1, "AC_BE_TimeoutInterval"); %>";
var AC_BE_MinSizeOfPacketInAggr = "<% getParam(1, "AC_BE_MinSizeOfPacketInAggr"); %>";
var AC_BE_ADDBATimeout = "<% getParam(1, "AC_BE_ADDBATimeout"); %>";
var AC_BE_AggregationWindowSize = "<% getParam(1, "AC_BE_AggregationWindowSize"); %>";
var AC_BE_CWmin = "<% getParam(1, "AC_BE_CWmin"); %>";
var AC_BE_CWmax = "<% getParam(1, "AC_BE_CWmax"); %>";
var AC_BE_AIFSN = "<% getParam(1, "AC_BE_AIFSN"); %>";
var AC_BE_TXOP = "<% getParam(1, "AC_BE_TXOP"); %>";
var AC_BE_CWmin_AP = "<% getParam(1, "AC_BE_CWmin_AP"); %>";
var AC_BE_CWmax_AP = "<% getParam(1, "AC_BE_CWmax_AP"); %>";
var AC_BE_AIFSN_AP = "<% getParam(1, "AC_BE_AIFSN_AP"); %>";
var AC_BE_TXOP_AP = "<% getParam(1, "AC_BE_TXOP_AP"); %>";
	
var AC_VI_UseAggregation = "<% getParam(1, "AC_VI_UseAggregation"); %>";
var AC_VI_AcceptAggregation = "<% getParam(1, "AC_VI_AcceptAggregation"); %>";
var AC_VI_MaxNumOfPackets = "<% getParam(1, "AC_VI_MaxNumOfPackets"); %>";
var AC_VI_MaxNumOfBytes = "<% getParam(1, "AC_VI_MaxNumOfBytes"); %>";
var AC_VI_TimeoutInterval = "<% getParam(1, "AC_VI_TimeoutInterval"); %>";
var AC_VI_MinSizeOfPacketInAggr = "<% getParam(1, "AC_VI_MinSizeOfPacketInAggr"); %>";
var AC_VI_ADDBATimeout = "<% getParam(1, "AC_VI_ADDBATimeout"); %>";
var AC_VI_AggregationWindowSize = "<% getParam(1, "AC_VI_AggregationWindowSize"); %>";
var AC_VI_CWmin = "<% getParam(1, "AC_VI_CWmin"); %>";
var AC_VI_CWmax = "<% getParam(1, "AC_VI_CWmax"); %>";
var AC_VI_AIFSN = "<% getParam(1, "AC_VI_AIFSN"); %>";
var AC_VI_TXOP = "<% getParam(1, "AC_VI_TXOP"); %>";
var AC_VI_CWmin_AP = "<% getParam(1, "AC_VI_CWmin_AP"); %>";
var AC_VI_CWmax_AP = "<% getParam(1, "AC_VI_CWmax_AP"); %>";
var AC_VI_AIFSN_AP = "<% getParam(1, "AC_VI_AIFSN_AP"); %>";
var AC_VI_TXOP_AP = "<% getParam(1, "AC_VI_TXOP_AP"); %>";
	
var AC_VO_UseAggregation = "<% getParam(1, "AC_VO_UseAggregation"); %>";
var AC_VO_AcceptAggregation = "<% getParam(1, "AC_VO_AcceptAggregation"); %>";
var AC_VO_MaxNumOfPackets = "<% getParam(1, "AC_VO_MaxNumOfPackets"); %>";
var AC_VO_MaxNumOfBytes = "<% getParam(1, "AC_VO_MaxNumOfBytes"); %>";
var AC_VO_TimeoutInterval = "<% getParam(1, "AC_VO_TimeoutInterval"); %>";
var AC_VO_MinSizeOfPacketInAggr = "<% getParam(1, "AC_VO_MinSizeOfPacketInAggr"); %>";
var AC_VO_ADDBATimeout = "<% getParam(1, "AC_VO_ADDBATimeout"); %>";
var AC_VO_AggregationWindowSize = "<% getParam(1, "AC_VO_AggregationWindowSize"); %>";
var AC_VO_CWmin = "<% getParam(1, "AC_VO_CWmin"); %>";
var AC_VO_CWmax = "<% getParam(1, "AC_VO_CWmax"); %>";
var AC_VO_AIFSN = "<% getParam(1, "AC_VO_AIFSN"); %>";
var AC_VO_TXOP = "<% getParam(1, "AC_VO_TXOP"); %>";
var AC_VO_CWmin_AP = "<% getParam(1, "AC_VO_CWmin_AP"); %>";
var AC_VO_CWmax_AP = "<% getParam(1, "AC_VO_CWmax_AP"); %>";
var AC_VO_AIFSN_AP = "<% getParam(1, "AC_VO_AIFSN_AP"); %>";
var AC_VO_TXOP_AP = "<% getParam(1, "AC_VO_TXOP_AP"); %>";

var AC_BK_UseAggregation = "<% getParam(1, "AC_BK_UseAggregation"); %>";
var AC_BK_AcceptAggregation = "<% getParam(1, "AC_BK_AcceptAggregation"); %>";
var AC_BK_MaxNumOfPackets = "<% getParam(1, "AC_BK_MaxNumOfPackets"); %>";
var AC_BK_MaxNumOfBytes = "<% getParam(1, "AC_BK_MaxNumOfBytes"); %>";
var AC_BK_TimeoutInterval = "<% getParam(1, "AC_BK_TimeoutInterval"); %>";
var AC_BK_MinSizeOfPacketInAggr = "<% getParam(1, "AC_BK_MinSizeOfPacketInAggr"); %>";
var AC_BK_ADDBATimeout = "<% getParam(1, "AC_BK_ADDBATimeout"); %>";
var AC_BK_AggregationWindowSize = "<% getParam(1, "AC_BK_AggregationWindowSize"); %>";
var AC_BK_CWmin = "<% getParam(1, "AC_BK_CWmin"); %>";
var AC_BK_CWmax = "<% getParam(1, "AC_BK_CWmax"); %>";
var AC_BK_AIFSN = "<% getParam(1, "AC_BK_AIFSN"); %>";
var AC_BK_TXOP = "<% getParam(1, "AC_BK_TXOP"); %>";
var AC_BK_CWmin_AP = "<% getParam(1, "AC_BK_CWmin_AP"); %>";
var AC_BK_CWmax_AP = "<% getParam(1, "AC_BK_CWmax_AP"); %>";
var AC_BK_AIFSN_AP = "<% getParam(1, "AC_BK_AIFSN_AP"); %>";
var AC_BK_TXOP_AP = "<% getParam(1, "AC_BK_TXOP_AP"); %>";

function changeFieldContent(value) {
	document.getElementById("accessModeStatus").innerHTML = value;
	
	/*document.getElementById("tdUseAggregation").innerHTML = "Use Aggregation " + value + ":";
	document.getElementById("tdAcceptAggregation").innerHTML = "Accept Aggregation " + value + ":";
	document.getElementById("tdMaxNumOfPacket").innerHTML = "Max. Number Of Packets in aggregation " + value + ":";
	document.getElementById("tdMaxSize").innerHTML = "Max. Aggregation size " + value + ":";
	document.getElementById("tdTimeoutInterval").innerHTML = "Timeout Interval " + value + ":";
	document.getElementById("tdMinSize").innerHTML = "Min. size of packet in Aggregation " + value + ":";
	document.getElementById("tdADDBATimeout").innerHTML = "ADDBA timeout " + value + ":";
	document.getElementById("tdWindowSize").innerHTML = "Aggregation Window Size " + value + ":";
	document.getElementById("tdSTACWmin").innerHTML = "CW min for STA " + value + ":";
	document.getElementById("tdSTACWMax").innerHTML = "CW max for STA " + value + ":";
	document.getElementById("tdSTAAIFSN").innerHTML = "AIFSN for STA " + value + ":";
	document.getElementById("tdSTATXOP").innerHTML = "TXOP for STA " + value + ":";
	document.getElementById("tdAPCWMin").innerHTML = "CW min for AP " + value + ":";
	document.getElementById("tdAPCWMax").innerHTML = "CW max for AP " + value + ":";
	document.getElementById("tdAPAIFSN").innerHTML = "AIFSN for AP " + value + ":";
	document.getElementById("tdAPTXOP").innerHTML = "TXOP for AP " + value + ":";*/
}

/*function removeAllOption(selectID) {
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
	docTempName.add(newElement);
}*/

function changeVOValue()
{
	document.getElementById("AC_BK_UseAggregation").options[AC_VO_UseAggregation].selected = true;
	document.getElementById("AC_BK_AcceptAggregation").options[AC_VO_AcceptAggregation].selected = true;
	document.getElementById("AC_BK_MaxNumOfPackets").value = AC_VO_MaxNumOfPackets;
	document.getElementById("AC_BK_MaxNumOfBytes").value = AC_VO_MaxNumOfBytes;
	document.getElementById("AC_BK_TimeoutInterval").value = AC_VO_TimeoutInterval;
	document.getElementById("AC_BK_MinSizeOfPacketInAggr").value = AC_VO_MinSizeOfPacketInAggr;
	document.getElementById("AC_BK_ADDBATimeout").value = AC_VO_ADDBATimeout;
	document.getElementById("AC_BK_AggregationWindowSize").value = AC_VO_AggregationWindowSize;
	//document.getElementById("AC_VO_CWmin").options[AC_VO_CWmin].selected = true;
	//document.getElementById("AC_VO_CWmax").options[AC_VO_CWmax].selected = true;
	docTemp = document.getElementById("selectCWMinSTA");
	for (loopCount=0; loopCount<docTemp.length; loopCount++)
		if (docTemp.options[loopCount].value == AC_VO_CWmin)
			docTemp.options[loopCount].selected = true;
	
	docTemp = document.getElementById("selectCWMaxSTA");
	for (loopCount=0; loopCount<docTemp.length; loopCount++)
		if (docTemp.options[loopCount].value == AC_VO_CWmax)
			docTemp.options[loopCount].selected = true;

	document.getElementById("AC_BK_AIFSN").value = AC_VO_AIFSN;
	document.getElementById("AC_BK_TXOP").value = AC_VO_TXOP;
	//document.getElementById("AC_VO_CWmin_AP").options[AC_VO_CWmin_AP].selected = true;
	//document.getElementById("AC_VO_CWmax_AP").options[AC_VO_CWmax_AP].selected = true;
	docTemp = document.getElementById("selectCWMinAP");
	for (loopCount=0; loopCount<docTemp.length; loopCount++)
		if (docTemp.options[loopCount].value == AC_VO_CWmin_AP)
			docTemp.options[loopCount].selected = true;
			
	docTemp = document.getElementById("selectCWMaxAP");
	for (loopCount=0; loopCount<docTemp.length; loopCount++)
		if (docTemp.options[loopCount].value == AC_VO_CWmax_AP)
			docTemp.options[loopCount].selected = true;

	document.getElementById("AC_BK_AIFSN_AP").value = AC_VO_AIFSN_AP;
	document.getElementById("AC_BK_TXOP_AP").value = AC_VO_TXOP_AP;
}

function changeVIValue()
{
	document.getElementById("AC_BK_UseAggregation").options[AC_VI_UseAggregation].selected = true;
	document.getElementById("AC_BK_AcceptAggregation").options[AC_VI_AcceptAggregation].selected = true;
	document.getElementById("AC_BK_MaxNumOfPackets").value = AC_VI_MaxNumOfPackets;
	document.getElementById("AC_BK_MaxNumOfBytes").value = AC_VI_MaxNumOfBytes;
	document.getElementById("AC_BK_TimeoutInterval").value = AC_VI_TimeoutInterval;
	document.getElementById("AC_BK_MinSizeOfPacketInAggr").value = AC_VI_MinSizeOfPacketInAggr;
	document.getElementById("AC_BK_ADDBATimeout").value = AC_VI_ADDBATimeout;
	document.getElementById("AC_BK_AggregationWindowSize").value = AC_VI_AggregationWindowSize;
	//document.getElementById("AC_VI_CWmin").options[AC_VI_CWmin].selected = true;
	//document.getElementById("AC_VI_CWmax").options[AC_VI_CWmax].selected = true;
	docTemp = document.getElementById("selectCWMinSTA");
	for (loopCount=0; loopCount<docTemp.length; loopCount++)
		if (docTemp.options[loopCount].value == AC_VI_CWmin)
			docTemp.options[loopCount].selected = true;
	
	docTemp = document.getElementById("selectCWMaxSTA");
	for (loopCount=0; loopCount<docTemp.length; loopCount++)
		if (docTemp.options[loopCount].value == AC_VI_CWmax)
			docTemp.options[loopCount].selected = true;

	document.getElementById("AC_BK_AIFSN").value = AC_VI_AIFSN;
	document.getElementById("AC_BK_TXOP").value = AC_VI_TXOP;
	//document.getElementById("AC_VI_CWmin_AP").options[AC_VI_CWmin_AP].selected = true;
	//document.getElementById("AC_VI_CWmax_AP").options[AC_VI_CWmax_AP].selected = true;
	docTemp = document.getElementById("selectCWMinAP");
	for (loopCount=0; loopCount<docTemp.length; loopCount++)
		if (docTemp.options[loopCount].value == AC_VI_CWmin_AP)
			docTemp.options[loopCount].selected = true;
			
	docTemp = document.getElementById("selectCWMaxAP");
	for (loopCount=0; loopCount<docTemp.length; loopCount++)
		if (docTemp.options[loopCount].value == AC_VI_CWmax_AP)
			docTemp.options[loopCount].selected = true;
	
	document.getElementById("AC_BK_AIFSN_AP").value = AC_VI_AIFSN_AP;
	document.getElementById("AC_BK_TXOP_AP").value = AC_VI_TXOP_AP;
}

function changeBEValue()
{
	document.getElementById("AC_BK_UseAggregation").options[AC_BE_UseAggregation].selected = true;
	document.getElementById("AC_BK_AcceptAggregation").options[AC_BE_AcceptAggregation].selected = true;
	document.getElementById("AC_BK_MaxNumOfPackets").value = AC_BE_MaxNumOfPackets;
	document.getElementById("AC_BK_MaxNumOfBytes").value = AC_BE_MaxNumOfBytes;
	document.getElementById("AC_BK_TimeoutInterval").value = AC_BE_TimeoutInterval;
	document.getElementById("AC_BK_MinSizeOfPacketInAggr").value = AC_BE_MinSizeOfPacketInAggr;
	document.getElementById("AC_BK_ADDBATimeout").value = AC_BE_ADDBATimeout;
	document.getElementById("AC_BK_AggregationWindowSize").value = AC_BE_AggregationWindowSize;
	//document.getElementById("AC_BE_CWmin").options[AC_BE_CWmin].selected = true;
	//document.getElementById("AC_BE_CWmax").options[AC_BE_CWmax].selected = true;
	docTemp = document.getElementById("selectCWMinSTA");
	for (loopCount=0; loopCount<docTemp.length; loopCount++)
		if (docTemp.options[loopCount].value == AC_BE_CWmin)
			docTemp.options[loopCount].selected = true;
	
	docTemp = document.getElementById("selectCWMaxSTA");
	for (loopCount=0; loopCount<docTemp.length; loopCount++)
		if (docTemp.options[loopCount].value == AC_BE_CWmax)
			docTemp.options[loopCount].selected = true;

	document.getElementById("AC_BK_AIFSN").value = AC_BE_AIFSN;
	document.getElementById("AC_BK_TXOP").value = AC_BE_TXOP;
	//document.getElementById("AC_BE_CWmin_AP").options[AC_BE_CWmin_AP].selected = true;
	//document.getElementById("AC_BE_CWmax_AP").options[AC_BE_CWmax_AP].selected = true;
	docTemp = document.getElementById("selectCWMinAP");
	for (loopCount=0; loopCount<docTemp.length; loopCount++)
		if (docTemp.options[loopCount].value == AC_BE_CWmin_AP)
			docTemp.options[loopCount].selected = true;
			
	docTemp = document.getElementById("selectCWMaxAP");
	for (loopCount=0; loopCount<docTemp.length; loopCount++)
		if (docTemp.options[loopCount].value == AC_BE_CWmax_AP)
			docTemp.options[loopCount].selected = true;
	
	document.getElementById("AC_BK_AIFSN_AP").value = AC_BE_AIFSN_AP;
	document.getElementById("AC_BK_TXOP_AP").value = AC_BE_TXOP_AP;
}

function changeBKValue()
{
	document.getElementById("AC_BK_UseAggregation").options[AC_BK_UseAggregation].selected = true;
	document.getElementById("AC_BK_AcceptAggregation").options[AC_BK_AcceptAggregation].selected = true;
	document.getElementById("AC_BK_MaxNumOfPackets").value = AC_BK_MaxNumOfPackets;
	document.getElementById("AC_BK_MaxNumOfBytes").value = AC_BK_MaxNumOfBytes;
	document.getElementById("AC_BK_TimeoutInterval").value = AC_BK_TimeoutInterval;
	document.getElementById("AC_BK_MinSizeOfPacketInAggr").value = AC_BK_MinSizeOfPacketInAggr;
	document.getElementById("AC_BK_ADDBATimeout").value = AC_BK_ADDBATimeout;
	document.getElementById("AC_BK_AggregationWindowSize").value = AC_BK_AggregationWindowSize;
	//document.getElementById("AC_BK_CWmin").options[AC_BK_CWmin].selected = true;
	//document.getElementById("AC_BK_CWmax").options[AC_BK_CWmax].selected = true;
	docTemp = document.getElementById("selectCWMinSTA");
	for (loopCount=0; loopCount<docTemp.length; loopCount++)
		if (docTemp.options[loopCount].value == AC_BK_CWmin)
			docTemp.options[loopCount].selected = true;
	
	docTemp = document.getElementById("selectCWMaxSTA");
	for (loopCount=0; loopCount<docTemp.length; loopCount++)
		if (docTemp.options[loopCount].value == AC_BK_CWmax)
			docTemp.options[loopCount].selected = true;

	document.getElementById("AC_BK_AIFSN").value = AC_BK_AIFSN;
	document.getElementById("AC_BK_TXOP").value = AC_BK_TXOP;
	//document.getElementById("AC_BK_CWmin_AP").options[AC_BK_CWmin_AP].selected = true;
	//document.getElementById("AC_BK_CWmax_AP").options[AC_BK_CWmax_AP].selected = true;
	docTemp = document.getElementById("selectCWMinAP");
	for (loopCount=0; loopCount<docTemp.length; loopCount++)
		if (docTemp.options[loopCount].value == AC_BK_CWmin_AP)
			docTemp.options[loopCount].selected = true;
			
	docTemp = document.getElementById("selectCWMaxAP");
	for (loopCount=0; loopCount<docTemp.length; loopCount++)
		if (docTemp.options[loopCount].value == AC_BK_CWmax_AP)
			docTemp.options[loopCount].selected = true;
			
	document.getElementById("AC_BK_AIFSN_AP").value = AC_BK_AIFSN_AP;
	document.getElementById("AC_BK_TXOP_AP").value = AC_BK_TXOP_AP;
}

function selectAccessMode(mode) {
	document.getElementById("accessMode").selected = true;
		
	document.getElementById("CurrentWMM").value = mode*1;
	
	removeAllOption("selectCWMinSTA");
	removeAllOption("selectCWMaxSTA");
	removeAllOption("selectCWMinAP");
	removeAllOption("selectCWMaxAP");
	if (mode == 0) {
		changeFieldContent("<!--#tr id=\"w.wmm.1-1\" -->Background<!--#endtr-->");
		document.getElementById("MSG_MaxNumOfPackets").innerHTML = "(0-7)";
		document.getElementById("MSG_MaxNumOfBytes").innerHTML = "(0-16000)";
		/*document.getElementById("selectCWMinSTA").name = "AC_BK_CWmin";
		document.getElementById("selectCWMaxSTA").name = "AC_BK_CWmax";
		document.getElementById("selectCWMinAP").name = "AC_BK_CWmin_AP";
		document.getElementById("selectCWMaxAP").name = "AC_BK_CWmax_AP";*/
		
		addNewOption("selectCWMinSTA", "STACWMin_15", "STACWMin_15", "4", "15");
		addNewOption("selectCWMinSTA", "STACWMin_31", "STACWMin_31", "5", "31");
		addNewOption("selectCWMinSTA", "STACWMin_63", "STACWMin_63", "6", "63");
		addNewOption("selectCWMinSTA", "STACWMin_127", "STACWMin_127", "7", "127");
		addNewOption("selectCWMinSTA", "STACWMin_255", "STACWMin_255", "8", "255");
		addNewOption("selectCWMinSTA", "STACWMin_511", "STACWMin_511", "9", "511");
		addNewOption("selectCWMinSTA", "STACWMin_1023", "STACWMin_1023", "10", "1023");
		
		addNewOption("selectCWMaxSTA", "STACWMax_15", "STACWMax_15", "4", "15");
		addNewOption("selectCWMaxSTA", "STACWMax_31", "STACWMax_31", "5", "31");
		addNewOption("selectCWMaxSTA", "STACWMaxx_63", "STACWMaxx_63", "6", "63");
		addNewOption("selectCWMaxSTA", "STACWMax_127", "STACWMax_127", "7", "127");
		addNewOption("selectCWMaxSTA", "STACWMax_255", "STACWMax_255", "8", "255");
		addNewOption("selectCWMaxSTA", "STACWMax_511", "STACWMax_511", "9", "511");
		addNewOption("selectCWMaxSTA", "STACWMax_1023", "STACWMax_1023", "10", "1023");
		
		addNewOption("selectCWMinAP", "APCWMin_15", "APCWMin_15", "4", "15");
		addNewOption("selectCWMinAP", "APCWMin_31", "APCWMin_31", "5", "31");
		addNewOption("selectCWMinAP", "APCWMin_63", "APCWMin_63", "6", "63");
		addNewOption("selectCWMinAP", "APCWMin_127", "APCWMin_127", "7", "127");
		addNewOption("selectCWMinAP", "APCWMin_255", "APCWMin_255", "8", "255");
		addNewOption("selectCWMinAP", "APCWMin_511", "APCWMin_511", "9", "511");
		addNewOption("selectCWMinAP", "APCWMin_1023", "APCWMin_1023", "10", "1023");
		
		addNewOption("selectCWMaxAP", "APCWMax_15", "APCWMax_15", "4", "15");
		addNewOption("selectCWMaxAP", "APCWMax_31", "APCWMax_31", "5", "31");
		addNewOption("selectCWMaxAP", "APCWMax_63", "APCWMax_63", "6", "63");
		addNewOption("selectCWMaxAP", "APCWMax_127", "APCWMax_127", "7", "127");
		addNewOption("selectCWMaxAP", "APCWMax_255", "APCWMax_255", "8", "255");
		addNewOption("selectCWMaxAP", "APCWMax_511", "APCWMax_511", "9", "511");
		addNewOption("selectCWMaxAP", "APCWMax_1023", "APCWMax_1023", "10", "1023");		
		changeBKValue();
	}
	else if (mode == 1) {
		changeFieldContent("<!--#tr id=\"w.wmm.1-2\" -->Best Effort<!--#endtr-->");
		document.getElementById("MSG_MaxNumOfPackets").innerHTML = "(0-10)";
		document.getElementById("MSG_MaxNumOfBytes").innerHTML = "(0-20000)";
		
		/*document.getElementById("selectCWMinSTA").name = "AC_BE_CWmin";
		document.getElementById("selectCWMaxSTA").name = "AC_BE_CWmax";
		document.getElementById("selectCWMinAP").name = "AC_BE_CWmin_AP";
		document.getElementById("selectCWMaxAP").name = "AC_BE_CWmax_AP";*/
		
		addNewOption("selectCWMinSTA", "STACWMin_15", "STACWMin_15", "4", "15");
		addNewOption("selectCWMinSTA", "STACWMin_31", "STACWMin_31", "5", "31");
		addNewOption("selectCWMinSTA", "STACWMin_63", "STACWMin_63", "6", "63");
		addNewOption("selectCWMinSTA", "STACWMin_127", "STACWMin_127", "7", "127");
		addNewOption("selectCWMinSTA", "STACWMin_255", "STACWMin_255", "8", "255");
		addNewOption("selectCWMinSTA", "STACWMin_511", "STACWMin_511", "9", "511");
		addNewOption("selectCWMinSTA", "STACWMin_1023", "STACWMin_1023", "10", "1023");
		
		addNewOption("selectCWMaxSTA", "STACWMax_15", "STACWMax_15", "4", "15");
		addNewOption("selectCWMaxSTA", "STACWMax_31", "STACWMax_31", "5", "31");
		addNewOption("selectCWMaxSTA", "STACWMaxx_63", "STACWMaxx_63", "6", "63");
		addNewOption("selectCWMaxSTA", "STACWMax_127", "STACWMax_127", "7", "127");
		addNewOption("selectCWMaxSTA", "STACWMax_255", "STACWMax_255", "8", "255");
		addNewOption("selectCWMaxSTA", "STACWMax_511", "STACWMax_511", "9", "511");
		addNewOption("selectCWMaxSTA", "STACWMax_1023", "STACWMax_1023", "10", "1023");
		
		addNewOption("selectCWMinAP", "APCWMin_15", "APCWMin_15", "4", "15");
		addNewOption("selectCWMinAP", "APCWMin_31", "APCWMin_31", "5", "31");
		addNewOption("selectCWMinAP", "APCWMin_63", "APCWMin_63", "6", "63");
		
		addNewOption("selectCWMaxAP", "APCWMax_15", "APCWMax_15", "4", "15");
		addNewOption("selectCWMaxAP", "APCWMax_31", "APCWMax_31", "5", "31");
		addNewOption("selectCWMaxAP", "APCWMax_63", "APCWMax_63", "6", "63");
		changeBEValue();
	}
	else if (mode == 2) {
		changeFieldContent("<!--#tr id=\"w.wmm.1-3\" -->Video<!--#endtr-->");
		document.getElementById("MSG_MaxNumOfPackets").innerHTML = "(0-7)";
		document.getElementById("MSG_MaxNumOfBytes").innerHTML = "(0-16000)";
		
		/*document.getElementById("selectCWMinSTA").name = "AC_VI_CWmin";
		document.getElementById("selectCWMaxSTA").name = "AC_VI_CWmax";
		document.getElementById("selectCWMinAP").name = "AC_VI_CWmin_AP";
		document.getElementById("selectCWMaxAP").name = "AC_VI_CWmax_AP";*/
		
		addNewOption("selectCWMinSTA", "STACWMin_7", "STACWMin_7", "3", "7");		
		addNewOption("selectCWMaxSTA", "STACWMax_15", "STACWMax_15", "4", "15");
		addNewOption("selectCWMinAP", "APCWMin_7", "APCWMin_7", "3", "7");		
		addNewOption("selectCWMaxAP", "APCWMax_15", "APCWMax_15", "4", "15");
		changeVIValue();
	}
	else if (mode == 3) {
		changeFieldContent("<!--#tr id=\"w.wmm.1-4\" -->Voice<!--#endtr-->");
		document.getElementById("MSG_MaxNumOfPackets").innerHTML = "(0-2)";
		document.getElementById("MSG_MaxNumOfBytes").innerHTML = "(0-16000)";
		
		/*document.getElementById("selectCWMinSTA").name = "AC_VO_CWmin";
		document.getElementById("selectCWMaxSTA").name = "AC_VO_CWmax";
		document.getElementById("selectCWMinAP").name = "AC_VO_CWmin_AP";
		document.getElementById("selectCWMaxAP").name = "AC_VO_CWmax_AP";*/
		
		addNewOption("selectCWMinSTA", "STACWMin_3", "STACWMin_3", "2", "3");		
		addNewOption("selectCWMaxSTA", "STACWMax_7", "STACWMax_7", "3", "7");
		addNewOption("selectCWMinAP", "APCWMin_3", "APCWMin_3", "2", "3");		
		addNewOption("selectCWMaxAP", "APCWMax_7", "APCWMax_7", "3", "7");
		changeVOValue();
	}
	else {
		alert("<!--#tr id=\"w.wmm.alert.1\" -->Can't get mode.<!--#endtr-->");
	}
}

function isNumber(val){
	//var reg = /^[0-9]*$/;
	var reg = /^0$|^[1-9][0-9]*$/; //user could only input 0(only 1 digit) or a number which fist digit is not 0
	return reg.test(val);
}

function checkValue() {
	var applyValue = true;
	var tempTagValue;

	docTemp = document.getElementById("AC_BK_MaxNumOfPackets").value;

	tempTagValue = document.getElementById("accessMode").value;
	if ((tempTagValue*1 == 0) || (tempTagValue*1 == 2))
	{
		if (!isNumber(docTemp) || (docTemp*1 < 0) || (docTemp*1 > 7)) {
			alert("<!--#tr id=\"w.wmm.alert.2\" -->The Max. Number Of Packets in aggregation must between<!--#endtr-->" + " 0 ~ 7 !");
			applyValue = false;
			return;
		}
	}
	else if (tempTagValue*1 == 1)
	{
		if (!isNumber(docTemp) || (docTemp*1 < 0) || (docTemp*1 > 10)) {
			alert("<!--#tr id=\"w.wmm.alert.3\" -->The Max. Number Of Packets in aggregation must between<!--#endtr-->" + " 0 ~ 10 !");
			applyValue = false;
			return;
		}
	}
	else if (tempTagValue*1 == 3)
	{
		if (!isNumber(docTemp) || (docTemp*1 < 0) || (docTemp*1 > 2)) {
			alert("<!--#tr id=\"w.wmm.alert.4\" -->The Max. Number Of Packets in aggregation must between<!--#endtr-->" + " 0 ~ 2 !");
			applyValue = false;
			return;
		}
	}
	
	docTemp = document.getElementById("AC_BK_MaxNumOfBytes").value;

	tempTagValue = document.getElementById("accessMode").value;
	if (tempTagValue*1 != 1)
	{
		if (!isNumber(docTemp) || (docTemp*1 < 0) || (docTemp*1 > 16000)) {
			alert("<!--#tr id=\"w.wmm.alert.5\" -->The Max. Aggregation size must between<!--#endtr-->" + " 0 ~ 16000 !");
			applyValue = false;
			return;
		}
	}
	else
	{
		if (!isNumber(docTemp) || (docTemp*1 < 0) || (docTemp*1 > 20000)) {
			alert("<!--#tr id=\"w.wmm.alert.6\" -->The Max. Aggregation size must between<!--#endtr-->" + " 0 ~ 20000 !");
			applyValue = false;
			return;
		}
	}
	
	docTemp = document.getElementById("AC_BK_TimeoutInterval").value;

	if (!isNumber(docTemp) || (docTemp*1 < 0) || (docTemp*1 > 100)) {
		alert("<!--#tr id=\"w.wmm.alert.11\" -->The Timeout Interval value must between<!--#endtr-->" + " 0 ~ 100 !");
		applyValue = false;
		return;
	}
	
	docTemp = document.getElementById("AC_BK_MinSizeOfPacketInAggr").value;
	if (!isNumber(docTemp) || (docTemp*1 < 0) || (docTemp*1 > 1500)) {
		alert("<!--#tr id=\"w.wmm.alert.8\" -->The Min. size of packet in Aggregation must between 0 and 1500 !<!--#endtr-->");
		applyValue = false;
		return;
	}
	
	docTemp = document.getElementById("AC_BK_ADDBATimeout").value;
	if (!isNumber(docTemp) || (docTemp*1 < 0) || (docTemp*1 > 65535)) {
		alert("<!--#tr id=\"w.wmm.alert.9\" -->The ADDBA timeout value must between 0 and 65535 !<!--#endtr-->");
		applyValue = false;
		return;
	}
	
	docTemp = document.getElementById("AC_BK_AggregationWindowSize").value;
	if (!isNumber(docTemp) || (docTemp*1 < 0) || (docTemp*1 > 64)) {
		alert("<!--#tr id=\"w.wmm.alert.10\" -->The Aggregation Window Size must between 0 and 64 !<!--#endtr-->");
		applyValue = false;
		return;
	}
	
	if (applyValue)
	{
		//Jacky.Yang 13-Dec-2007 - waitting page and redirect url
		//totalWaitTime = 100; //second
		//wait_page();
		document.getElementById("waitPad").style.display="block";
	
		document.accessControl.submit();
		//return false;
	}
}

function stationMode() {
	objectDisplay("selectCWMinSTAField", "none");
	objectDisplay("selectCWMaxSTAField", "none");
	objectDisplay("AC_BK_AIFSNField", "none");
	objectDisplay("AC_BK_TXOPField", "none");
	objectDisplay("selectCWMinAPField", "none");
	objectDisplay("selectCWMaxAPField", "none");
	objectDisplay("AC_BK_AIFSN_APField", "none");
	objectDisplay("AC_BK_TXOP_APField", "none");
}

function apMode() {
	objectDisplay("selectCWMinSTAField", "block");
	objectDisplay("selectCWMaxSTAField", "block");
	objectDisplay("AC_BK_AIFSNField", "block");
	objectDisplay("AC_BK_TXOPField", "block");
	objectDisplay("selectCWMinAPField", "block");
	objectDisplay("selectCWMaxAPField", "block");
	objectDisplay("AC_BK_AIFSN_APField", "block");
	objectDisplay("AC_BK_TXOP_APField", "block");
}

function page_load() {
	fwUpgraceStatus("<% getFWUpgrade(); %>", "<% getCurrectLanIP(); %>");
	wpsStatus("<% getWPSStatus(); %>", "");
	
	var mode = "<% getParam(1, "network_type"); %>";
	var wirelessConfigType = "<% getParam(1, "wirelessConfigType"); %>";
	var CurrentWMM = "<% getParam(1, "CurrentWMM"); %>";
	
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

	document.getElementById("accessMode").options[CurrentWMM].selected = true;
	selectAccessMode(CurrentWMM*1);
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
									<td class="subMenuOption"><font class="small"><a href="/station/wireless_basic.asp">Basic Wireless Settings</a></font></td>
									<td class="subMenuDIV">|</td>
									<td class="subMenuOption"><font class="small"><a href="/station/wps_status.asp">Wi-Fi Protected Setup™</a></font></td>
									<td class="subMenuDIV">|</td>
									<td class="subMenuOption"><font class="small"><a href="/station/site_survey.asp">Wireless Network Site Survey</a></font></td>
									<td class="subMenuDIV">|</td>
									<td class="subMenuOption">WMM®</td>
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
	<form method="post" name="accessControl" action="/goform/accessControl">
	<table class="mainTable" cellspacing="0">
		<tr>
			<td class="noSPACE">
				<table class="mainTableContent" cellspacing="0">
					<tr>
						<td class="subMenuMainContent" colspan="2"><!--#tr id="w.wmm.title" -->WMM®<!--#endtr--></td>
						<td colspan="2" class="blankContent"></td>
					</tr>
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
					<tr id="accessModeField">
						<td class="subMenuSubContent"><!--#tr id="w.wmm.1" -->Access Mode<!--#endtr--></td>
						<td class="subMenuLeftSide"></td>
					  <td colspan="2" class="blankContent">
						<input type="hidden" id="nowMode" name="nowMode" value="">
						<input type="hidden" id="CurrentWMM" name="CurrentWMM" value="">
						<select id="accessMode" name="accessMode" onchange="selectAccessMode(this.value);">
                          <option id="background" value="0"><!--#tr id="w.wmm.1-1" -->Background<!--#endtr--></option>
                          <option id="bestEffort" value="1"><!--#tr id="w.wmm.1-2" -->Best Effort<!--#endtr--></option>
                          <option id="video" value="2"><!--#tr id="w.wmm.1-3" -->Video<!--#endtr--></option>
                          <option id="voice" value="3"><!--#tr id="w.wmm.1-4" -->Voice<!--#endtr--></option>
                        </select></td>
					</tr>
					<tr>
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="blankContent" colspan="2"><hr /></td>
					</tr>
					<tr>
						<td class="subMenuSubContent" id="accessModeStatus"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel" id="tdUseAggregation"><!--#tr id="w.wmm.1-5" -->Use Aggregation:<!--#endtr--></td>
						<td class="tdContent">
							<select id="AC_BK_UseAggregation" name="AC_BK_UseAggregation" onchange="">
								<option value="0"><!--#tr id="w.wmm.1-6" -->No<!--#endtr--></option>
								<option value="1"><!--#tr id="w.wmm.1-7" -->Yes<!--#endtr--></option>
							</select>						</td>
					</tr>
		        	<tr>
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel" id="tdAcceptAggregation"><!--#tr id="w.wmm.1-8" -->Accept Aggregation:<!--#endtr--></td>
						<td class="tdContent">
							<select id="AC_BK_AcceptAggregation" name="AC_BK_AcceptAggregation" onchange="">
								<option value="0"><!--#tr id="w.wmm.1-9" -->No<!--#endtr--></option>
								<option value="1" selected><!--#tr id="w.wmm.1-10" -->Yes<!--#endtr--></option>
						</select>						</td>
					</tr>
					<tr>
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel" id="tdMaxNumOfPacket"><!--#tr id="w.wmm.1-11" -->Max. Number of Packets in Aggregation:<!--#endtr--></td>
						<td class="tdContent">
							<input type="text" id="AC_BK_MaxNumOfPackets" name="AC_BK_MaxNumOfPackets" size="7" maxlength="2" value=""> <span id="MSG_MaxNumOfPackets">(0~7)</span>						</td>
					</tr>
					<tr>
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel" id="tdMaxSize"><!--#tr id="w.wmm.1-12" -->Max. Aggregation Size:<!--#endtr--></td>
						<td class="tdContent">
							<input type="text" id="AC_BK_MaxNumOfBytes" name="AC_BK_MaxNumOfBytes" size="7" maxlength="5" value=""> <span id="MSG_MaxNumOfBytes">(0~16000)</span>						</td>
					</tr>
					<tr>
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel" id="tdTimeoutInterval"><!--#tr id="w.wmm.1-13" -->Timeout Interval:<!--#endtr--></td>
						<td class="tdContent">
							<input type="text" id="AC_BK_TimeoutInterval" name="AC_BK_TimeoutInterval" size="7" maxlength="3" value=""> <!--#tr id="w.wmm.1-13d" -->(0-100ms)<!--#endtr-->						</td>
					</tr>
					<tr>
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel" id="tdMinSize"><!--#tr id="w.wmm.1-14" -->Min. Size of Packets in Aggregation:<!--#endtr--></td>
						<td class="tdContent">
							<input type="text" id="AC_BK_MinSizeOfPacketInAggr" name="AC_BK_MinSizeOfPacketInAggr" size="7" maxlength="4" value=""> (0~1500)						</td>
					</tr>
					<tr>
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel" id="tdADDBATimeout"><!--#tr id="w.wmm.1-15" -->ADDBA Timeout:<!--#endtr--></td>
						<td class="tdContent">
							<input type="text" id="AC_BK_ADDBATimeout" name="AC_BK_ADDBATimeout" size="7" maxlength="5" value=""> (0~65535)						</td>
					</tr>
					<tr>
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel" id="tdWindowSize"><!--#tr id="w.wmm.1-16" -->Aggregation Window Size:<!--#endtr--></td>
						<td class="tdContent">
							<input type="text" id="AC_BK_AggregationWindowSize" name="AC_BK_AggregationWindowSize" size="7" maxlength="2" value=""> (0~64)						</td>
					</tr>
					<tr id="selectCWMinSTAField">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel" id="tdSTACWmin"><!--#tr id="w.wmm.1-17" -->CW min for STA:<!--#endtr--></td>
						<td class="tdContent">
							<select id="selectCWMinSTA" name="selectCWMinSTA" onchange="">
							</select>						</td>
					</tr>
					<tr id="selectCWMaxSTAField">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel" id="tdSTACWMax"><!--#tr id="w.wmm.1-18" -->CW max for STA:<!--#endtr--></td>
						<td class="tdContent">
							<select id="selectCWMaxSTA" name="selectCWMaxSTA" onchange="">
							</select>						</td>
					</tr>
					<tr id="AC_BK_AIFSNField">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel" id="tdSTAAIFSN"><!--#tr id="w.wmm.1-19" -->AIFSN for STA:<!--#endtr--></td>
						<td class="tdContent">
							<input type="text" id="AC_BK_AIFSN" name="AC_BK_AIFSN" onchange="" value="">						</td>
					</tr>
					<tr id="AC_BK_TXOPField">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel" id="tdSTATXOP"><!--#tr id="w.wmm.1-20" -->TXOP for STA:<!--#endtr--></td>
						<td class="tdContent">
							<input type="text" id="AC_BK_TXOP" name="AC_BK_TXOP" onchange="" value="">						</td>
					</tr>
					<tr id="selectCWMinAPField">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel" id="tdAPCWMin"><!--#tr id="w.wmm.1-21" -->CW min for AP:<!--#endtr--></td>
						<td class="tdContent">
							<select id="selectCWMinAP" name="selectCWMinAP" onchange="">
							</select>						</td>
					</tr>
					<tr id="selectCWMaxAPField">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel" id="tdAPCWMax"><!--#tr id="w.wmm.1-22" -->CW max for AP:<!--#endtr--></td>
						<td class="tdContent">
							<select id="selectCWMaxAP" name="selectCWMaxAP" onchange="">
							</select>						</td>
					</tr>
					<tr id="AC_BK_AIFSN_APField">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel" id="tdAPAIFSN"><!--#tr id="w.wmm.1-23" -->AIFSN for AP:<!--#endtr--></td>
						<td class="tdContent">
							<input type="text" id="AC_BK_AIFSN_AP" name="AC_BK_AIFSN_AP" onchange="" value="">						</td>
					</tr>
					<tr id="AC_BK_TXOP_APField">
						<td class="subMenuSubContent"></td>
						<td class="subMenuLeftSide"></td>
						<td class="tdLabel" id="tdAPTXOP"><!--#tr id="w.wmm.1-24" -->TXOP for AP:<!--#endtr--></td>
						<td class="tdContent">
							<input type="text" id="AC_BK_TXOP_AP" name="AC_BK_TXOP_AP" onchange="" value="">						</td>
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
							<a class="bottonClass" href="javascript:checkValue()"><!--#tr id="w.wmm.save" -->Save Settings<!--#endtr--></a>
							<a class="bottonClass" href="javascript:document.location.reload(true)"><!--#tr id="w.wmm.cancel" -->Cancel Changes<!--#endtr--></a>
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
