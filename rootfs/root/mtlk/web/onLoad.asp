
// Collect ENV Params
exec (web_root+"/mtlk_read_ENV.sh","/tmp/ENVPARAMS.ini","mac_lan","ethaddr");

loadConfig("/tmp/HW.ini");
loadConfig("/tmp/ENVPARAMS.ini");
loadConfig("/tmp/updates.ini");
exec ("rm","/tmp/updates.ini");

if (HW_24G != 1 && HW_52G != 1)
{
	HW_24G = 1;
	HW_52G = 1;
}	
if (HW_24G == 0 && FrequencyBand != 0)
{
	FrequencyBand=0;
	Channel=36; 
	
	if (network_type == 2)
	{
		NetworkMode=14;
		applyParams("FrequencyBand","Channel","NetworkMode");
	} 
	else
    {
		NetworkModeSTA=14;
		applyParams("FrequencyBand","Channel","NetworkModeSTA");
    }	
	
	
} else if (HW_52G == 0 && FrequencyBand != 1)
{
	FrequencyBand=1;
	Channel=7;
	if (network_type == 2)
	{
		NetworkMode=23;
		applyParams("FrequencyBand","Channel","NetworkMode");
	} 
	else
    {
		NetworkModeSTA=23;
		applyParams("FrequencyBand","Channel","NetworkModeSTA");
    }		
}

if (network_type==2)
{
	Enrollee_type = 0;
}

DBG = 0
MSEC = 0
NonProc_WPS_manualAPselection = 0