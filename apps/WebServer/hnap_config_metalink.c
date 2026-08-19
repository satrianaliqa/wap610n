
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/route.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#include "uemf.h"
#include "webs.h"
#include "mt_api.h" //add for metalink goahead
#include "wsIntrn.h"
#include "xml_tree.h"
#include "expat.h"
#include "hnap_interfaces.h"
#include "hnap_config_metalink.h"
#include "zlib.h"

#define DEFAULT_FLASH_FILE		"/dev/mtdblock2"
#define MAX_RETRY_BURN_TIMES	3

int hnap_is_multicast_mac(char *str)
{
	unsigned int tmp[6];
	
   	sscanf(str, "%02X:%02X:%02X:%02X:%02X:%02X", &tmp[0], &tmp[1], &tmp[2], &tmp[3], &tmp[4], &tmp[5]);

	if(tmp[0]&0x01){
		return 1;
	}

	return 0;
}

int hnap_valid_mac(char *str)
{
	int i, len = strlen(str);
	
	if(len != 17)
		return 0;

	for(i=0; i<5; i++){
		if( (!isxdigit( str[i*3])) || (!isxdigit( str[i*3+1])) || (str[i*3+2] != ':') )
			return 0;
	}

	if(!(isxdigit(str[15]) && isxdigit(str[16]))){
		return 0;
	}

	if(hnap_is_multicast_mac(str)){
		return 0;
	}
	
	return 1;
}

/*
 * is_valid_subnet_mask()
 *	Verify that a subnet mask is valid.
 */
int hnap_valid_subnet_mask(unsigned long mask)
{
	if (!mask || (mask == 0xffffffff)) {
		return FALSE;
	}

	/*
	 * Ensure that we don't have any more significant zero bits after the least
	 * significant one bit.
	 */
	mask = (~mask) + 1;
	if ((mask & (-mask)) != mask) {
		return FALSE;
	}
	
	return TRUE;
}

/*
 * is_valid_host_ip
 *	Verify that an IP address is valid.
 */
int hnap_valid_host_ip(unsigned long ip)
{
	/*
	 * Don't allow non-unicast or loopback IP addresses.
	 */
	return ((ip != 0) && (ip < 0xe0000000) && ((ip & 0x7f000000) != 0x7f000000));
}

/*
 * is_valid_host_ip_with_subnet
 *	Verify that an IP address is valid.
 */
int hnap_valid_host_ip_with_subnet(char *ipStr, char *maskStr)
{
	struct in_addr ipaddr, ipnetmask; 
	unsigned long ip, mask;

	if(!inet_aton (ipStr, &ipaddr) || !inet_aton (maskStr, &ipnetmask)){
		return FALSE;
	}

	ip=htonl(ipaddr.s_addr);
	mask=htonl(ipnetmask.s_addr);

	if (!hnap_valid_host_ip(ip)) {
		printf("ERROR: The IP address is invalid\n");
		return FALSE;
	}

	if(!hnap_valid_subnet_mask(mask)){
		printf("ERROR: The IP netmask is invalid\n");
		return FALSE;		
	}

	if ((ip & mask) == ip) {
		printf("ERROR: The IP address is a network address\n");
		return FALSE;
	}

	unsigned long wildcard = ~mask;
	if ((ip & wildcard) == wildcard) {
		printf("ERROR: The IP address is a broadcast address\n");
		return FALSE;
	}
	
	return TRUE;
}

int hnap_validate_gateway_with_ip(char *ipStr, char *gatewayStr, char *netmaskStr)
{
	struct in_addr ipaddr, ipgateway, ipnetmask; 
	unsigned long ip, gateway, mask;
	
	if(!inet_aton (ipStr, &ipaddr) || !inet_aton (gatewayStr, &ipgateway) || !inet_aton (netmaskStr, &ipnetmask) ){
		printf("ERROR: inet_aton error.. \n");
		return FALSE;
	}
	
	ip=htonl(ipaddr.s_addr);
	gateway=htonl(ipgateway.s_addr);
	mask=htonl(ipnetmask.s_addr);

	if((ip&mask) != (gateway&mask)){
		printf("ERROR:Gateway unreachable!! \n");
		return FALSE;
	}

	return TRUE;
}

