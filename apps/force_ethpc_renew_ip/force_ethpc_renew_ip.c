#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/route.h>

#define CONFIG_VALE_MAXLEN	512
#define WLAN_CONFIG_PATH	"/tmp/wlan0.conf"
#define SYS_CONFIG_PATH	"/tmp/sys.conf"

int securityMode=0, authMode=0, ipConfigMethod=0;

int gotSIGUSR1=0;

void signal_handler(int sig)
{
	if(sig == SIGUSR1){
		printf("Got a SIGUSER1 from external process, prepare to terminate force_ethpc_renew_ip..\n");
		gotSIGUSR1=1;
	}
}

int check_somefile_is_exist(char *file_path)
{
	struct stat statBuf;
	int result=0;

	result = stat(file_path, &statBuf);
	if(result == -1){
		if(errno!=2){
			printf("errno for stat is %d..\n", errno);
		}else{
			printf("%s is not exist..\n", file_path);		
		}
		return 0;
	}

	printf("found %s..\n", file_path);
	return 1;
}

int get_config(char *config_value, char *config_name, char *config_file)
{
	FILE *fptr;
	char command[256]={0};

	sprintf(command, "awk -F \"=\" '/^%s/ {str = $2; gsub(/ /, \"\", str); sub(/\\r/, \"\", str); sub(/\\n/, \"\", str); print str}' %s", config_name, config_file);

	fptr = popen(command, "r");
	if(fptr==NULL){
		printf("ERROR: popen fail for get_config..\n");
		return -1;
	}

	memset(config_value, 0, CONFIG_VALE_MAXLEN);
	if(fgets(config_value, CONFIG_VALE_MAXLEN, fptr)==NULL){
		pclose(fptr);
		return -1;
	}
	
	pclose(fptr);
	return 0;
}

int getClientGateway(char *gateway, int isDHCP)
{
	char   buff[256];
	int    nl = 0 ;
	struct in_addr dest;
	struct in_addr gw;
	int    flgs, ref, use, metric, mtu, window, irtt;
	unsigned long int d,g,m;
	
	char configValue[CONFIG_VALE_MAXLEN]={0}; //for get current setting from configuration

	if(isDHCP){
		FILE *fp = fopen("/proc/net/route", "r");

		while (fgets(buff, sizeof(buff), fp) != NULL) {
			if (nl) {
				int ifl = 0;
				while (buff[ifl]!=' ' && buff[ifl]!='\t' && buff[ifl]!='\0')
					ifl++;
				buff[ifl]=0; 
				
				if (sscanf(buff+ifl+1, "%lx%lx%X%d%d%d%lx%d%d%d",
							&d, &g, &flgs, &ref, &use, &metric, &m, &mtu, &window, &irtt)!=10) {
					continue;
				}else{
					if (flgs&RTF_UP) {
						dest.s_addr = d;
						gw.s_addr   = g;

						if (dest.s_addr == 0) {
							strcpy(gateway, (gw.s_addr==0 ? "0.0.0.0" : inet_ntoa(gw)));
							fclose(fp);
							return 0;
						}
					}
				}
			}
			nl++;
		}
		fclose(fp);

		strcpy(gateway, "0.0.0.0");
	}else{
		get_config(configValue, "default_gw", SYS_CONFIG_PATH);
		printf("default_gw -1 = %s (len=%d)\n", configValue, strlen(configValue));
		if(strlen(configValue)>=16)
			return -1;

		if(strlen(configValue)!=0)
			strcpy(gateway, configValue);
		else
			strcpy(gateway, "0.0.0.0");
		printf("default_gw -2 = %s (len=%d)\n", gateway, strlen(gateway));
	}
	
	return 0;
}

