/*
 *  This source code is copyright (c) 2010 U-Media .
 *  All rights reserved.
 *
 *  History :
 *
 *  2010.12.30 June.Chen
 *  Porting from TEW-640MB and modifying for WES-610N
 *
 *  2010.07.07 Joan.Huang
 *  For TEW-640MB detected bridge loop
 *  1.Check bridge is loop
 *     1.1 If loop then disable loop port .
 *     1.2 Every 2 second to check loop status .
 *  2.Check Physical port and wireless status
 *     2.1 If loop port link down then reset port status.
 *     2.2 Every 4 second to check it .
 */

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

#define WLAN_CONFIG_PATH	"/mnt/jffs2/wlan0.conf"
#define CONFIG_VALUE_MAXLEN	512
#define SYS_CONFIG_PATH	"/tmp/sys.conf"

int securityMode=0, authMode=0, ipConfigMethod=0;

int check_somefile_is_exist(char *file_path)
{
	struct stat statBuf;
	int result=0;

	result = stat(file_path, &statBuf);
	if(result == -1){
/*		
		if(errno!=2){
			printf("errno for stat is %d..\n", errno);
		}else{
			printf("%s is not exist..\n", file_path);		
		}
*/
		return 0;
	}

	//printf("found %s..\n", file_path);
	return 1;
}

static int is_valid_ip(const char *ip)
{
	int dots = 0, num = 0, has_digit = 0;
	const char *p = ip;
	if (!ip || !*ip) return 0;
	while (*p) {
		if (*p >= '0' && *p <= '9') {
			num = num * 10 + (*p - '0');
			if (num > 255) return 0;
			has_digit = 1;
		} else if (*p == '.') {
			if (!has_digit) return 0;
			dots++;
			num = 0;
			has_digit = 0;
		} else {
			return 0;
		}
		p++;
	}
	return (dots == 3 && has_digit);
}

int get_config(char *config_value, char *config_name, char *config_file)
{
	FILE *fptr;
	char line[512];
	size_t klen;

	if (!config_value || !config_name || !config_file)
		return -1;
	memset(config_value, 0, CONFIG_VALUE_MAXLEN);
	fptr = fopen(config_file, "r");
	if (fptr == NULL) {
		return -1;
	}
	klen = strlen(config_name);
	while (fgets(line, sizeof(line), fptr) != NULL) {
		char *p = line;
		char *eq, *nl;
		while (*p == ' ' || *p == '\t')
			p++;
		if (strncmp(p, config_name, klen) == 0 &&
		    (p[klen] == '=' || p[klen] == ' ' || p[klen] == '\t')) {
			eq = strchr(p + klen, '=');
			if (eq) {
				char *val = eq + 1;
				int out_i = 0;
				nl = strpbrk(val, "\r\n");
				if (nl) *nl = '\0';
				while (*val && out_i < CONFIG_VALUE_MAXLEN - 1) {
					if (*val != ' ' && *val != '\t')
						config_value[out_i++] = *val;
					val++;
				}
				config_value[out_i] = '\0';
				fclose(fptr);
				return 0;
			}
		}
	}
	fclose(fptr);
	return -1;
}