int hnap_validate_ipaddr(char *ipStr)
{

	struct in_addr ipaddr; 
	int ip[4], ret = 0; 
  
   	ret = sscanf (ipStr, "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]); 
  
   	if (ret != 4 || !inet_aton (ipStr, &ipaddr)){ 
		printf("ERROR: hnap_validate_ipaddr fail..\n");
     		return FALSE; 
   	}else{
     		return TRUE; 
   	}
}

//substitution of getNthValue which dosen't destroy the original value
int hnap_getNthValueSafe(int index, char *value, char delimit, char *result, int len)
{
    int i=0, result_len=0;
    char *begin, *end;

    if(!value || !result || !len)
        return -1;

    begin = value;
    end = strchr(begin, delimit);

    while(i<index && end){
        begin = end+1;
        end = strchr(begin, delimit);
        i++;
    }

    //no delimit
    if(!end){
		if(i == index){
			end = begin + strlen(begin);
			result_len = (len-1) < (end-begin) ? (len-1) : (end-begin);
		}else
			return -1;
	}else
		result_len = (len-1) < (end-begin)? (len-1) : (end-begin);

	memcpy(result, begin, result_len );
	*(result+ result_len ) = '\0';

	return 0;
}

int get_signal_noise_percent(int noiseLevel, char *noisePercent)
{
	int noise=0;

	if(noiseLevel>100){
		noise = 0;
	}else if(noiseLevel<=40){
		noise = 100;
	}else{
		noise = (1-(((float)noiseLevel-40)/(100-40)))*100;
	}
	printf("Signal Noise = %d dBm (%d%%)\n", noiseLevel, noise);
	sprintf(noisePercent, "%d", noise);

	return 0;
}

int get_signal_strength_percent(int signalLevel, char *signalStrengthPercent)
{
	int signalStrength=0;

	if(signalLevel>88){
		signalStrength = 0;
	}else if(signalLevel<=22){
		signalStrength = 100;
	}else{
		signalStrength = (1-(((float)signalLevel-22)/(88-22)))*100;
	}

	sprintf(signalStrengthPercent, "%d", signalStrength);

	return 0;
}

/*
 * arguments: ifname - interface name
 *            if_net - a 16-byte buffer to store subnet mask
 * description: fetch subnet mask associated to given interface name
 *              0 = bridge, 1 = gateway, 2 = wirelss isp
 */
int hnap_getIfNetmask(char *ifname, char *if_net)
{
	struct ifreq ifr;
	int skfd = 0;

	if((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		error(E_L, E_LOG, T("getIfNetmask: open socket error"));
		return -1;
	}

	strncpy(ifr.ifr_name, ifname, IF_NAMESIZE);
	if (ioctl(skfd, SIOCGIFNETMASK, &ifr) < 0) {
		//error(E_L, E_LOG, T("getIfNetmask: ioctl SIOCGIFNETMASK error for %s\n"), ifname);
		close(skfd);
		return -1;
	}
	strcpy(if_net, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
	close(skfd);
	return 0;
}

/*
 * arguments: ifname  - interface name
 *            if_addr - a 16-byte buffer to store ip address
 * description: fetch ip address, netmask associated to given interface name
 */
int hnap_getIfIp(char *ifname, char *if_addr)
{
	struct ifreq ifr;
	int skfd = 0;

	if((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		error(E_L, E_LOG, T("getIfIp: open socket error"));
		return -1;
	}

	strncpy(ifr.ifr_name, ifname, IF_NAMESIZE);
	if (ioctl(skfd, SIOCGIFADDR, &ifr) < 0) {
		//error(E_L, E_LOG, T("getIfIp: ioctl SIOCGIFADDR error for %s"), ifname);
		close(skfd); 
		return -1;
	}
	strcpy(if_addr, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));

	close(skfd);
	return 0;
}

/*
 * arguments: ifname  - interface name
 *            if_addr - a 18-byte buffer to store mac address
 * description: fetch mac address according to given interface name
 */
int hnap_getIfMac(char *ifname, char *if_hw)
{
	struct ifreq ifr;
	char *ptr;
	int skfd;

	if((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("ERROR: open socket fail when take %s MAC fail..\n", ifname);
		return -1;
	}

	strncpy(ifr.ifr_name, ifname, IF_NAMESIZE);
	if(ioctl(skfd, SIOCGIFHWADDR, &ifr) < 0) {
		printf("ERROR: IOCTL fail when take %s MAC fail..\n", ifname);
		close(skfd);
		return -1;
	}

	ptr = (char *)&ifr.ifr_addr.sa_data;
	sprintf(if_hw, "%02X:%02X:%02X:%02X:%02X:%02X",
			(ptr[0] & 0377), (ptr[1] & 0377), (ptr[2] & 0377),
			(ptr[3] & 0377), (ptr[4] & 0377), (ptr[5] & 0377));

	close(skfd);
	return 0;
}

int hnap_getClientGateway(char *gateway)
{
	char   buff[256];
	int    nl = 0 ;
	struct in_addr dest;
	struct in_addr gw;
	int    flgs, ref, use, metric, mtu, window, irtt;
	unsigned long int d,g,m;

	FILE *fp = fopen("/proc/net/route", "r");
	if (fp == NULL) {
		strcpy(gateway, "0.0.0.0");
		return -1;
	}

	while (fgets(buff, sizeof(buff), fp) != NULL) {
		if (nl) {
			int ifl = 0;
			while (buff[ifl]!=' ' && buff[ifl]!='\t' && buff[ifl]!='\0')
				ifl++;
			buff[ifl]=0;    /* interface */
			
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

	return 0;
}

/*int hnap_detect_link_by_packets(void)
{
	char	gateway[16]={0};
	FILE *fptr;
	char command[128]={0}, pingResult[128]={0}, checkResult[128]={0};
	char *charPtr;

	if(MT_getClientGateway(gateway) == -1){
		printf("ERROR: Get LAN Gateway fail .. \n");
		return -1;
	}
	if(!strcmp(gateway, "0.0.0.0")){
		printf("The default gateway doesn't be configured\n");
		return -1;
	}
	//printf("Current LAN Gateway = %s\n", gateway);		

	memset(command, 0, 128);
	sprintf(command, "/bin/ping %s", gateway);
	fptr = popen(command, "r");
	if(fptr!=NULL){
		if(fgets(pingResult, 128, fptr)!=NULL){
			charPtr=strchr(pingResult, '\n');
			if(charPtr){
				*charPtr='\0';
			}
			sprintf(checkResult, "%s is alive!", gateway);
			//printf("The result of ping is %s\nThe string for check result is %s\n", pingResult, checkResult);
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

	return 0;
}

int hnap_detect_link_by_dhcp(void)
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
	printf("The lease status for DHCP Client is %s\n", lease_status);
	fclose(fptr);
	
	if(!strncmp(lease_status, "bind", 5)){
		return 1;
	}

	return 0;
}

int hnap_detect_station_link_status(void)
{
	int isLinkUp = 0;
	char paramValue[MT_MAX_PARAM_VALUE_LENGTH]={0}; //for get current setting from configuration

	printf("Detect station link status for WPA-PSK, WEP-OPEN or WEP-AUTO\n");

	hnap_get_config(CONFIG_SYS_LAN_IP_CONFIG_METHOD, paramValue);
	if(!strncmp(paramValue, STATIC_LAN_IP, 2)){
		isLinkUp = hnap_detect_link_by_packets();
	}else{
		isLinkUp = hnap_detect_link_by_dhcp();
	}

	return isLinkUp;
}

int hnap_get_station_link_status(void)
{
	uint8_t tmpBuf[512] = {0};
	uint8_t ifName[16] = {0};
	int link_status=0, nwid, crypt, frag, retry, misc, missed_beacon;
	uint8_t link[16]={0}, level[16]={0}, noise[16]={0};
	int paramCount = 0;

	char paramValue[MT_MAX_PARAM_VALUE_LENGTH]={0}; //for get current setting from configuration
	char securityMode[2]={0}, authMode[2]={0};
	
	FILE *fptr = popen("cat /proc/net/wireless", "r");

	//printf("is_station_wlan_link_up..\n");
	while(fgets(tmpBuf, 512, fptr) != NULL){
		if(strstr(tmpBuf, "Inter-") || strstr(tmpBuf, "face")){
			continue;
		}
		
		paramCount = sscanf(tmpBuf, "%s %d %s %s %s %d %d %d %d %d %d", ifName, &link_status, link, level, noise, &nwid, &crypt, &frag, &retry, &misc, &missed_beacon);
		//printf("paramCount=%d, link_status=%d\n", paramCount, link_status);
		//if(paramCount == 11)
			//printf("=====> link status of wlan is %d\n", link_status);
	}
	
	pclose(fptr);

	//If security is configured to WPA-PSK, WEP-OPEN or WEP-AUTO, 
	//then we need to detect wireless links status by myself.
	//Because Metalink driver will report associated before link real UP when
	//configured to WPA-PSK, WEP-OPEN or WEP-AUTO.
	hnap_get_config(CONFIG_WLAN_SECURITY_MODE, paramValue);
	strcpy(securityMode, paramValue);
	hnap_get_config(CONFIG_WLAN_WEP_AUTHENTICATION_STA, paramValue);
	strcpy(authMode, paramValue);	
	if(link_status==3 && 
		(!strncmp(securityMode, HNAP_SECURITY_WPA_PERSIONAL, 2) || 
			!strncmp(securityMode, HNAP_SECURITY_WPA_ENTERPRISE, 2) ||
			(!strncmp(securityMode, HNAP_SECURITY_WEP, 2) && strcmp(authMode, HNAP_WEP_SHARED)))){
		int detectedLinkStatus=0;
		detectedLinkStatus=hnap_detect_station_link_status();
		if(detectedLinkStatus!=-1){
			return detectedLinkStatus;
		}
		//If detect wireless link my myself fail, then I will use driver report for link status.
	}

	return link_status==3?1:0;
}*/

int hnap_get_station_layer2_link_status(void)
{
	uint8_t tmpBuf[512] = {0};
	uint8_t ifName[16] = {0};
	int link_status=0, nwid, crypt, frag, retry, misc, missed_beacon;
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

	return link_status==3?1:0;
}

int hnap_get_station_wireless_info(char *request, char *result)
{
	FILE *fptr;
	char tmpBuf[256], *tmpPtr, *tmpPtr1, *tmpPtr2;
	int tmpCount=0;
	char ifname[8]={0}, radioband[16]={0}, essid[41]={0};
	int isFail=0;

	memset(tmpBuf, 0, 256);

	if(hnap_get_station_layer2_link_status()!=1){
		printf("ERROR: Wireless doesn't associate with any AP, so can not query wireless information at present\n");
		return -1;
	}

	if(!strcmp(request, "ssid")){
		fptr = popen("iwconfig wlan0 |grep wlan0", "r");
		if(fptr!=NULL){
			if(fgets(tmpBuf, 256, fptr)!=NULL){
				tmpPtr=strstr(tmpBuf, "ESSID:");
				if(tmpPtr){
					tmpPtr1=index(tmpPtr+6, '"');
					if(tmpPtr1){
						tmpPtr1++;
						tmpPtr2=rindex(tmpPtr1, '"');
						if(tmpPtr2)
							*tmpPtr2='\0';
					}else{
						tmpPtr1=tmpPtr+6;
					}
					strcpy(result, tmpPtr1);
				}
			}else{
				isFail=1;
			}
			pclose(fptr);
		}else{
			perror("popen in hnap");
			isFail=1;
		}
	}else if(!strcmp(request, "frequency")){
		fptr = popen("iwpriv wlan0 g_ConInfo", "r");
		if(fptr!=NULL){
			if(fgets(tmpBuf, 256, fptr)!=NULL){
				if(strstr(tmpBuf, "5.2GHz") || strstr(tmpBuf, "2.4GHz")){
					strcpy(result, strstr(tmpBuf, "5.2GHz")?"5":"2");
				}else{
					strcpy(result, "");
					isFail=1;
				}
			}else{
				isFail=1;
			}
			pclose(fptr);
		}else{
			perror("popen in hnap");
			isFail=1;
		}
	}else if(!strcmp(request, "noise")){
		fptr = popen("iwconfig wlan0 |grep Noise", "r");
		if(fptr!=NULL){
			if(fgets(tmpBuf, 256, fptr)!=NULL){
				if(strstr(tmpBuf, "Noise level=")){
					tmpPtr=strstr(tmpBuf, "Noise level=")+12;
					//if(strstr(tmpPtr, "dBm")){
					//	*(strstr(tmpPtr, "dBm"))='\0';
					//}
					tmpCount=1;
					while(*(strstr(tmpPtr, "dBm")-tmpCount)==0x20 || *(strstr(tmpPtr, "dBm")-tmpCount)=='\t'){
						tmpCount++;
					}
					*(strstr(tmpPtr, "dBm")-(tmpCount-1))='\0';
					strcpy(result, tmpPtr);
				}else{
					isFail=1;
				}
			}else{
				isFail=1;
			}			
			pclose(fptr);
		}else{
			perror("popen in hnap");
			isFail=1;
		}
	}else if(!strcmp(request, "signalstrength")){
		fptr = popen("iwconfig wlan0 |grep Signal", "r");
		if(fptr!=NULL){
			if(fgets(tmpBuf, 256, fptr)!=NULL){
				if(strstr(tmpBuf, "Signal level=")){
					tmpPtr=strstr(tmpBuf, "Signal level=")+13;
					//if(strstr(tmpPtr, "dBm")){
					//	*(strstr(tmpPtr, "dBm"))='\0';
					//}
					tmpCount=1;
					while(*(strstr(tmpPtr, "dBm")-tmpCount)==0x20 || *(strstr(tmpPtr, "dBm")-tmpCount)=='\t'){
						tmpCount++;
					}
					*(strstr(tmpPtr, "dBm")-(tmpCount-1))='\0';
					strcpy(result, tmpPtr);
				}else{
					isFail=1;
				}
			}else{
				isFail=1;
			}			
			pclose(fptr);
		}else{
			perror("popen in hnap");
			isFail=1;
		}
	}else if(!strcmp(request, "band")){
		fptr = popen("iwconfig wlan0 |grep wlan0", "r");
		if(fptr!=NULL){
			if(fgets(tmpBuf, 256, fptr)!=NULL){
				if(sscanf(tmpBuf, "%s %s %s", ifname, radioband, essid)==3){
					if(!strcmp(radioband, "802.11bg"))
						strcpy(result, "802.11bg");
					else if(!strcmp(radioband, "802.11bgn"))
						strcpy(result, "802.11bgn");
					else if(!strcmp(radioband, "802.11a"))
						strcpy(result, "802.11a");
					else if(!strcmp(radioband, "802.11an"))
						strcpy(result, "802.11an");
					else if(!strcmp(radioband, "802.11b"))
						strcpy(result, "802.11b");
				}else{
					isFail=1;
				}
			}else{
				isFail=1;
			}			
			pclose(fptr);
		}else{
			perror("popen in hnap");
			isFail=1;
		}
	}else if(!strcmp(request, "txbitrate")){
		fptr = popen("/root/mtlk/etc/mtdump wlan0 constatus", "r");
		if(fptr!=NULL){
			while(1){
				memset(tmpBuf, 0, 256);
				if(fgets(tmpBuf, 256, fptr)==NULL){
					isFail=1;
					break;
				}
				if((tmpPtr=index(tmpBuf, '|'))==0){
					continue;
				}
				if(strstr(tmpBuf, "MAC address") &&
					strstr(tmpBuf, "RSSI dbm") &&
					strstr(tmpBuf, "PHY Rate Mb/s") &&
					strstr(tmpBuf, "Max Data Mb/s") &&
					strstr(tmpBuf, "Type") &&
					strstr(tmpBuf, "Tx packets") &&
					strstr(tmpBuf, "Tx dropped") &&
					strstr(tmpBuf, "Rx packets")){
					continue;
				}
				if((tmpPtr1=index(tmpPtr+1, '|'))==0){
					continue;
				}
				if((tmpPtr2=index(tmpPtr1+1, '|'))==0){
					continue;
				}
				*tmpPtr2='\0';
				sprintf(result, "%u", (unsigned int)(atof(tmpPtr1+1)*1024));
				
				break;
			}
			pclose(fptr);
		}else{
			perror("popen in hnap");
			isFail=1;
		}
		/*fptr = popen("iwconfig wlan0 |grep Bit", "r");
		if(fptr!=NULL){
			if(fgets(tmpBuf, 256, fptr)!=NULL){
				if(strstr(tmpBuf, "Bit Rate:")){
					tmpPtr=strstr(tmpBuf, "Bit Rate:")+9;
					if(strstr(tmpPtr, "Mb/s")){
						tmpCount=1;
						while(*(strstr(tmpPtr, "Mb/s")-tmpCount)==0x20 || *(strstr(tmpPtr, "Mb/s")-tmpCount)=='\t'){
							tmpCount++;
						}
						*(strstr(tmpPtr, "Mb/s")-(tmpCount-1))='\0';
					}
					sprintf(result, "%u", (unsigned int)(atof(tmpPtr)*1024));
				}else{
					isFail=1;
				}
			}else{
				isFail=1;
			}			
			pclose(fptr);
		}else{
			perror("popen in hnap");
			isFail=1;
		}*/	
	}else if(!strcmp(request, "channel")){
		fptr = popen("iwpriv wlan0 g_ConInfo", "r");
		if(fptr!=NULL){
			if(fgets(tmpBuf, 256, fptr)!=NULL){
				hnap_getNthValueSafe(1, tmpBuf, ',', result, sizeof(tmpBuf));
			}else{
				isFail=1;
			}
			pclose(fptr);
		}else{
			isFail=1;
			printf("popen fail for get_wireless_info channel query\n");
		}
	}else if(!strcmp(request, "channelwidth")){
		fptr = popen("iwpriv wlan0 g_ConInfo", "r");
		if(fptr!=NULL){
			if(fgets(tmpBuf, 256, fptr)!=NULL){
				if(strstr(tmpBuf, "40MHz") || strstr(tmpBuf, "20MHz")){
					strcpy(result, strstr(tmpBuf, "40MHz")?"40":"20");
				}else{
					strcpy(result, "");
					isFail=1;
				}
			}else{
				isFail=1;
			}
			pclose(fptr);
		}else{
			perror("popen in hnap");
			isFail=1;
		}
	}else{
		isFail=1;
		printf("Unknow request for get_wireless_info !!!\n");
	}

	return isFail?-1:0;
}

int hnap_prepare_ap_list(char *listBuf)
{
	FILE *fptr;
	char tmpBuf[1024]={0};
	int i=0, j=0;
	char *tagName, *tagValue, *colon, *strEnd;
	char address[18], essid[128];
	int channel, wepOn, is5G, isHT, is40, supportWPS;
	char quality[5], signal[5], noise[5];
	int mode;
	struct ie{
		int wpaType; //1:wpa, 2:wpa2
		int cipherType; //1:tkip, 2:aes, 3:both
		int authType; //1:psk, 2:radius
	}ie1, ie2;
	int firstAP=1;
	int NumOfAp=0;
	char tmpAPInfo[130]={0};
	//char tmpAPInfo2[130]={0};
	char *endPtr;

	fptr = fopen("/tmp/ap_list.catch", "r");
	if(fptr==NULL){
		perror("fopen");
		return -1;
	}

	//count the number of AP
	while(fgets(tmpBuf, 1024, fptr)!=NULL){
		if(strstr(tmpBuf, "Cell")){
			NumOfAp++;
		}
	}
	//printf("The total number of AP = %d\n", NumOfAp);
	sprintf(tmpBuf, "%d;", NumOfAp);
	strcat(listBuf, tmpBuf);
	//reset the read/write position
	rewind(fptr);

	memset(address, 0, 18);
	memset(essid, 0, 128);
	memset(&ie1, 0, sizeof(struct ie));
	memset(&ie2, 0, sizeof(struct ie));
	channel=wepOn=is5G=isHT=is40=supportWPS=mode=0;
	memset(tmpAPInfo, 0, 130);
	//memset(tmpAPInfo2, 0, 130);

	//Skip first line, "wlan0     Scan completed :"
	fgets(tmpBuf, 1024, fptr);
	memset(tmpBuf, 0, 1024);
	
	//start to get information of APs
	while(fgets(tmpBuf, 1024, fptr)!=NULL){
		//printf(">>%s", tmpBuf);

		if(strstr(tmpBuf, "Cell")){
			if(!firstAP){
				//printf("\n===== AP-%d =====\n", i);
				//printf("Address=%s\n", address);
				//printf("ESSID=%s\n", essid);
				//printf("Channel=%d\n", channel);
				//printf("Radio Band=%s\n", is5G?"5G":"2.4G");
				//printf("HT=%s\n", isHT?"Enabled":"Disabled");
				//printf("Band Width=%s\n", is40?"40 MHz":"20 MHz");
				if(wepOn){
					//printf("ie1.wpaType=%d, ie2.wpaType=%d\n", ie1.wpaType, ie2.wpaType);
					if(ie1.wpaType || ie2.wpaType){
						if(ie1.wpaType){
							if(ie1.authType==1){
								if(ie1.wpaType==1)
									;//printf("Security=WPAPSK, Cipher=%s\n", (ie1.cipherType==3)?"TKIP+AES":((ie1.cipherType==2)?"AES":"TKIP"));
								else
									;//printf("Security=WPA2PSK, Cipher=%s\n", (ie1.cipherType==3)?"TKIP+AES":((ie1.cipherType==2)?"AES":"TKIP"));
							}else{
								if(ie1.wpaType==1)
									;//printf("Security=WPA, Cipher=%s\n", (ie1.cipherType==3)?"TKIP+AES":((ie1.cipherType==2)?"AES":"TKIP"));
								else
									;//printf("Security=WPA2, Cipher=%s\n", (ie1.cipherType==3)?"TKIP+AES":((ie1.cipherType==2)?"AES":"TKIP"));
							}
						}
						if(ie2.wpaType){
							if(ie2.authType==1){
								if(ie2.wpaType==1)
									;//printf("Security=WPAPSK, Cipher=%s\n", (ie2.cipherType==3)?"TKIP+AES":((ie2.cipherType==2)?"AES":"TKIP"));
								else
									;//printf("Security=WPA2PSK, Cipher=%s\n", (ie2.cipherType==3)?"TKIP+AES":((ie2.cipherType==2)?"AES":"TKIP"));
							}else{
								if(ie2.wpaType==1)
									;//printf("Security=WPA, Cipher=%s\n", (ie2.cipherType==3)?"TKIP+AES":((ie2.cipherType==2)?"AES":"TKIP"));
								else
									;//printf("Security=WPA2, Cipher=%s\n", (ie2.cipherType==3)?"TKIP+AES":((ie2.cipherType==2)?"AES":"TKIP"));
							}
						}
					}else{
						//printf("Security=WEP\n");
					}
				}else{
					//printf("Security=NONE\n");
					//address,ssid,mode,is5G,channel,enable HT,band width,encryption key,ie1,ie1 pairwise cipher,ie1 authentication suites,ie2,ie2 pairwise cipher,ie2 authentication suites, quality, signal, noise;
				}
				sprintf(tmpAPInfo, "%s\r%s\r%d\r%d\r%d\r%d\r%d\r%d\r%d\r%d\r%d\r%d\r%d\r%d\r%s\r%s\r%s\r%d\t", address, essid, mode, is5G, channel, isHT, is40, wepOn, ie1.wpaType, ie1.cipherType, ie1.authType, ie2.wpaType, ie2.cipherType, ie2.authType, quality, signal, noise, supportWPS);
				//sprintf(tmpAPInfo2, "%s %s %d %d %d %d %d %d %d %d %d %d %d %d %s %s %s %d", address, essid, mode, is5G, channel, isHT, is40, wepOn, ie1.wpaType, ie1.cipherType, ie1.authType, ie2.wpaType, ie2.cipherType, ie2.authType, quality, signal, noise, supportWPS);
				//printf("tmpAPInfo %d =%s\n", i, tmpAPInfo2);
				//printf("essid=%s(%d)\n", essid, strlen(essid));
				if(!strcmp(essid, "\"\"")){
					printf("Skip NULL SSID\n");
					NumOfAp--;
				}else if(strlen(essid)>2 && strcmp(essid, "\"\"")){ //Skip hidden SSID
					strcat(listBuf, tmpAPInfo);
				}
				i++;
				memset(address, 0, 18);
				memset(essid, 0, 128);
				memset(&ie1, 0, sizeof(struct ie));
				memset(&ie2, 0, sizeof(struct ie));
				channel=wepOn=is5G=isHT=is40=supportWPS=mode=0;
				memset(tmpAPInfo, 0, 130);	
				//memset(tmpAPInfo2, 0, 130);	
			}else{
				i++;
				firstAP=0;
			}
			
			memset(address, 0, 18);
			memset(essid, 0, 128);
			memset(&ie1, 0, sizeof(struct ie));
			memset(&ie2, 0, sizeof(struct ie));
			channel=wepOn=is5G=isHT=is40=supportWPS=mode=0;
			memset(tmpAPInfo, 0, 130);
			//memset(tmpAPInfo2, 0, 130);

			if((tagName = strstr(tmpBuf, "Address")) != NULL){
				colon = (char *)index(tagName, ':');
				j=1;
				while(*(colon+j) == 0x20)
					j++;
				tagValue = colon+j;
				memcpy(address, tagValue, 17);
			}
		}else if((colon = (char *)index(tmpBuf, ':')) != NULL){
			//get tagName
			j=0;
			while(*(tmpBuf+j) == 0x20)
				j++;
			tagName = tmpBuf+j;
			*colon = '\0';

			//get tagValue
			j=1;
			while(*(colon+j) == 0x20)
				j++;
			tagValue = colon+j;
			strEnd=(char *)index(tagValue, 0x0a);
			*strEnd = '\0';

			//printf("%s=%s\n", tagName, tagValue);

			if(!strcmp(tagName, "ESSID")){
				strcpy(essid, tagValue);
			}else if(!strcmp(tagName, "Channel")){
				channel = atoi(tagValue);
			}else if(!strcmp(tagName, "Encryption key")){
				if(!strcmp(tagValue, "on")){
					wepOn=1;
				}
			}else if(!strcmp(tagName, "Extra")){
				//printf("Extra value = %s\n", tagValue);//Ricky Trace
				if(!strcmp(tagValue, "5.2 band")){
					is5G = 1;
				}else if(!strcmp(tagValue, "HT")){
					isHT=1;
				}else if(!strcmp(tagValue, "40 MHz")){
					is40=1;
				}else if(!strcmp(tagValue, "WPS") && strcmp(tagValue, "not WPS")){
					supportWPS=1;
				}
			}else if(!strcmp(tagName, "IE")){
				if(strstr(tagValue, "WPA2")){
					if(ie1.wpaType==0){
						ie1.wpaType=2;
					}else if(ie2.wpaType==0){
						ie2.wpaType=2;
					}
				}else{
					if(ie1.wpaType==0){
						ie1.wpaType=1;
					}else if(ie2.wpaType==0){
						ie2.wpaType=1;
					}
				}
			}else if(strstr(tagName, "Pairwise Ciphers")){
				//printf("value for Pairwis Ciphers = %s\n", tagValue);
				if(ie1.wpaType!=0 && ie1.cipherType==0){
					if(strstr(tagValue, "TKIP") && strstr(tagValue, "CCMP")){
						ie1.cipherType=3;
					}else if(strstr(tagValue, "TKIP")){
						ie1.cipherType=1;
					}else if(strstr(tagValue, "CCMP")){
						ie1.cipherType=2;
					}else{
						ie1.cipherType=3;
					}
				}else if(ie2.wpaType!=0 && ie2.cipherType==0){
					if(strstr(tagValue, "TKIP") && strstr(tagValue, "CCMP")){
						ie2.cipherType=3;
					}else if(strstr(tagValue, "TKIP")){
						ie2.cipherType=1;
					}else if(strstr(tagValue, "CCMP")){
						ie2.cipherType=2;
					}else{
						ie2.cipherType=3;
					}
				}
			}else if(strstr(tagName, "Authentication Suites")){
				if(ie1.wpaType!=0 && ie1.authType==0){
					if(!strncmp(tagValue, "PSK", 3)){
						ie1.authType=1;
					}else{
						ie1.authType=2;
					}
				}else if(ie2.wpaType!=0 && ie2.authType==0){
					if(!strncmp(tagValue, "PSK", 3)){
						ie2.authType=1;
					}else{
						ie2.authType=2;
					}
				}
			}else if(strstr(tagName, "Mode")){
				if(!strncmp(tagValue, "Auto", 4)){
					mode=1;
				}
			}
		}else if(strstr(tmpBuf, "Quality")){
			memset(quality, 0, 5);
			memset(signal, 0, 5);
			memset(noise, 0, 5);
			tagName=strstr(tmpBuf, "Quality");
			tagValue=(char *)index(tagName, '=')+1;
			strEnd=(char *)index(tagValue, 0x20);
			memcpy(quality, tagValue, (strEnd-tagValue));

			tagName=strstr(strEnd, "Signal level");
			tagValue=(char *)index(tagName, '=')+1;
			strEnd=(char *)index(tagValue, 0x20);
			memcpy(signal, tagValue, (strEnd-tagValue));

			tagName=strstr(strEnd, "Noise level");
			tagValue=(char *)index(tagName, '=')+1;
			strEnd=(char *)index(tagValue, 0x20);
			memcpy(noise, tagValue, (strEnd-tagValue));			
		}
		
		memset(tmpBuf, 0, 1024);
	}

	if(i!=0){
		//printf("\n===== AP-%d =====\n", i);
		//printf("Address=%s\n", address);
		//printf("ESSID=%s\n", essid);
		//printf("Channel=%d\n", channel);
		//printf("Radio Band=%s\n", is5G?"5G":"2.4G");
		//printf("HT=%s\n", isHT?"Enabled":"Disabled");
		//printf("Band Width=%s\n", is40?"40 MHz":"20 MHz");
		if(wepOn){
			//printf("ie1.wpaType=%d, ie2.wpaType=%d\n", ie1.wpaType, ie2.wpaType);
			if(ie1.wpaType || ie2.wpaType){
				if(ie1.wpaType){
					if(ie1.authType==1){
						if(ie1.wpaType==1)
							;//printf("Security=WPAPSK, Cipher=%s\n", (ie1.cipherType==3)?"TKIP+AES":((ie1.cipherType==2)?"AES":"TKIP"));
						else
							;//printf("Security=WPA2PSK, Cipher=%s\n", (ie1.cipherType==3)?"TKIP+AES":((ie1.cipherType==2)?"AES":"TKIP"));
					}else{
						if(ie1.wpaType==1)
							;//printf("Security=WPA, Cipher=%s\n", (ie1.cipherType==3)?"TKIP+AES":((ie1.cipherType==2)?"AES":"TKIP"));
						else
							;//printf("Security=WPA2, Cipher=%s\n", (ie1.cipherType==3)?"TKIP+AES":((ie1.cipherType==2)?"AES":"TKIP"));
					}
				}
				if(ie2.wpaType){
					if(ie2.authType==1){
						if(ie2.wpaType==1)
							;//printf("Security=WPAPSK, Cipher=%s\n", (ie2.cipherType==3)?"TKIP+AES":((ie2.cipherType==2)?"AES":"TKIP"));
						else
							;//printf("Security=WPA2PSK, Cipher=%s\n", (ie2.cipherType==3)?"TKIP+AES":((ie2.cipherType==2)?"AES":"TKIP"));
					}else{
						if(ie2.wpaType==1)
							;//printf("Security=WPA, Cipher=%s\n", (ie2.cipherType==3)?"TKIP+AES":((ie2.cipherType==2)?"AES":"TKIP"));
						else
							;//printf("Security=WPA2, Cipher=%s\n", (ie2.cipherType==3)?"TKIP+AES":((ie2.cipherType==2)?"AES":"TKIP"));
					}
				}
			}else{
				//printf("Security=WEP\n");
			}
		}else{
			//printf("Security=NONE\n");
		}

		//for test
		/*if(i<200){
			int v1;
			int v2=200-i;
			char *tmpSSID[33];
			for(v1=1; v1<=v2; v1++){	
				sprintf(tmpSSID, "\"FakeSSID_%d\"", v1);
				memset(tmpAPInfo, 0, 130);
				sprintf(tmpAPInfo, "%s,%s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%s,%s,%s,%d\t", address, tmpSSID, mode, is5G, channel, isHT, is40, wepOn, ie1.wpaType, ie1.cipherType, ie1.authType, ie2.wpaType, ie2.cipherType, ie2.authType, quality, signal, noise, supportWPS);
				printf("tmpAPInfo %d =%s\n", i, tmpAPInfo);
				strcat(listBuf, tmpAPInfo);
				i++;
			}
		}*/
		//for test
		
		sprintf(tmpAPInfo, "%s\r%s\r%d\r%d\r%d\r%d\r%d\r%d\r%d\r%d\r%d\r%d\r%d\r%d\r%s\r%s\r%s\r%d", address, essid, mode, is5G, channel, isHT, is40, wepOn, ie1.wpaType, ie1.cipherType, ie1.authType, ie2.wpaType, ie2.cipherType, ie2.authType, quality, signal, noise, supportWPS);
		//sprintf(tmpAPInfo2, "Final AP Info : %s %s %d %d %d %d %d %d %d %d %d %d %d %d %s %s %s %d", address, essid, mode, is5G, channel, isHT, is40, wepOn, ie1.wpaType, ie1.cipherType, ie1.authType, ie2.wpaType, ie2.cipherType, ie2.authType, quality, signal, noise, supportWPS);
		//printf("tmpAPInfo %d =%s\n", i, tmpAPInfo2);
		//printf("essid=%s(%d)\n", essid, strlen(essid));
		if(!strcmp(essid, "\"\"")){
			printf("Skip NULL SSID at last AP information\n");
			NumOfAp--;
			endPtr = rindex(listBuf, '\t');
			*endPtr = 0x00;
		}else if(strlen(essid)>2 && strcmp(essid, "\"\"")){ //Skip hidden SSID
			strcat(listBuf, tmpAPInfo);		
		}
	}
	//printf("Completed!!!!\n");
	fclose(fptr);

	//printf("NumOfAp = %d\n", NumOfAp);

	return NumOfAp;
}

int hnap_doSystem(char *format, ...)
{
	va_list arg;
	char *cmd;
	int rv=0;
	
	va_start(arg, format);
	if (fmtValloc(&cmd, WEBS_BUFSIZE, format, arg) >= WEBS_BUFSIZE) {
		trace(0, T("doSystem: lost data, buffer overflow\n"));
	}
	va_end(arg);
	
	if (cmd) {
		trace(0, T("%s\n"), cmd);
		rv = system(cmd);
		bfree(B_L, cmd);
	}
	return rv;
}

//hnap_get_config()
//This function is a interface for hnap get configuration from Metalink configuration
int hnap_get_config(char *paramName, char *paramValue)
{
	char if_mac[18]={0};

	if(!strcmp(paramName, CONFIG_WLAN_MAC)){
		hnap_getIfMac("wlan0", if_mac);
		printf("wlan0 MAC address is %s\n", if_mac);
		strcpy(paramValue, if_mac);
		return 0;
	}
	
	//printf("start hnap_get_config: paramName=%s\n", paramName);//Ricky Trace
	if(MT_Get_Param(paramName, paramValue, MT_MAX_PARAM_VALUE_LENGTH-1)==0){
		return 0;
	}
	//printf("end hnap_get_config: paramValue=%s\n", paramValue);//Ricky Trace
	return 1;
}

void hnap_reload_driver(void)
{
	hnap_doSystem("cd /root/mtlk/etc ; /root/mtlk/etc/reload_mtlk_driver.sh");
	hnap_doSystem("killall hostapd");
	hnap_doSystem("/root/mtlk/etc/hostapd -d /mnt/jffs2/hostapd0.conf >/dev/null 2>/dev/null &");
}

void hnap_active_lan_basic(char *lanIP, char *lanNetmask)
{
	hnap_doSystem("/sbin/ifconfig %s %s netmask %s", LAN_IF_NAME, lanIP, lanNetmask);
	hnap_doSystem("/root/mtlk/web/post_apply.tcl");
}

void hnap_active_wlan_basic(int isRadioOn)
{
	char command[256]={0};
	char paramValue[MT_MAX_PARAM_VALUE_LENGTH]={0};
	
	hnap_doSystem("/sbin/ifconfig wlan0 down");
	
	//doSystem("/root/mtlk/etc/mtpriv wlan0 FrequencyBand %s", FrequencyBand);
	
	hnap_doSystem("/root/mtlk/etc/mtpriv wlan0 FrequencyBand 1");
	
	hnap_get_config(CONFIG_WLAN_CHANNEL_WIDTH, paramValue);
	hnap_doSystem("/root/mtlk/etc/mtpriv wlan0 IsHTEnabled %d", atoi(paramValue));
	
	hnap_doSystem("/sbin/ifconfig wlan0 up");
	
	hnap_doSystem("/sbin/ifconfig wlan0 down");

	hnap_get_config(CONFIG_WLAN_CHANNEL, paramValue);
	if (atoi(paramValue) == 0) {
		hnap_doSystem("/sbin/iwconfig wlan0 channel auto");
	}else{
		hnap_doSystem("/sbin/iwconfig wlan0 channel %s", paramValue);
	}

	hnap_get_config(CONFIG_WLAN_CHANNEL_UPPERLOWER, paramValue);
	hnap_doSystem("/root/mtlk/etc/mtpriv wlan0 UpperLowerChannelBonding %s", paramValue);

	hnap_doSystem("/root/mtlk/web/post_apply.tcl");

	//if(isRadioOn)
		hnap_doSystem("/sbin/ifconfig wlan0 up");
}


void hnap_active_wlan_security(void)
{
	hnap_doSystem("killall hostapd");
	hnap_doSystem("cd /root/mtlk/web/ ; /root/mtlk/web/init_security.tcl");
	hnap_doSystem("/root/mtlk/etc/hostapd -d /mnt/jffs2/hostapd0.conf >/dev/null 2>/dev/null &");
}

void hnap_active_station_wlan_security(void)
{
	char paramValue[MT_MAX_PARAM_VALUE_LENGTH]={0}; //for get current setting from configuration

	//FILE *fptr;
	//char tmpBuf1[256]={0}, tmpBuf2[256]={0}, cloneMac[18]={0};
	//char *tmpPtr;
	//int bridgeMode=0;
	//int result=0;
	//struct stat statBuf;

	//hnap_doSystem("cd /root/mtlk/web/");
	//hnap_doSystem("/root/mtlk/web/init_security.tcl");
	hnap_get_config(CONFIG_WLAN_WILDCARD_SSID, paramValue);
	hnap_doSystem("ifconfig wlan0 down");
	hnap_doSystem("ifconfig wlan0 up");	
	hnap_doSystem("/root/mtlk/etc/mtpriv wlan0 Wildcard_ESSID %s", paramValue);
	hnap_doSystem("cd /root/mtlk/web/; /root/mtlk/web/init_security.tcl reactivate");

	//Metalink had fixed abnormal wireless link status even detected and it will renew IP properly
	//so we don't need restart udhcpc to for renew IP - Ricky Cao on Nov. 21 2008
	/*memset(paramValue, 0, MT_MAX_PARAM_VALUE_LENGTH);
	hnap_get_config(CONFIG_SYS_LAN_IP_CONFIG_METHOD, paramValue);
	if(!strncmp(paramValue, DHCP_LAN_IP, 2)){
		sleep(2);
		printf("Restart DHCP Client for renew IP from DHCP server!\n");
		hnap_doSystem("/etc/udhcpc/dhcp.tcl kill");
		sleep(1);
		hnap_doSystem("/etc/udhcpc/dhcp.tcl startup");
	}*/

	/*fptr=popen("ps | grep wpa_supplicant", "r");
	while(fgets(tmpBuf1, 256, fptr)!=NULL){
		if(!strstr(tmpBuf1, "grep") && strstr(tmpBuf1, "wpa_supplicant")){
			hnap_doSystem("killall -HUP wpa_supplicant");
			pclose(fptr);
			return;
		}
	}
	pclose(fptr);

	fptr=popen("cat /mnt/jffs2/sys.conf | grep BridgeMode", "r");
	while(fgets(tmpBuf1, 256, fptr)!=NULL){
		if(strstr(tmpBuf1, "BridgeMode")){
			tmpPtr=strchr(tmpBuf1, '=')+1;
			if(tmpPtr!=0){
				bridgeMode=atoi(tmpPtr);
				printf("BridgeMode=%d\n", bridgeMode);
			}
		}
	}
	pclose(fptr);
	
	if(bridgeMode!=0 && bridgeMode==3){
		result = stat("/tmp/mac_cloning.addr",&statBuf);
		if(result != -1){
			fptr = popen("cat /tmp/mac_cloning.addr", "r");
			if(fptr!=NULL){
				memset(tmpBuf1, 0, 256);
				memset(tmpBuf2, 0, 256);
				fgets(tmpBuf1, 256, fptr);
				pclose(fptr);
				memcpy(cloneMac, tmpBuf1, 17);
				printf("MAC Address for MAC Cloning=%s\n", cloneMac);
				//sprintf(tmpBuf2, "/root/mtlk/etc/wpa_supplicant -Dwext -bbr0 -iwlan0 -c /mnt/jffs2/wpa_supplicant0.conf -p maclone=%s > /dev/null 2>/dev/null &", cloneMac);
				//printf("command=%s\n", tmpBuf2);
				//system(tmpBuf2);
				hnap_doSystem("/root/mtlk/etc/wpa_supplicant -Dwext -bbr0 -iwlan0 -c /mnt/jffs2/wpa_supplicant0.conf -p maclone=%s > /dev/null 2>/dev/null &", cloneMac);
			}else{
				hnap_doSystem("/root/mtlk/etc/wpa_supplicant -Dwext -bbr0 -iwlan0 -c /mnt/jffs2/wpa_supplicant0.conf > /dev/null 2>/dev/null &");
			}
		}
	}else{
		hnap_doSystem("/root/mtlk/etc/wpa_supplicant -Dwext -bbr0 -iwlan0 -c /mnt/jffs2/wpa_supplicant0.conf > /dev/null 2>/dev/null &");
	}*/
}

void hnap_active_mac_filter(void)
{
	hnap_reload_driver();
}

int device_set_password(char *password_new)
{
	char_t *argp[] = {MTSECURITY_USER_PASSWD_VAR_NAME};

	MT_Set_Param(MTSECURITY_USER_PASSWD_VAR_NAME, password_new, MT_SET_FIRST_VALUE);
	MT_WriteConfFiles(1, argp);

	return 1;
}

int device_set_string_value(char *name, char* value)
{
	//NEED FIXED
	if(!strcmp(name, CONFIG_SYS_ADMINPASSWORD)){
		return device_set_password(value);
	}else if(!strcmp(name, CONFIG_WLAN_CHANNEL_WIDTH)){
		if(!strcmp(value, "40"))
			MT_Set_Param(name, "1", MT_SET_FIRST_VALUE);
		else
			MT_Set_Param(name, "0", MT_SET_FIRST_VALUE);
	}else if(!strcmp(name, CONFIG_WLAN_CHANNEL_UPPERLOWER)){
		if(!strcmp(value, "upper")){
			MT_Set_Param(name, "0", MT_SET_FIRST_VALUE);
		}else{
			MT_Set_Param(name, "1", MT_SET_FIRST_VALUE);
		}
	}else{
		MT_Set_Param(name, value, MT_SET_FIRST_VALUE);
	}
	
	return 1;
}

int hnap_commit_changes(int argc, char **argv, char *confType)
{
	char network_type[4];
	MT_Get_Param("network_type", network_type, sizeof(network_type));

	MT_WriteConfFiles(argc, argv);

	if(!strcmp(confType, COMMIT_SYS)){
		system("/bin/cp /mnt/jffs2/sys.conf /tmp/sys.conf");
	}else if(!strcmp(confType, COMMIT_WLAN0)){
		system("/bin/cp /mnt/jffs2/wlan0.conf /tmp/wlan0.conf");
	}

	system("/bin/config_umount.sh");
	system("/bin/config_mount.sh");

	return 1; 
}

int system_reset(system_restart_types reset_type)
{
	switch(reset_type){
       	case srt_restart:
            		return kill(1, SIGHUP);
        	case srt_reboot:
			//printf("Device Rebooting..");
            		//return system("/sbin/reboot");
           		return kill(1, SIGTERM);
        	case srt_servicerestart:
            		return kill(1, SIGUSR1);
        	default:
            		return -1;
    	}

	return 1;
}

unsigned int system_sleep(unsigned int seconds)
{
    return sleep(seconds);
}

//NEED BE FIXED
int device_is_ready()
{
	//This function need be fixed
	//I always reply the device status is ready for test - Ricky Cao
	
    	return 1;
}

//The header of firmwre image
typedef struct {
	unsigned int magic;
	unsigned int pcksum2;
	unsigned int headerID;
	unsigned int deviceID;
	unsigned int filler[3];
	unsigned int cksum;
} MT_IMAGE_HEADER;

// Reverse 32-bit values for big-endian hosts
void hnap_reverse_ulong(unsigned int* pVal)
{
	*pVal = (((*pVal) & 0xFF) << 24) |
		(((*pVal) & 0xFF00) << 8) |
		(((*pVal) & 0xFF0000) >> 8) |
		((*pVal) >> 24);
}

//determine whether device is a little endian system
int hnap_IsLittleEndian(void)
{
	int testInt = 0x12345678;
	unsigned char* pTestByte = (unsigned char*)&testInt;
	return (int)(*pTestByte == 0x78);
}

//Burn the firmware image to flash ,then ,take it out from flash and verify it again..
//Upgrade successfully: return 0, Upgrade fail: return -1
#define HNAP_FILE_SINGLE_WRITE 65536

int hnap_burn_and_verify_image(char_t *data, int dataSize)
{
	int locWrite = 0;
	int locVerify = 0;
	int numLeft = 0;
	int numWrite = 0;
	int numRead = 0;
	int retryCount = 0;
	int verifiedOk = 0;
	//int i = 0;
	int chunkSize = 0;
	FILE* fp = NULL;
	MT_IMAGE_HEADER imageHeader;
	int isLittleEndian = hnap_IsLittleEndian();
	int dummy_crc[] = {0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF};
	int precentComplete=0;

	printf("Flash update in progress - do not interrupt this process !\n");

	// Make sure to override the ending checksum in the current image header
	// 1. Read header
	// 2. Extract the location of the last CRC
	// 3. Write FF instead of the current CRC
	// 4. Close the file.
	printf("1. Make sure to override the ending checksum in the current image header...\n");
	if ((fp = fopen(DEFAULT_FLASH_FILE, "rb")) != NULL){
		numRead = fread(&imageHeader,1,sizeof(MT_IMAGE_HEADER),fp);
		fclose(fp);
		fp = NULL;

		if (numRead == sizeof(MT_IMAGE_HEADER)){
			if (!isLittleEndian){
				hnap_reverse_ulong(&imageHeader.magic);
				hnap_reverse_ulong(&imageHeader.headerID);
				hnap_reverse_ulong(&imageHeader.deviceID);
				hnap_reverse_ulong(&imageHeader.cksum);
				hnap_reverse_ulong(&imageHeader.pcksum2);
			}

			printf ("imageheader.pcksum2 = %d\n",imageHeader.pcksum2);

			if ((fp = fopen(DEFAULT_FLASH_FILE, "ab")) != NULL){
				fseek(fp,imageHeader.pcksum2,SEEK_SET);
				fwrite(dummy_crc,1,4*sizeof(int),fp);
				fclose(fp);
				fp = NULL;
			}else{
				printf("Error : Could not open flash image for CRC erase\n");
				return -1;
			}
			
		}else{
			printf("Error : Could not read header (read %d bytes out of %d)\n",numRead,sizeof(MT_IMAGE_HEADER));
			return -1;
		}
	}else{
		printf("Error : Could not open flash image for reading header.\n");
		return -1;
	}
	printf("Completed..\n");
	precentComplete = 100;

	// Burning
	printf("2. Start burning..\n");
	precentComplete = 0;

    	locWrite = 0;
    	numLeft = dataSize;

	system("echo 2 > /dev/gpio2"); //Blink LED to indicate firmware upgrade is progressing

	for (retryCount = 0; retryCount<MAX_RETRY_BURN_TIMES && !verifiedOk; retryCount++){
		if (retryCount>0){
			printf("Flash update in progress - do not interrupt this process !(retry %d)\n",retryCount+1);
		}
		
		if ((fp = fopen(DEFAULT_FLASH_FILE, "w+b")) != NULL){
			while (numLeft > 0) {
				int size = numLeft > HNAP_FILE_SINGLE_WRITE ? HNAP_FILE_SINGLE_WRITE : numLeft;
				//printf("%d\n",numLeft); //Ricky Trace
				//printf("Start write Image data..\n");
				numWrite = fwrite(&(data[locWrite]), sizeof(char_t), size, fp);
				//printf("End write Image data..\n");
                		//fflush(fp);
				if (!numWrite) {
					printf("Error : File could not be written\n ferror=%d locWrite=%d numLeft=%d numWrite=%d Size=%d bytes", ferror(fp), locWrite, numLeft, numWrite, dataSize);
					precentComplete = -1;
					printf("Error numWrite=0");
					break;
				}
        
				locWrite += numWrite;
				numLeft -= numWrite;
				precentComplete = locWrite*100/dataSize;
				printf("%d %% completed ..\n", precentComplete);
				//////////////////////////////////////////////////////////////////////////
				//////////////////////////////////////////////////////////////////////////
				/*chunkSize += numWrite;
				if (chunkSize >= (HNAP_FILE_SINGLE_WRITE)) 
				{
					chunkSize = 0;
					printf("chunkSize larger then FILE_SINGLE_WRITE(%d), close file descriptor\n", HNAP_FILE_SINGLE_WRITE);
					// close the file to allow it to flush all data.
					if (fclose(fp) != 0) {
						printf("Error : File close failed  (reopen for flushing).\n locWrite=%d numLeft=%d numWrite=%d Size=%d bytes\n", locWrite, numLeft, numWrite, dataSize);             
						precentComplete = -1;
						printf("Error can not close file  (reopen for flushing)\n");
						break;
					}

					fp = NULL;
					printf("open file descriptor\n");
					// reopen the file and seek to the last location to continue writing the next chunk.
					if ((fp = fopen(DEFAULT_FLASH_FILE, "w+b")) == NULL){
						printf("Error : Flash File could not be Opened (reopen for flushing)\n");
						precentComplete = -1;
						printf("can not open file  (reopen for flushing)\n");
						break;
					}
					printf("Seek writed position\n");
					fseek(fp,locWrite,SEEK_SET);
					printf("Completed process to chunkSize larger then FILE_SINGLE_WRITE(%d)\n", HNAP_FILE_SINGLE_WRITE);
				}*/
				//////////////////////////////////////////////////////////////////////////
				//////////////////////////////////////////////////////////////////////////
			}
			if (numLeft == 0) {
				if (fclose(fp) != 0) {
					printf("Error : File close failed.\n locWrite=%d numLeft=%d numWrite=%d Size=%d bytes\n", locWrite, numLeft, numWrite, dataSize);             
					precentComplete = -1;
					printf("Error can not close file\n");
				}
			} else {
				printf("Error : File upload incomplete - numLeft=%d locWrite=%d Size=%d bytes\n", numLeft, locWrite,  dataSize);
				precentComplete = -1;
				printf("Error numLeft=0\n");
			}
		}else{
			printf("Error : Flash File could not be Opened\n");
			precentComplete = -1;
			printf("can not open file\n");
		}

		if (precentComplete==100){
			/*char_t verBuff[1024];

			// Verifying
			printf("Image burning completed!!\n");
			printf("Verifying burned image ..\n");
			precentComplete = 0;
			numLeft = dataSize;
			locVerify = 0;
			printf("Trying to open file %s for verifying..\n",DEFAULT_FLASH_FILE);
			if ((fp = fopen(DEFAULT_FLASH_FILE, "rb")) != NULL){
				verifiedOk = 1;
				while (numLeft > 0 && verifiedOk == 1) {
					//printf("Reading file\n"); //Ricky Trace
					numRead = fread(verBuff,sizeof(char_t),1024,fp);
                   			 // make sure you don't exceed buffer , because this isn't really a file ...
                    			if (numRead > numLeft) 
                         			numRead = numLeft;
					//printf("%d bytes be read\n", numRead); //Ricky Trace
                    			if (numRead>0){
						for (i = 0; i < numRead;i++){
							if (verBuff[i]!=data[locVerify]) {
								verifiedOk = 0;
								printf ("ERROR FOUND !\n");
								break;
							}
							locVerify++;
							precentComplete = locVerify*100/dataSize;
						}
						numLeft-=numRead;
						//printf("%d%%completed, left %d bytes to verifying\n", precentComplete, numLeft); //Ricky Trace
					}
				}
				printf ("Closing\n");
				fclose(fp);
			}else{
				printf("ERROR:Fail to load image for verifying ..\n");
			}*/
			verifiedOk = 1;
			if (verifiedOk == 1){
				precentComplete = 100;
				break;
			}
		}
	}

	if (verifiedOk != 1){
		precentComplete = -1;
		printf("Error : Error verifying image at location %d\n ferror=%d....\n", ferror(fp), locVerify);
		printf("Firmware upgrade fail.. Orz..\n");
		return -1;
	}else{
		precentComplete = 100;
		printf("Firmware upgrade full successful.. \\^o^/ \\^o^/ \\^o^/\n");
	}
	
	// Need to reboot the system now !
	return 0;
}

// Validate the CRC and HW information in the image
// valid: return 0, invalid: return -1
int hnap_validate_Image(const char_t* imageBuf, int imageSize)
{
	unsigned int crcFromFooter, imageSizeFromFile;
	MT_IMAGE_HEADER imageHeader;
	uLong crc;
	char tmpOutputFile[MT_MAX_PATH_LENGTH];
	char mtdBlockCmnd[MT_MAX_PATH_LENGTH];
	FILE* tmpFile;
	int mtdDeviceID = 0;
	int isLittleEndian = hnap_IsLittleEndian();

	// Validate that header is correct
	if (imageSize < sizeof(MT_IMAGE_HEADER) + 8){
		printf("Error: This is not an image file, since \"too small\"\n");
		return -1;
	}

	memcpy(&imageHeader, imageBuf, sizeof(MT_IMAGE_HEADER));
	if (!isLittleEndian){
		hnap_reverse_ulong(&imageHeader.magic);
		hnap_reverse_ulong(&imageHeader.headerID);
		hnap_reverse_ulong(&imageHeader.deviceID);
		hnap_reverse_ulong(&imageHeader.cksum);
		hnap_reverse_ulong(&imageHeader.pcksum2);
	}

	if (imageHeader.magic != 0xEA000006){
		printf("Error: This is not an image file since \"wrong magic code\"\n");
		return -1;
	}

	if (imageHeader.pcksum2 + 16 != imageSize){ // 12 empty bytes, then CRC
		printf("Error: Wrong image size in header \"%d\"\n", imageHeader.pcksum2);
		return -1;
	}

	if (imageHeader.headerID != 1){
		printf("Error: Unsupported image, since header version is \"%d\"\n", imageHeader.headerID);
		return -1;
	}

	// Check if the device ID is the same as in mtdblock3:
	sprintf(tmpOutputFile, "%s/web_device_id.tmp", MT_WebTmpDir);
	sprintf(mtdBlockCmnd, "get_env_param device_id >%s", tmpOutputFile);
	system(mtdBlockCmnd);
	tmpFile = fopen(tmpOutputFile, "rt");
	if (tmpFile)
	{
		fscanf(tmpFile, "%x", &mtdDeviceID);
		fclose(tmpFile);
	}

	if (mtdDeviceID != 0) // If there's information in mtdblock3, compare device ID
	{
		if (imageHeader.deviceID != mtdDeviceID)
		{
//june.chen, 2011-01-17, support Device ID for both WES610N (0x13) and WET610N (0x11). This is because WET610N and WES610N fw is interchangeable
#if 1
			if(((imageHeader.deviceID == 0x11) || (imageHeader.deviceID == 0x13)) && ((mtdDeviceID == 0x11) || (mtdDeviceID == 0x13))){
			}
			else{
				printf("Error: The image is not compatible with this hardware (device ID 0x%08X instead of 0x%08X)", imageHeader.deviceID, mtdDeviceID);
				return -1;
			}
#else
			printf("Error: The image is not compatible with this hardware (device ID 0x%08X instead of 0x%08X)", imageHeader.deviceID, mtdDeviceID);
			return -1;
#endif
//june.chen end
		}
	}else{
		printf("Warning: device ID not stored in mtdblock3 (or not stored correctly)\n");
	}

	memcpy(&crcFromFooter, imageBuf + imageSize - 4, 4);
	if (!isLittleEndian) {
		hnap_reverse_ulong(&crcFromFooter);
	}

	imageSizeFromFile = imageHeader.pcksum2 - sizeof(MT_IMAGE_HEADER);

	crc = crc32(0L, Z_NULL, 0);
	crc = crc32(crc, imageBuf + sizeof(MT_IMAGE_HEADER), imageSizeFromFile);
	if ((unsigned int)crc != crcFromFooter){
		printf("Error: Bad CRC: CRC is %08X but in the header its %08X !!!\n", (unsigned int)crc, crcFromFooter);
		return -1;
	}

	printf("CRC of image: %08X - OK.\n", (unsigned int)crc);

	if (crcFromFooter != imageHeader.cksum){
		printf("Error: Wrong image file (header CRC %08X != footer CRC %08X)\n", imageHeader.cksum, crcFromFooter);
		return -1;
	}

	return 0;
}

int hnap_ap_wbridge_upgrade( webs_t wp, int len, char *msg )
{
	//int i = 0, j=0;
	char_t *pstarTag, *pendTag;
	char_t *imageBuf;
	int imageLen_before_b64decode = 0, imageLen_after_b64decode=0;

	/*printf("The length of POST data is %d, and the content as below\n", wp->lenPostData);
	while(i<wp->lenPostData){
		printf("%c", *((wp->postData)+i));
		if(i!=0&&(*((wp->postData)+i-1) != '<')&&(*((wp->postData)+i) == '>'))
			printf("\n");
		i++;
	}
	printf("\n");*/
	
	pstarTag = strstr(wp->postData, "<Base64Image>");
	pstarTag += 13; //skip "<Base64Image>", and the pointer will locate the first byte of firmware image
	//printf("pstarTag=%d(%x), First 3 characters in firmware image is '%c%c%c'\n", pstarTag, pstarTag, *pstarTag, *(pstarTag+1), *(pstarTag+2));

	pendTag = strstr(wp->postData, "</Base64Image>");
	//This will replace '<' to '\0. we don't use the XML tag later, so just modify it ..
	//This is for b64_decode input string
	*pendTag = '\0'; 
	//printf("pendTag=%d(%x), Last 3 characters in firmware image is '%c%c%c'\n", pendTag, pendTag, *(pendTag-2), *(pendTag-1), *pendTag);

	printf("Got the frimware image from HNAP message ..\n");
	imageLen_before_b64decode = pendTag-pstarTag+1;
	printf("The size of firmware image before base64 decode is %d bytes\n", imageLen_before_b64decode-1);
	/*printf("The full image before base64 decode is as below..\n");
	printf("%s\n", pstarTag);*/

	//alloc memory buffer for base 64 decode operation of firmware image
	imageBuf = (char_t *)balloc(B_L, imageLen_before_b64decode* sizeof(char_t));
	if(imageBuf == NULL){
		printf("ERROR:allocate memory for base 64 decode fail\n");
		//bfree(B_L, wp->postData);
		return -1;
	}else{
		printf("allocate %d bytes buffer for base 64 decode\n", imageLen_before_b64decode);
	}
	
	//start to decode and b64_decode will return the length of firmware image after decode.
	imageLen_after_b64decode = b64_decode(pstarTag, imageBuf, imageLen_before_b64decode);
	printf("The size of firmware image after base64 decode is %d bytes\n", imageLen_after_b64decode);
	//since firmware image is so big, it will consume larger memory, so I use ourginal postData buffer to keep the 
	//decoded firmware image, and free the buffer of b64_decode operation
	memset(wp->postData, 0, wp->lenPostData);
	memcpy(wp->postData, imageBuf, imageLen_after_b64decode);
	bfree(B_L, imageBuf);
	
	/*printf("The full image after base64 decode is as below..\n");*/
	/*i=0, j=0;
	while(i<128){
		if(wp->postData[i]<0x10){
			printf("0%x ", wp->postData[i]);
		}else{
			printf("%2x ", wp->postData[i]);
		}
		i++;
		j++;
		if(j>15){
			printf("\n");
			j=0;
		}
	}
	printf("\n");*/

	if(hnap_validate_Image(wp->postData, imageLen_after_b64decode)!=-1){
		printf("This is a valid image .. \\^o^/..\n");
		if(hnap_burn_and_verify_image(wp->postData, imageLen_after_b64decode)!=-1){
			printf("Firmware upgrade successfully..device will be reboot..\n");
			return 0;
		}
	}else{
		printf("This is a invalid image .. Orz .. \n");
	}

	return -1;
}

int hnap_translate_month_string(char *month)
{
	char tmpBuf[4]={0};

	strcpy(tmpBuf, month);

	if(!strcasecmp(tmpBuf, "Jan")){
		strcpy(month, "01");
	}else if(!strcasecmp(tmpBuf, "Feb")){
		strcpy(month, "02");
	}else if(!strcasecmp(tmpBuf, "Mar")){
		strcpy(month, "03");	
	}else if(!strcasecmp(tmpBuf, "Apr")){
		strcpy(month, "04");
	}else if(!strcasecmp(tmpBuf, "May")){
		strcpy(month, "05");
	}else if(!strcasecmp(tmpBuf, "Jun")){
		strcpy(month, "06");
	}else if(!strcasecmp(tmpBuf, "Jul")){
		strcpy(month, "07");
	}else if(!strcasecmp(tmpBuf, "Aug")){
		strcpy(month, "08");
	}else if(!strcasecmp(tmpBuf, "Sep")){
		strcpy(month, "09");
	}else if(!strcasecmp(tmpBuf, "Oct")){
		strcpy(month, "10");
	}else if(!strcasecmp(tmpBuf, "Nov")){
		strcpy(month, "11");
	}else if(!strcasecmp(tmpBuf, "Dec")){
		strcpy(month, "12");
	}else{
		return -1;
	}

	return 0;
}

int hnap_get_frimware_version_date(char *Version, char *releaseDate)
{
	FILE *fptr;
	char tmpBuf[512]={0}, *tmpPtr1, *tmpPtr2;
	char yyyy[5]={0}, mm[4]={0}, dd[3]={0};	
	int i=0;
	int dateType = 1;

	fptr =  fopen("/root/mtlk/web/fw_version.txt", "r");
	if(fptr == NULL){
		printf("ERROR:%s\n", strerror(errno));
		return -1;
	}

	while(fgets(tmpBuf, 512, fptr) != NULL){
		if(strstr(tmpBuf, "ProjectFirmwareVersionDate")){
			if((tmpPtr1=strchr(tmpBuf, '"'))!=0){
				if((tmpPtr2=strchr(tmpBuf, ','))!=0){
					memcpy(Version, tmpPtr1+1, tmpPtr2-tmpPtr1-1);
					printf("The firmware version is %s\n", Version);

					tmpPtr1 = tmpPtr2+1;
					if((tmpPtr2=strchr(tmpPtr1, '"'))!=0){
						while(*tmpPtr1==0x20 && *tmpPtr1!='\0')
							tmpPtr1++;
						memcpy(releaseDate, tmpPtr1, tmpPtr2-tmpPtr1);
						while(*(releaseDate+i) != '\0'){
							if(*(releaseDate+i) == '-'){
								*(releaseDate+i) = 0x20;
								dateType=1;
							}else if(*(releaseDate+i) == ','){
								*(releaseDate+i) = 0x20;
								dateType=2;
							}
							i++;
						}
						if(dateType==1){
							if(sscanf(releaseDate, "%s %s %s", dd, mm, yyyy)!=3){
								printf("ERROR:get release date string fail !!\n");//Ricky Trace
								fclose(fptr);
								return -1;
							}
						}else if(dateType==2){
							if(sscanf(releaseDate, "%s %s %s", mm, dd, yyyy)!=3){
								printf("ERROR:get release date string fail !!\n");//Ricky Trace
								fclose(fptr);
								return -1;
							}
						}
						hnap_translate_month_string(mm);
						if(atoi(dd)>0 && atoi(dd)<10){
							sprintf(releaseDate, "%s-%s-0%sT00:00:00", yyyy, mm, dd);
							printf("The firmware release date is %s-%s-0%s\n", yyyy, mm, dd);
						}else{
							sprintf(releaseDate, "%s-%s-%sT00:00:00", yyyy, mm, dd);
							printf("The firmware release date is %s-%s-%s\n", yyyy, mm, dd);
						}
						
						fclose(fptr);
						return 0;
					}
				}
			}
			
			break;
		}
	}
	
	fclose(fptr);	
	printf("ERROR:Unknow Firmware Version !!\n");//Ricky Trace

	return -1;
}

int hnap_configure_dhcp(char *lanIp, char *lanNetMask)
{
	struct in_addr ipaddr, ipnetmask, first_addr, last_addr; 
	unsigned long ip=0, mask=0, netaddress=0, dhcpStartN=0, dhcpEndN=0;
	char DhcpRangeStart[16]={0}, DhcpRangeEnd[16]={0};
	printf("Check Point 1\n");
	if(!inet_aton (lanIp, &ipaddr) || !inet_aton (lanNetMask, &ipnetmask)){
		return -1;
	}
	ip=htonl(ipaddr.s_addr);
	mask=htonl(ipnetmask.s_addr);
	netaddress=ip&mask;

	if(~mask >= 255){
		dhcpStartN = netaddress+101;
		dhcpEndN=dhcpStartN+49;
	}else if(~mask >= 127){
		dhcpStartN = netaddress+51;
		dhcpEndN=dhcpStartN+49;
	}else if(~mask >= 63){
		dhcpStartN = netaddress+21;
		dhcpEndN=dhcpStartN+29;
	}else if(~mask >= 31){
		dhcpStartN = netaddress+11;
		dhcpEndN=dhcpStartN+14;
	}else if(~mask >= 15){
		dhcpStartN = netaddress+3;
		dhcpEndN=dhcpStartN+9;
	}else if(~mask >= 7){
		dhcpStartN = netaddress+2;
		dhcpEndN=dhcpStartN+3;
	}else if(~mask >= 3){
		dhcpStartN=netaddress+1;
		if(ip==dhcpStartN){
			dhcpStartN+=1;
		}
		dhcpEndN=netaddress+1+~mask-1;
		if(ip==dhcpEndN){
			dhcpEndN-=1;
		}
	}
	
	first_addr.s_addr=ntohl(dhcpStartN);
	last_addr.s_addr=ntohl(dhcpEndN);
	sprintf(DhcpRangeStart, "%s", inet_ntoa(first_addr));
	sprintf(DhcpRangeEnd, "%s", inet_ntoa(last_addr));
	printf("Lan IP=%s, Lan Netmask=%s, Configure the range of dhcp server ip address pool to \"%s - %s\"\n", lanIp, lanNetMask, DhcpRangeStart, DhcpRangeEnd);

	device_set_string_value(CONFIG_SYS_LAN_DHCP_START, DhcpRangeStart);
	device_set_string_value(CONFIG_SYS_LAN_DHCP_END, DhcpRangeEnd);

	device_set_string_value(CONFIG_SYS_LAN_DHCP_SUBNET, lanNetMask);
	device_set_string_value(CONFIG_SYS_LAN_DHCP_WINS, lanIp);

	return 0;
}

int hnap_do_wps_pbc_direct_tv(void)
{
	int i=0, pid=0;
	char tmpBuf[128]={0};
	FILE *fptr;

	fptr = popen("ps |grep /dev/gpio13", "r");
	if(fptr==NULL)
		return -1;
	
	while(fgets(tmpBuf, 128, fptr)!=NULL){
		if(!strstr(tmpBuf, "grep") && strstr(tmpBuf, "/dev/gpio13")){
			while(tmpBuf[i]==0x20)
				i++;
			
			while(tmpBuf[i]!=0x20 && tmpBuf[i]!='\t')
				i++;
			
			tmpBuf[i]='\0'; //Got pid
			pid=atoi(tmpBuf);
			pclose(fptr);
			memset(tmpBuf, 0, 128);
			sprintf(tmpBuf, "kill -9 %d", pid);
			system(tmpBuf);
			return 0;
		}
	}
	pclose(fptr);
	
	return -1;
}

#define WPS_STATUS_IDLE						0
#define WPS_STATUS_SEARCHING					1
#define WPS_STATUS_REGISTERING				2
#define WPS_STATUS_CONNECTING				3
#define WPS_STATUS_ERROR_WALK_TIMEOUT		10
#define WPS_STATUS_ERROR_SESSION_TIMEOUT	11
#define WPS_STATUS_ERROR_SESSION_OVERLAP	12
#define WPS_STATUS_ERROR_INVALID_PIN			13
#define WPS_STATUS_ERROR_CONNECT_FAILURE	14
#define WPS_STATUS_CONNECTED					99

int hnap_clear_unconfigured_wildcardSSID(void)
{
	char_t *argp1[] = {CONFIG_WLAN_WILDCARD_SSID};
	char_t *argp2[] = {"unconfigured", CONFIG_WLAN_WILDCARD_SSID};	
	char paramValue[MT_MAX_PARAM_VALUE_LENGTH]={0};

	device_set_string_value(CONFIG_WLAN_WILDCARD_SSID, "");

	hnap_get_config("unconfigured", paramValue); //0:station, 2:AP
	if (!strncmp(paramValue, "1", 1)){
		device_set_string_value("unconfigured", "0");		
		hnap_commit_changes(2, argp2, COMMIT_WLAN0);
	}else{
		hnap_commit_changes(1, argp1, COMMIT_WLAN0);
	}

	return 0;
}

int hnap_clear_unconfigured_flag(void)
{
	char_t *argp[] = {"unconfigured"};
	int argc = 1;
	char paramValue[MT_MAX_PARAM_VALUE_LENGTH]={0};

	hnap_get_config("unconfigured", paramValue); //0:station, 2:AP
	if (!strncmp(paramValue, "1", 1)){
		device_set_string_value("unconfigured", "0");		
		hnap_commit_changes(argc, argp, COMMIT_WLAN0);
	}

	return 0;
}

/*int recover_wildcardssid(void)
{
	char paramValue[MT_MAX_PARAM_VALUE_LENGTH]={0};
	char_t *argp[] = {"Wildcard_ESSID"};

	hnap_get_config("NonProc_ESSID", paramValue);
	device_set_string_value("Wildcard_ESSID", paramValue);		
	hnap_commit_changes(1, argp, COMMIT_WLAN0);

	return 0;
}*/

int hnap_active_wps_monitor(void)
{
	FILE *fptr;
	struct stat statBuf;
	char buf[128]={0};	
	int result=0, currentStatus=0;

	//1. Detect whether has another WPS in progress.., if yse, abort it before start this request
	result = stat("/tmp/hnap_wps_status",&statBuf);
	if(result != -1){
		fptr = popen("cat /tmp/hnap_wps_status", "r");
		if(fptr!=NULL){
			fgets(buf, 128, fptr);
			currentStatus=atoi(buf);
			pclose(fptr);
		}
	}
	//2. If current WPS status is SEARCHING, REGISTING, CONNECTNG or any ERROR, then abort them first
	if(currentStatus!=WPS_STATUS_IDLE){
		//hnap_doSystem("/root/mtlk/etc/mtlk_wps_cmd.tcl abort; /root/mtlk/etc/mtlk_wps_cmd.tcl start");
		if((currentStatus != WPS_STATUS_ERROR_WALK_TIMEOUT) && (currentStatus != WPS_STATUS_ERROR_SESSION_TIMEOUT) && (currentStatus != WPS_STATUS_ERROR_SESSION_OVERLAP) && (currentStatus != WPS_STATUS_ERROR_INVALID_PIN) && (currentStatus != WPS_STATUS_ERROR_CONNECT_FAILURE)){
			system("/root/mtlk/web/stop_wps_progress.sh");
		}
	}

	//3. Detect whether has another WPS status monitor is running.., If yes, then just kill it.
	result = stat("/var/run/hnap_wps_status_monitor", &statBuf);
	if(result != -1){
		fptr = popen("cat /var/run/hnap_wps_status_monitor", "r");
		if(fptr!=NULL){
			fgets(buf, 128, fptr);
			kill(atoi(buf), SIGUSR1);
			pclose(fptr);
			sleep(2);
		}
	}

	//4. Active WPS status monitor
	//For new wsccmd seem doesn't delete the wps_current_status sometime, so we need delete it by ourself for hnap_wps_status_monitor
	result = stat("/tmp/wps_current_status", &statBuf);
	if(result != -1){
		system("rm /tmp/wps_current_status");
	}
	system("echo 0 > /tmp/hnap_wps_status");
	system("/root/mtlk/etc/hnap_wps_status_monitor &");

	return 0;
}

int hnap_do_wps_pbc(void)
{
	char paramValue[MT_MAX_PARAM_VALUE_LENGTH]={0};
	int result=0, retryCount=0;
	struct stat statBuf;
	//pthread_t wps_monitor_thread;

	//1. Active the hnap wps monitor 
	hnap_active_wps_monitor();

	//2. Clear unconfigured flag in configuration database, 
	//    if we don't clear it then WET610 will not connect to AP after WPS completed
	//hnap_clear_unconfigured_flag(); //Jacky do it in wps.tcl, so I don't need to clear it

	//3. Do WPSStart with PBC.
	hnap_get_config(CONFIG_NETWORK_TYPE, paramValue); //0:station, 2:AP
    	if (!strncmp(paramValue, "0", 1)){
		printf("Trigger WPS-PBC of Wireless Client\n"); //Ricky Test
    	}else{
    		printf("Trigger WPS-PBC of Access Point\n"); //Ricky Test
    	}
	hnap_doSystem("/root/mtlk/etc/mtpriv wlan0 Wildcard_ESSID '' &");
	hnap_doSystem("/root/mtlk/etc/mtlk_wps_cmd.tcl %s &", !strncmp(paramValue, "0", 2)?"get_conf_via_pbc":"conf_via_pbc");

	//Check whether WPS is real running
	while(retryCount<15){
		result = stat("/tmp/wps_action",&statBuf);
		if(result == -1){
			sleep(1);
			retryCount++;
			continue;
		}else{
			break;
		}
	}

	system("echo 1 > /tmp/hnap_wps_status");
	//sleep(3); //For pass the WPS test of TestDevice

	return 0;
}

int hnap_do_wps_pin(char *pinCode, char *bssid)
{
	char paramValue[MT_MAX_PARAM_VALUE_LENGTH]={0};
	int result=0, retryCount=0;
	struct stat statBuf;
	//pthread_t wps_monitor_thread;

	//1. Active the hnap wps monitor 
	hnap_active_wps_monitor();

	//2. Clear unconfigured flag in configuration database, 
	//    if we don't clear it then WET610 will not connect to AP after WPS completed
	//hnap_clear_unconfigured_flag(); //Jacky do it in wps.tcl, so I don't need to clear it

	//3. Do WPSStart with PIN.	
	hnap_get_config(CONFIG_NETWORK_TYPE, paramValue); //0:station, 2:AP
	hnap_doSystem("/root/mtlk/etc/mtpriv wlan0 Wildcard_ESSID ''");
    	if (!strncmp(paramValue, "0", 1)){
		printf("Trigger WPS-PIN for Wireless Client\n");//Ricky Test
		if(bssid==NULL){
			//HNAP client doesn't assign a BSSID for specific AP, we will auto detect an AP for connect by WPS PIN.
			printf("Auto detect an AP for connect by WPS PIN\n");
			hnap_doSystem("/root/mtlk/etc/mtlk_wps_cmd.tcl get_conf_via_pin 0 &");
		}else{
			//Do WPS PIN with specfic AP
			printf("Do WPS PIN with specfic AP(%s)\n", bssid);
			hnap_doSystem("/root/mtlk/etc/mtlk_wps_cmd.tcl get_conf_via_pin 1; /root/mtlk/etc/mtlk_wps_cmd.tcl ap_selected_from_list %s &", bssid);
			//hnap_doSystem("/root/mtlk/etc/mtlk_wps_cmd.tcl ap_selected_from_list %s", bssid);
		}
    	}else{
    		printf("Trigger WPS-PIN for Access Point\n");//Ricky Test
    		hnap_doSystem("/root/mtlk/etc/mtlk_wps_cmd.tcl get_conf_via_pin &");
	}

	//Check whether WPS is real running
	while(retryCount<15){
		result = stat("/tmp/wps_action",&statBuf);
		if(result == -1){
			sleep(1);
			retryCount++;
			continue;
		}else{
			break;
		}
	}

	system("echo 1 > /tmp/hnap_wps_status");
	//sleep(3); //For pass the WPS test of TestDevice

	return 0;
}

int hnap_stop_wps(void)
{
	int result=0;
	struct stat statBuf;
	FILE *fptr;
	char buf[128]={0};

	printf("Calling StopWPS .. \n");

	//recover_wildcardssid();	

	//1. Detect whether WPS is running.., If yes, then just skip the STOP request.
	result = stat("/tmp/hnap_wps_status",&statBuf);
	if(result == -1){
		memset(buf, 0, 128);
		strcpy(buf, strerror(errno));
		//printf("ERROR MSG: %s(%d)\n", buf, errno); //Ricky Trace
		if(errno==2){
			printf(" WPS is IDLE, skip the STOP request.\n");
			return 0;
		}
	}

	fptr = popen("cat /tmp/hnap_wps_status", "r");
	fgets(buf, 128, fptr);
	if(atoi(buf)==0){
		printf(" WPS is IDLE, skip the STOP request.\n");
		pclose(fptr);
		return 0;		
	}
	pclose(fptr);

	//3. Do WPS STOP
	system("/root/mtlk/web/stop_wps_progress.sh");
	//system("/root/mtlk/etc/mtlk_wps_cmd.tcl abort; /root/mtlk/etc/mtlk_wps_cmd.tcl start");
	hnap_doSystem("rm /tmp/wps_last_code");
	//sleep(1);
	//hnap_doSystem("/root/mtlk/etc/mtlk_wps_cmd.tcl start");

	//2. Detect whether has another WPS status monitor is running.., If yes, then just kill it.
	result = stat("/var/run/hnap_wps_status_monitor", &statBuf);
	if(result != -1){
		fptr = popen("cat /var/run/hnap_wps_status_monitor", "r");
		if(fptr!=NULL){
			fgets(buf, 128, fptr);
			kill(atoi(buf), SIGUSR1);
			pclose(fptr);
		}
	}

	return 0;
}

int hnap_get_wps_status(char *status)
{
	int wpsStatus=0, result=0;
	struct stat statBuf;
	FILE *fptr;
	char buf[128]={0};
	int retry=0;

	//1. Check the exist of hnap_wps_status
	while(1){
		result = stat("/tmp/hnap_wps_status",&statBuf);
		if(result == -1){
			memset(buf, 0, 128);
			strcpy(buf, strerror(errno));
			printf("WARNNING: %s(%d)\n", buf, errno); //Ricky Trace
			if(errno==2){
				//If we can not found the hnap_wps_status then we will try again after 1 seconds
				result = stat("/var/run/hnap_wps_status_monitor", &statBuf);
				if(result != -1){
					retry++;
					if(retry>5){
						//If retry exceed 5 times, then we will report IDLE directly
						printf("Can not found WPS status, report IDLE.\n");
						strcpy(status, "IDLE");
						return 0;					
					}else{
						sleep(1);
						continue;
					}
				}else{
					printf("Can not found WPS status, report IDLE.\n");
					strcpy(status, "IDLE");
					return 0;					
				}		
			}
		}else{
			break;
		}
	}

	//2. Check the NULL status of hnap_wps_status
	//If hnap_wps_status_monitor is writing status to hnap_wps_status, then we may got NULL status from it.
	//So we try every 1 seconds for get correct status
	//If retry exceed 5 times, then we will report IDLE directly	
	retry=0;
	while(1){
		fptr = fopen("/tmp/hnap_wps_status", "r");
		if(fptr!=NULL){
			memset(buf, 0, 128);
			fgets(buf, 128, fptr);
			fclose(fptr);
			if(strlen(buf)!=0 && strcmp(buf, "")){
				wpsStatus = atoi(buf);
				if(strchr(buf, '\n'))
					*(strchr(buf, '\n'))='\0';
				printf("status got from hnap_wps_status is %d(%s)\n", wpsStatus, buf);
				break;
			}
			printf("WARNING: Got NULL string from /tmp/hnap_wps_status..\n");
		}

		retry++;
		printf("WARNING: %dth cat /tmp/hnap_wps_status fail\n", retry);
		if(retry>5){
			printf("ERROR: Fail to cat /tmp/hnap_wps_status, report IDLE\n");
			wpsStatus=0;
			break;
		}else{
			sleep(1);
		}
	}

	//3. Verify status of hnap_wps_status
	if(wpsStatus!=WPS_STATUS_IDLE && wpsStatus!=WPS_STATUS_SEARCHING && 
		wpsStatus!=WPS_STATUS_REGISTERING && wpsStatus!=WPS_STATUS_CONNECTING &&
		wpsStatus!=WPS_STATUS_ERROR_WALK_TIMEOUT && wpsStatus!=WPS_STATUS_ERROR_SESSION_TIMEOUT &&
		wpsStatus!=WPS_STATUS_ERROR_SESSION_OVERLAP && wpsStatus!=WPS_STATUS_ERROR_INVALID_PIN && 
		wpsStatus != WPS_STATUS_ERROR_CONNECT_FAILURE && wpsStatus!=WPS_STATUS_CONNECTED){
		printf("ERROR: Got a invalid status code\n");
		return -1;
	}

	//4. Expose the WPS statuss
	switch(wpsStatus){
		case WPS_STATUS_IDLE:
			printf("Got IDLE event\n");
			strcpy(status, "IDLE");
			break;
		case WPS_STATUS_SEARCHING:
			strcpy(status, "SEARCHING");
			break;
		case WPS_STATUS_REGISTERING:
			strcpy(status, "REGISTERING");
			break;
		case WPS_STATUS_CONNECTING:
			strcpy(status, "CONNECTING");
			break;
		case WPS_STATUS_ERROR_WALK_TIMEOUT:
			strcpy(status, "ERROR_WALK_TIMEOUT");
			break;
		case WPS_STATUS_ERROR_SESSION_TIMEOUT:
			strcpy(status, "ERROR_SESSION_TIMEOUT");
			break;
		case WPS_STATUS_ERROR_SESSION_OVERLAP:
			strcpy(status, "ERROR_SESSION_OVERLAP");
			break;
		case WPS_STATUS_ERROR_INVALID_PIN:
			strcpy(status, "ERROR_INVALID_PIN");
			break;
		case WPS_STATUS_ERROR_CONNECT_FAILURE:
			strcpy(status, "ERROR_CONNECT_FAILURE");
			break;
		case WPS_STATUS_CONNECTED:
			strcpy(status, "CONNECTED");
			break;
	}

	return 0;
}

/*int hnap_get_wps_status_old(char *status)
{
	int result=0, statusCode=0;
	struct stat statBuf;
	FILE *fptr;
	char buf[128]={0};
	char *bufptr;
	
	printf("1. Detect device last status..\n");
	result = stat("/tmp/wps_last_code", &statBuf);
	if(result != -1){
		printf("Found /tmp/wps_last_code !!!\n");
		fptr = popen("cat /tmp/wps_last_code", "r");
		if(fptr != NULL){
			printf("open /tmp/wps_last_code successfully!!!\n");
			while(fgets(buf, 128, fptr)!=NULL){
				printf("fgets = %s\n", buf);
				bufptr = strchr(buf, '=');
				if(bufptr==0){
					continue;
				}
				bufptr = bufptr+1;
				statusCode = atoi(bufptr);
				printf("Code in wps_last_code = %d\n", statusCode);
				pclose(fptr);
				if(statusCode==2 && hnap_get_station_link_status()){
					strcpy(status, "CONNECTED");
					system("rm /tmp/wps_last_code");
					return 0;
				}else if(statusCode==3){
					strcpy(status, "ERROR_SESSION_OVERLAP");
					system("rm /tmp/wps_last_code");
					return 0;				
				}else if(statusCode==4){
					fptr = fopen("/tmp/wps_current_status", "r");
					if(fptr!=NULL){
						memset(buf, 0, 128);
						if(fgets(buf, 128, fptr)!=-1){
							printf("wps_current_status = %s\n", buf);
							bufptr = strchr(buf, '=');
							bufptr = bufptr+1;
							statusCode = atoi(bufptr);
							printf("Status Code = %d\n", statusCode); //Ricky Trace
							if(statusCode==10){
								strcpy(status, "ERROR_WALK_TIMEOUT");
								system("rm /tmp/wps_last_code");
								fclose(fptr);
								return 0;
							}else if(statusCode==11){
								strcpy(status, "ERROR_SESSION_TIMEOUT");
								system("rm /tmp/wps_last_code");
								fclose(fptr);
								return 0;
							}
						}
						fclose(fptr);
					}else{
						strcpy(status, "ERROR_WALK_TIMEOUT");
						system("rm /tmp/wps_last_code");
					}
					system("rm /tmp/wps_last_code");
				}else if(statusCode==1 || statusCode==6){
					result = stat("/tmp/wps_startup_time",&statBuf);
					memset(buf, 0, 128);
					strcpy(buf, strerror(errno));
					printf("ERROR MSG: %s(%d)\n", buf, errno); //Ricky Trace
					if(errno==2){ 
						strcpy(status, "IDLE");
						system("rm /tmp/wps_last_code");
						return 0;
					}
				}
			}
		}
	}

	printf("2. Detect device is IDLE..\n");
	result = stat("/tmp/wps_startup_time",&statBuf);
	if(result == -1){
		memset(buf, 0, 128);
		strcpy(buf, strerror(errno));
		printf("ERROR MSG: %s(%d)\n", buf, errno); //Ricky Trace
		if(errno==2){
			result = stat("/tmp/wps_last_code",&statBuf);
			if(result==-1 && errno==2){
				memset(buf, 0, 128);
				strcpy(buf, strerror(errno));
				printf("ERROR MSG: %s(%d)\n", buf, errno); //Ricky Trace	
				if(errno==2){
					strcpy(status, "IDLE");
					return 0;
				}
			}
		}else{
			return -1;
		}
	}

	printf("3. Detect device is STATUS..\n");
	fptr = popen("cat /tmp/wps_current_status", "r");
	if(fptr==NULL){
		return -1;
	}
	memset(buf, 0, 128);
	if(fgets(buf, 128, fptr)!=-1){
		printf("wps_current_status = %s\n", buf);
		bufptr = strchr(buf, '=');
		bufptr = bufptr+1;
		statusCode = atoi(bufptr);
		printf("Status Code = %d\n", statusCode); //Ricky Trace

		if(statusCode==0){
			return -1;
		}else{
			if(statusCode==1){
				strcpy(status, "SEARCHING");
			}else if(statusCode==2){
				strcpy(status, "REGISERING");
			}else if(statusCode==2){
				strcpy(status, "REGISERING");
			}else if(statusCode==3){
				strcpy(status, "CONNECTING");
			}else if(statusCode==10){
				strcpy(status, "ERROR_WALK_TIMEOUT");
			}else if(statusCode==11){
				strcpy(status, "ERROR_SESSION_TIMEOUT");
			}else if(statusCode==12){
				strcpy(status, "ERROR_SESSION_OVERLAP");
			}else if(statusCode==13){
				strcpy(status, "ERROR_INVALID_PIN");
			}
		}
	}else{
		printf("ERROR: fgets fail\n");
		pclose(fptr);
		return -1;
	}
		
	pclose(fptr);
	
	return 0;
}*/

int hanp_get_value_from_eeprom(char *tag, char *value)
{
	char command[64]={0};
	FILE *fptr;

	sprintf(command, "/bin/get_env_param %s", tag);
	fptr=popen(command, "r");
	
	if(fptr==NULL){
		printf("ERROR: popen for /bin/get_env_param %s fail !!!\n", tag);
		return -1;
	}

	if(fgets(value, 64, fptr)==NULL){
		printf("fgets for /bin/get_env_param serial_no fail !!!\n");
		pclose(fptr);
		return -1;
	}

	printf("Device %s is %s\n", tag, value);
	pclose(fptr);

	return 0;
}

int hnap_get_serial_number(char *serialNo)
{
	FILE *fptr=popen("/bin/get_env_param serial_no", "r");
	
	if(fptr==NULL){
		printf("popen for /bin/get_env_param serial_no fail !!!\n");
		return -1;
	}

	if(fgets(serialNo, 16, fptr)==NULL){
		printf("fgets for /bin/get_env_param serial_no fail !!!\n");
		pclose(fptr);
		return -1;
	}

	printf("Device Serial Number is %s\n", serialNo);
	pclose(fptr);

	return 0;
}

int hnap_get_interface_statistics(char *ifName, void *data)
{
	FILE *fptr;
	char tmpBuf[512];
	struct netStats *netStatistic = (struct netStats *)data;
	char paramValue[MT_MAX_PARAM_VALUE_LENGTH]={0}; //for get current setting from configuration
	int networkMode = 0;

	fptr =  popen("cat /proc/net/dev", "r");
	if(fptr == NULL){
		printf("ERROR:%s\n", strerror(errno));
		return -1;
	}

	while(fgets(tmpBuf, 512, fptr)!=NULL){
		if(strstr(tmpBuf, ifName)){
			sscanf(tmpBuf, "%s %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu", netStatistic->if_name, 
				&netStatistic->rx_bytes, &netStatistic->rx_packets, &netStatistic->rx_errs, &netStatistic->rx_drop, 
				&netStatistic->rx_fifo, &netStatistic->rx_frame, &netStatistic->rx_compressed, &netStatistic->rx_multicast, 
				&netStatistic->tx_bytes, &netStatistic->tx_packets, &netStatistic->tx_errs, &netStatistic->tx_drop, 
				&netStatistic->tx_fifo, &netStatistic->tx_frame, &netStatistic->tx_compressed, &netStatistic->tx_multicast);
			if(!strcmp(ifName, "eth0")){
				strcpy(netStatistic->if_name, "LAN");
			}else if(!strcmp(ifName, "wlan0")){
				hnap_get_config(CONFIG_WLAN_NETWORK_MODE, paramValue);
				networkMode = atoi(paramValue);
				printf("NetworkMode in configuration is %d\n", networkMode);
				switch(networkMode){
					case 17: //b only
						strcpy(netStatistic->if_name, "WLAN 802.11b");
						break;
					case 18: //g only
					case 19: //b/g mixed
						strcpy(netStatistic->if_name, "WLAN 802.11g");
						break;
					case 22: //n only
					case 23: //b/g/n mixed
						strcpy(netStatistic->if_name, "WLAN 802.11n");
						break;
					case 10: //a only
						strcpy(netStatistic->if_name, "WLAN 802.11a");
						break;
					case 12: //n only
					case 14: //a/n mixed
						strcpy(netStatistic->if_name, "WLAN 802.11n");
						break;
					default:
						strcpy(netStatistic->if_name, "WLAN 802.11n");
						break;
				};			
			}
			
			printf("%s statistics as below..\n", ifName);
			printf("rx bytes = %lu\n", netStatistic->rx_bytes);
			printf("rx packets = %lu\n", netStatistic->rx_packets);
			printf("rx errs = %lu\n", netStatistic->rx_errs);
			printf("rx drop = %lu\n", netStatistic->rx_drop);
			printf("rx fifo = %lu\n", netStatistic->rx_fifo);
			printf("rx frame = %lu\n", netStatistic->rx_frame);
			printf("rx compressed = %lu\n", netStatistic->rx_compressed);
			printf("rx multicast = %lu\n", netStatistic->rx_multicast);
			printf("tx bytes = %lu\n", netStatistic->tx_bytes);
			printf("tx packets = %lu\n", netStatistic->tx_packets);
			printf("tx errs = %lu\n", netStatistic->tx_errs);
			printf("tx drop = %lu\n", netStatistic->tx_drop);
			printf("tx fifo = %lu\n", netStatistic->tx_fifo);
			printf("tx frame = %lu\n", netStatistic->tx_frame);
			printf("tx compressed = %lu\n", netStatistic->tx_compressed);
			printf("tx multicast = %lu\n", netStatistic->tx_multicast);			
		}
	}

	pclose(fptr);

	return 0;
}