int detect_layer3_link_with_ping(char *gateway)
{
	FILE *fptr;
	char command[128]={0}, pingResult[128]={0}, checkResult[128]={0};
	char *charPtr;

	//printf("Current LAN Gateway = %s\n", gateway);		
	memset(command, 0, 128);
	//Clear gateway entry in arp table
	sprintf(command, "/sbin/arp -d %s", gateway);
	memset(command, 0, 128);
	//Send a echo request for update arp table
	sprintf(command, "/bin/ping %s", gateway);
	fptr = popen(command, "r");
	if(fptr!=NULL){
		if(fgets(pingResult, 128, fptr)!=NULL){
			charPtr=strchr(pingResult, '\n');
			if(charPtr){
				*charPtr='\0';
			}
			sprintf(checkResult, "%s is alive!", gateway);
			printf("The result of ping is %s\nThe string for check result is %s\n", pingResult, checkResult);
			if(!strcmp(pingResult, checkResult)){
				printf("The Gateway %s is reachable, wireless link is UP\n", gateway);
				pclose(fptr);
				return 1;
			}else{
				printf("The Gateway %s is unreachable, wireless link is DOWN\n", gateway);
				pclose(fptr);
				return 0;
			}
		}else{
			printf("ERROR: Get ping result fail !!\n");
		}
		pclose(fptr);
	}else{
		printf("ERROR: ping fail !!\n");
	}
	return -1;
}

int check_gateway_in_arptable(char *gateway)
{
	FILE *arp_table;
	char buf[512]={0};
	char ip[16]={0}, hw_type[16]={0}, flags[16]={0}, hw_addr[24]={0}, mask[16]={0}, device[8]={0};
	arp_table=popen("cat /proc/net/arp", "r");
	fgets(buf, 512, arp_table);
	memset(buf, 0, 512);
	while(fgets(buf, 512, arp_table)!=NULL){
		if(sscanf(buf, "%s %s %s %s %s %s", ip, hw_type, flags, hw_addr, mask, device)==6){
			printf("gateway=%s, arp record: %s %s %s %s %s %s\n", gateway, ip, hw_type, flags, hw_addr, mask, device);
			if(!strcmp(gateway, ip) && strcmp(hw_addr, "00:00:00:00:00:00") && strcmp(flags, "0x0")){
				printf("Found gateway in ARP table\n");
				pclose(arp_table);
				return 1;
			}
		}
		memset(buf, 0, 512);
		memset(ip, 0, 16);
		memset(flags, 0, 16);
		memset(hw_addr, 0, 24);
	}
	pclose(arp_table);
	printf("Doesn't found gateway in ARP table\n");
	return 0;
}

int detect_layer3_link_by_packets(void)
{
	char	gateway[16]={0};

	if(getClientGateway(gateway, 0) == -1){
		printf("ERROR: Get LAN Gateway fail .. \n");
		return -1;
	}
	if(!strcmp(gateway, "0.0.0.0")){
		printf("The default gateway deesn't be configured\n");
		return -1;
	}

	detect_layer3_link_with_ping(gateway);

	//Because ping command in Metalink SDK is abnormal, it can receive reply from none-exist device
	//So, I skip to check the ping result, ping to gateway just for update arp table
	//I always use ARP table to determine wireless link status
	return check_gateway_in_arptable(gateway);
}

int detect_layer3_link_by_dhcp(void)
{
	FILE *fptr;
	char lease_status[16]={0};
	int i=0;
	char	gateway[16]={0};

	fptr = fopen("/tmp/dhcp_client_lease_status", "r");
	if(fptr==NULL){
		return 0;
	}
	fgets(lease_status, 16, fptr);
	while(i<strlen(lease_status)){
		if(*(lease_status+i) == '\n'){
			*(lease_status+i) = '\0';
		}
		i++;
	}
	printf("The lease status for DHCP Client is %s\n", lease_status);
	fclose(fptr);
	
	if(!strncmp(lease_status, "bind", 5)){
		/*if((MT_getClientGateway(gateway, 1) == -1) || !strcmp(gateway, "0.0.0.0")){
			printf("ERROR: Get LAN Gateway fail .. \n");
			//Get gateway address fail or default gateway doesn't be configured by DHCP server, 
			//Use DHCP client status to determine wireless link status			
			return 1;
		}
		
		MT_detect_link_with_ping(gateway);
		//Because ping command in Metalink SDK is abnormal, it can receive reply from none-exist device
		//So, I skip to check the ping result, ping to gateway just for update arp table
		//I always use ARP table to determine wireless link status
		return MT_detect_check_gateway_in_arptable(gateway);*/
		return 1;
	}

	return 0;
}