int getClientGateway(char *gateway, int isDHCP)
{
	char   buff[256];
	int    nl = 0 ;
	struct in_addr dest;
	struct in_addr gw;
	int    flgs, ref, use, metric, mtu, window, irtt;
	unsigned long int d,g,m;
	
	char configValue[CONFIG_VALUE_MAXLEN]={0}; //for get current setting from configuration

	if(isDHCP){
		FILE *fp = fopen("/proc/net/route", "r");
		if (fp == NULL) {
			strcpy(gateway, "0.0.0.0");
			return 0;
		}

		while (fgets(buff, sizeof(buff), fp) != NULL) {
			if (nl) {
				int ifl = 0;
				while (buff[ifl]!=' ' && buff[ifl]!='\t' && buff[ifl]!='\n' && buff[ifl]!='\0' && ifl < (int)sizeof(buff) - 1)
					ifl++;
				if (buff[ifl] == '\0') {
					continue;
				}
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
		//printf("default_gw -1 = %s (len=%d)\n", configValue, strlen(configValue));
		if(strlen(configValue)>=16)
			return -1;

		if(strlen(configValue)!=0)
			strcpy(gateway, configValue);
		else
			strcpy(gateway, "0.0.0.0");
		//printf("default_gw -2 = %s (len=%d)\n", gateway, strlen(gateway));
	}
	
	return 0;
}

int detect_layer3_link_with_ping(char *gateway)
{
	FILE *fptr;
	char command[128]={0}, pingResult[128]={0};
	char *charPtr;

	if (!is_valid_ip(gateway)) {
		return -1;
	}
	snprintf(command, sizeof(command), "/sbin/arp -d %s 2>/dev/null", gateway);
	system(command);
	memset(command, 0, sizeof(command));
	snprintf(command, sizeof(command), "/bin/ping -c 1 -W 2 %s", gateway);
	fptr = popen(command, "r");
	if(fptr!=NULL){
		int got_reply = 0, rc;
		while(fgets(pingResult, sizeof(pingResult), fptr)!=NULL){
			charPtr=strchr(pingResult, '\n');
			if(charPtr) *charPtr='\0';
			if(strstr(pingResult, "bytes from") != NULL || strstr(pingResult, "is alive!") != NULL){
				got_reply = 1;
			}
		}
		rc = pclose(fptr);
		return (rc == 0 || got_reply) ? 1 : 0;
	}
	return -1;
}

int check_gateway_in_arptable(char *gateway)
{
	FILE *arp_table;
	char buf[512]={0};
	char ip[16]={0}, hw_type[16]={0}, flags[16]={0}, hw_addr[24]={0}, mask[16]={0}, device[16]={0};
	arp_table=fopen("/proc/net/arp", "r");
	if(arp_table == NULL)
		return 0;
	fgets(buf, sizeof(buf), arp_table);
	memset(buf, 0, sizeof(buf));
	while(fgets(buf, sizeof(buf), arp_table)!=NULL){
		if(sscanf(buf, "%15s %15s %15s %23s %15s %15s", ip, hw_type, flags, hw_addr, mask, device)==6){
			if(!strcmp(gateway, ip) && strcmp(hw_addr, "00:00:00:00:00:00") && strcmp(flags, "0x0")){
				fclose(arp_table);
				return 1;
			}
		}
		memset(buf, 0, sizeof(buf));
		memset(ip, 0, sizeof(ip));
		memset(flags, 0, sizeof(flags));
		memset(hw_addr, 0, sizeof(hw_addr));
	}
	fclose(arp_table);
	//printf("Doesn't found gateway in ARP table\n");
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
	//printf("The lease status for DHCP Client is %s\n", lease_status);
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

	//printf("Detect station link status for WEP-OPEN or WEP-AUTO\n");
	
	if(ipConfigMethod==1){ //static ip
		isLinkUp = detect_layer3_link_by_packets();
		//printf("Detect wireless layer3 links status by packet, and the result is %s\n", isLinkUp==0?"DOWN": isLinkUp==1?"UP":"UNKNOW");			
	}else{
		isLinkUp = detect_layer3_link_by_dhcp();
		//printf("Detect wireless layer3 links status by DHCP status, and the result is %s\n", isLinkUp?"UP":"DOWN");			
	}

	return isLinkUp;
}

int detect_layer3_link_by_dhcp_retry(void)
{
	FILE *fptr;
	char retry_count[16]={0};

	fptr = fopen("/tmp/dhcp_client_retry_count", "r");
	if(fptr==NULL){
		return -1;
	}
	fgets(retry_count, 16, fptr);
	//printf("DHCP has require IP from DHCP server %d times\n", atoi(retry_count));
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
	FILE *fptr = fopen("/proc/net/wireless", "r");
	if (fptr == NULL) {
		return 0;
	}

	while(fgets((char *)tmpBuf, sizeof(tmpBuf), fptr) != NULL){
		if(strstr((char *)tmpBuf, "Inter-") || strstr((char *)tmpBuf, "face")){
			continue;
		}
		paramCount = sscanf((char *)tmpBuf, "%15s %d %15s %15s %15s %d %d %d %d %d %d", ifName, &link_status, link, level, noise, &nwid, &crypt, &frag, &retry, &misc, &missed_beacon);
	}
	(void)paramCount;
	fclose(fptr);

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
		//printf("Detect station link status for WPA-PSK/WPA2-PSK\n");
		fptr = popen("/root/mtlk/etc/wpa_cli status", "r");
		if(fptr!=NULL){
			int is_completed = 0;
			memset(tmpBuf, 0, sizeof(tmpBuf));
			while(fgets((char *)tmpBuf, sizeof(tmpBuf), fptr) != NULL){
				if(strncmp((char *)tmpBuf, "wpa_state=", 10) == 0){
					if(strstr((char *)tmpBuf + 10, "COMPLETED") != NULL){
						is_completed = 1;
					}
					break;
				}
			}
			pclose(fptr);
			if(is_completed){
				return 1;
			}else{
				return 0;
			}
		}else{
			//printf("ERROR: Fail to query wpa_supplicant status by wpa_cli..\n");
			goto end;
		}
	}
	
end:
	return link_status==3?1:0;
}

void initLinkUtil(void){
	char configValue[CONFIG_VALUE_MAXLEN]={0};

	if(get_config(configValue, "NonProcSecurityMode", WLAN_CONFIG_PATH)==-1){
		printf("get NonProcSecurityMode fail ..\n");
	}else{
		securityMode = atoi(configValue);
	}

	if(get_config(configValue, "NonProc_Authentication", WLAN_CONFIG_PATH)==-1){
		printf("get NonProc_Authentication fail ..\n");
	}else{
		authMode = atoi(configValue);
	}

	if(get_config(configValue, "ip_config_method", SYS_CONFIG_PATH)==-1){
		printf("get ip_config_method fail ..\n");
	}else{
		ipConfigMethod = atoi(configValue);
	}
}
