<html>
<head>
<META HTTP-EQUIV="CACHE-CONTROL" CONTENT="NO-CACHE">
<meta http-equiv="Pragma" content="no-cache">
<meta http-equiv="expires" content="Mon, 22 Jul 1999 11:12:01 GMT">

</head>
<body>
<%
	NonProc_ESSID = NonProc_tmpESSID;
	applyParams ("NLDParamName","NonProc_Authentication","NonProc_WepKeyLength","NonProc_WPA_Enterprise_Encapsulation","NonProc_tmpESSID","NonProc_ESSID");
	applyParams ("NonProc_WPA_Enterprise_Mode","NonProc_WPA_Enterprise_Radius_Password","NonProc_WPA_Enterprise_Radius_Username");
	applyParams ("NonProc_WPA_Personal_Encapsulation","NonProc_WPA_Personal_Mode","NonProc_WPA_Personal_PSK","NonProcSecurityMode");
	applyParams ("WepEncryption","WepKeys_DefaultKey0","WepKeys_DefaultKey1","WepKeys_DefaultKey2","WepKeys_DefaultKey3","WepTxKeyIdx");
	applyParams ("Wildcard_ESSID");
	'set the NeverConnected parameter to 0 so that supplicant will be activated after reboot'
	NeverConnected = 0;
	applyParams ("NeverConnected");
%>
<script type="text/javascript">
<!--
	window.location = "/cgi-bin/scan.tcl?name=Activate"
//-->
</script>
</body>
</html>