int detect_station_layer3_link_status(void)
{
	int isLinkUp = 0;

	printf("Detect station link status for WEP-OPEN or WEP-AUTO\n");
	
	if(ipConfigMethod==1){ //static ip
		isLinkUp = detect_layer3_link_by_packets();
		printf("Detect wireless layer3 links status by packet, and the result is %s\n", isLinkUp==0?"DOWN": isLinkUp==1?"UP":"UNKNOW");			
	}else{
		isLinkUp = detect_layer3_link_by_dhcp();
		printf("Detect wireless layer3 links status by DHCP status, and the result is %s\n", isLinkUp?"UP":"DOWN");			
	}

	return isLinkUp;
}

int detect_layer3_link_by_dhcp_retry(void)
{
	FILE *fptr;
	char retry_count[16]={0};
	int i=0;

	fptr = fopen("/tmp/dhcp_client_retry_count", "r");
	if(fptr==NULL){
		return -1;
	}
	fgets(retry_count, 16, fptr);
	printf("DHCP has require IP from DHCP server %d times\n", atoi(retry_count));
	fclose(fptr);

	if(atoi(retry_count) >= 2){
		return 0;
	}	

	return 1;
}

int get_station_layer3_link_status(void)
{
	uint8_t tmpBuf[512] = {0};
	uint8_t ifName[16] = {0};
	int link_status=0, nwid, crypt, frag, retry, misc, missed_beacon/*, we*/;
	uint8_t link[16]={0}, level[16]={0}, noise[16]={0};
	int paramCount = 0;	
	FILE *fptr = popen("cat /proc/net/wireless", "r");

	while(fgets(tmpBuf, 512, fptr) != NULL){
		if(strstr(tmpBuf, "Inter-") || strstr(tmpBuf, "face")){
			continue;
		}
		
		paramCount = sscanf(tmpBuf, "%s %d %s %s %s %d %d %d %d %d %d", ifName, &link_status, link, level, noise, &nwid, &crypt, &frag, &retry, &misc, &missed_beacon);
	}
	
	pclose(fptr);

	//If security is configured to WPA-PSK, WEP-OPEN or WEP-AUTO, 
	//then we need to detect wireless links status by myself.
	//Because Metalink driver will report associated before link real UP when
	//configured to WPA-PSK, WEP-OPEN or WEP-AUTO.
//june.chen, 2010-12-24, change WEP SHARE to 2
	if(link_status==3 && securityMode==2 && authMode!=2){
		int detectedLinkStatus=0;
		detectedLinkStatus=detect_station_layer3_link_status();
		if(detectedLinkStatus!=-1){
			if(detectedLinkStatus==0){
				if(ipConfigMethod==0){
					if(detect_layer3_link_by_dhcp_retry()){
						return detectedLinkStatus;
					}else{
						link_status=0;
						goto end;
					}
				}
			}
			return detectedLinkStatus;
		}
		//If detect wireless link my myself fail, then I will use driver report for link status.
	}else if(link_status==3 && (securityMode==3 || securityMode==4)){
		printf("Detect station link status for WPA-PSK/WPA2-PSK\n");
		system("/root/mtlk/etc/wpa_cli status |grep wpa_state > /tmp/wpa_state");
		fptr = popen("awk -F \"=\" '/^wpa_state/ {str = $2; gsub(/ /, \"\", str); sub(/\\r/, \"\", str); sub(/\\n/, \"\", str); print str}' /tmp/wpa_state", "r");
		if(fptr!=NULL){
			memset(tmpBuf, 0, 512);
			fgets(tmpBuf, 512, fptr);
			pclose(fptr);
			system("rm /tmp/wpa_state");
			printf("INFO: The state of wpa_supplicant is %s", tmpBuf);
			if(strstr(tmpBuf, "COMPLETED")){
			//june.chen, 20110316, add this check for ensuring ethernet not down/up when DUT does not get IP from DHCP server
#if 1
				int detectedLinkStatus = 0;
				detectedLinkStatus = detect_station_layer3_link_status();

				if(detectedLinkStatus != -1){
					if(detectedLinkStatus == 0){
						if(ipConfigMethod == 0){
							if(detect_layer3_link_by_dhcp_retry()){
		                       				return detectedLinkStatus;
                			      		}else{
			                        		link_status =0 ;
                        			    		goto end;
			                       		}
			              		}
			      		}
			  		return detectedLinkStatus;
				}	
#endif
//june.chen end
				return 1;
			}else{
				return 0;
			}
		}else{
			printf("ERROR: Fail to query wpa_supplicant status by wpa_cli..\n");
			goto end;
		}
	}
	
end:
	return link_status==3?1:0;
}

