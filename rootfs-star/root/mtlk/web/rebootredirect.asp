<html>
<head>
<meta http-equiv="cache-control" content="no-cache">
<meta http-equiv="pragma" content="no-cache">
<meta http-equiv="expires" content="0">
<title>Commit</title>
<link rel='stylesheet' href='/normal_ws.css' type='text/css'>
<% language=javascript %>
<script language="JavaScript" type="text/javascript">
	var IPandMask = "<% getCurrectLanIP(); %>";
	var urlPath = "<% getUrlPath(); %>";
	var currentIP;
	docTemp = IPandMask;
	index = docTemp.indexOf(','); //IP
	currentIP = docTemp.substr(0, index);
	
	function page_load()
	{
		//alert("http://" + currentIP + urlPath);
		top.location.href = "http://" + currentIP + urlPath;
		//top.location.href = urlPath;
	}
</script>
</head>
<body onLoad="page_load();">
WEB
</body>
</html>