int main(int argc, char** argv)
{
	char configValue[CONFIG_VALE_MAXLEN]={0};
	char buf[128]={0};
	int tempCount=0;

	printf("daemon start for force PC at ethernet port of device to renew ..\n");
	sprintf(buf, "echo %d > /var/run/force_ethpc_renew_ip", getpid());
	system(buf);

	struct sigaction sa;   
	memset(&sa, 0, sizeof(sa));   
    	sa.sa_handler = &signal_handler;   
    	sigaction(SIGUSR1, &sa, NULL);   

	if(get_config(configValue, "NonProcSecurityMode", WLAN_CONFIG_PATH)==-1){
		printf("get NonProcSecurityMode fail ..\n");
	}else{
		securityMode = atoi(configValue);
	}

	if(get_config(configValue, "NonProc_Authentication", WLAN_CONFIG_PATH)==-1){
		printf("get NonProcSecurityMode fail ..\n");
	}else{
		authMode = atoi(configValue);
	}

	if(get_config(configValue, "ip_config_method", SYS_CONFIG_PATH)==-1){
		printf("get ip_config_method fail ..\n");
	}else{
		ipConfigMethod = atoi(configValue);
	}	

	//printf("securityMode=%d, authMode=%d, ipConfigMethod=%d\n", securityMode, authMode, ipConfigMethod);
	//printf("layer3 link status for wireless is %s\n", get_station_layer3_link_status()==1?"UP":"DOWN");

	while(1){
		///var/gui_wps_waiting check is for Jacky's GUI redirect check
		if(get_station_layer3_link_status()==1 && check_somefile_is_exist("/var/gui_wps_waiting")==0){
			//Jacky.Yang 10-Jun-2009
			if(check_somefile_is_exist("/var/assign_get_ip")==1 || check_somefile_is_exist("/var/webCommit")==0 || tempCount > 20){
				printf("Detected wireless layer3 link is UP, do power cycle of PHY to force PC at Ethernet port to renew IP. (tempCount=%d)\n", tempCount);
				system("ifconfig eth0 down");
				sleep(5);
				system("rm -rf /var/Get_IPInfo_From_DHCP");
				system("ifconfig eth0 up");
				break;
			}
			else
				tempCount++;
		}
		if(gotSIGUSR1){
			break;
		}
		
		sleep(1);
	}
	
	system("rm /var/run/force_ethpc_renew_ip");
	printf("Terminate force_ethpc_renew_ip ..\n");

	return 0;
}

