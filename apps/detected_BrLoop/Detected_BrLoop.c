/*
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
#include <stdlib.h>
#include <string.h>

#define UMEDIA_TRUE 1
#define UMEDIA_FALSE 0

#define SYSFS_BRIDGE_LOOP "/sys/class/net/br0/bridge/bridge_loop_detected"
#define SYSFS_LOCAL_TOPOLOGY_CHANGE "/sys/class/net/br0/bridge/local_topology_change"
#define CHECK_INTERVAL 1
#define CHECK_PORT_PHY_INTERVAL 2
#define CHECK_STILL_LOOP_INTERVAL 4

#define CMD_LEN 200

typedef struct
{
	int br_loop;   //Is bridge loop?
	int tp_change; //Is topology change ?
//	int disTrue;
//	char portmap[5]; //Phy port disable state
	//char *ptr_portmap;	
	int disablePort; //which port have been disable when loop 
	char portLink[4];  //port0~port4 link status 0 is down,1 is up	
	int ifw_status; //previously wireless interface status 0 is down ,1 is up

	//june.chen, 2011-01-03, for record previous port link status
	char prePortLink[4];
	int initialized;
} TPState; //topology state

extern int get_station_layer3_link_status(void);
extern void initLinkUtil();
extern int check_somefile_is_exist(char *file_path);
//june.chen, 2011-01-03, index for currenr diable port, round to 0 when > 4
static int cur_dis_port = 0; 

void Usage()
{
	printf("usage: BrDetectLoop\n");

}

void port_status(TPState *tp)
{
	FILE * pp;
	int i = 0;

	pp = popen("cat /proc/str9100/switch_get_port_link_status", "r");

	if(pp == NULL){
		printf("[%s %d] popen fail!\n",__func__, __LINE__);
		return;
	}

	while(i < 4){
		tp->portLink[i++] = fgetc(pp);
		fgetc(pp);
	}

//	printf("port link status = %s\n", tp->portLink);

	pclose(pp);
}


void reEnableDisPort()
{
	FILE *pp;
	int i = 0;
	char portStatus[4] = {0};
	pp = popen("cat /proc/str9100/switch_port_enable", "r");
	if(pp == NULL){
		printf("[%s %d] popen fail!\n",__func__, __LINE__);
		return;
	}

	i = 3;
	while(i >= 0){
		portStatus[i] = fgetc(pp);		

		if(portStatus[i] != '3'){
			char cmd_sys[CMD_LEN] = {0};
			sprintf(cmd_sys,"echo %d > /proc/str9100/switch_port_enable", i);
			system(cmd_sys);
		}
		i--;
	}
	
	pclose(pp);
}

void reset_Portmap(TPState *tp)
{
	char cmd_sys[CMD_LEN] = {0};

	reEnableDisPort();

	tp->disablePort = -1;
//	tp->disTrue=UMEDIA_FALSE;
	tp->initialized = UMEDIA_FALSE;
	
	//set local_topology_change is 1 for aging time of bridge use forward_delay time,see IEEE Std  802.1D  
	sprintf(cmd_sys,"echo 1 > %s",SYSFS_LOCAL_TOPOLOGY_CHANGE);	
	system(cmd_sys);		
	tp->tp_change=UMEDIA_TRUE;
}

void init_status(TPState *tp)
{
	char cmd_sys[CMD_LEN]={0};
	reset_Portmap(tp);
	//get port link status
	port_status(tp);

	memset(tp->portLink, 0, sizeof(tp->portLink));
	memset(tp->prePortLink, 0, sizeof(tp->prePortLink));
	tp->ifw_status = 0; //wireless default not connected
	tp->initialized = UMEDIA_FALSE;
	
	sprintf(cmd_sys, "echo 0 > %s", SYSFS_LOCAL_TOPOLOGY_CHANGE);	
	system(cmd_sys);		
	tp->tp_change=UMEDIA_FALSE;	
}

int ifw_status()
{
	int result = get_station_layer3_link_status();
	return result;
}

int getPreDisPort()
{
	if(cur_dis_port == 0){
		return 3;
	}
	else{
		return (cur_dis_port -1);
	}
}

int main(int argc, char* argv[])
{	
	int i, checkTime = 0, tp_time_out = 0, checkStillLoopTime = 0;
	static TPState tp;
	int cur_ifw_status = 0;
	char cmd_sys[CMD_LEN], cmd_loop[CMD_LEN];
 
	FILE * pp;
	if(argc < 1)
	{
		Usage();
		return -1;
	}
	memset(cmd_loop, 0, sizeof(cmd_loop));
	sprintf(cmd_loop, "cat %s", SYSFS_BRIDGE_LOOP);

	init_status(&tp);
	//june.chen, 2011-01-03, initialize some parameters for link utility use
	initLinkUtil();

	//june.chen, 2010-03-22, reset bridge forward delay to 0 
	//june.chen, 2010-03-16, set bridge forward delay to 0.1 to work around issue that cannot connect to Buffalo WZR-AG300N
	//june.chen, 2010-01-13, set bridge forward delay to 0.5 to prevent abnormal WPS status page flashing
	//june.chen, 2010-01-06, workaround to set bridge forward delay to 1, it could speed up loop detection
	//system("brctl setfd br0 0.1");
	//june.chen, 2010-01-06, tell bridge the device type is WES610N
	system("echo 1 > /sys/class/net/br0/bridge/device_is_wes610n");
	
	while(1)
	{		
		/*********  reset	local_topology_change to default (0) when aging time out **********/	
		//printf("tp.tp_change=%d \n",tp.tp_change);
		if(tp.tp_change){
			tp_time_out = tp_time_out + CHECK_INTERVAL;
			//printf("tp_time_out=%d \n",tp_time_out);

			if(tp_time_out > 15){
				memset(cmd_sys, 0, sizeof(cmd_sys));
				sprintf(cmd_sys, "echo 0 > %s", SYSFS_LOCAL_TOPOLOGY_CHANGE);
				system(cmd_sys);
				tp.tp_change = UMEDIA_FALSE;	
				tp_time_out = 0;//UMEDIA_FALSE;
			}
		}

		/*********  check port and wireless link status	**********/	
		if(checkTime >= (CHECK_INTERVAL * CHECK_PORT_PHY_INTERVAL) ){
//			printf("********* check port status *************\n");
			//get port link status
			port_status(&tp);	
			/* reset portmap when disablePort link is down */
			//printf("tp.portLink[%d]=%c\n", tp.disablePort, tp.portLink[tp.disablePort]);
//			if((tp.disablePort >= 0) && (tp.portLink[tp.disablePort] == '0')){
			if(tp.disablePort >= 0){
				if(tp.portLink[tp.disablePort] != tp.prePortLink[tp.disablePort]){
					printf("Physical port status change!!\n");
					reset_Portmap(&tp);
				}
			}

			memcpy(tp.prePortLink, tp.portLink, sizeof(tp.prePortLink));
			
			/* reset portmap when wireless link was change*/
			cur_ifw_status = ifw_status();
			//printf("cur_ifw_status=%d\n",cur_ifw_status);			
//			if((tp.disablePort > 0 ) && (cur_ifw_status != tp.ifw_status)){
			if((tp.disablePort >= 0) && (cur_ifw_status != tp.ifw_status)){
				printf("Wireless interface status change!!\n");
				reset_Portmap(&tp);
			}
		
			tp.ifw_status = cur_ifw_status;
			checkTime = 0;
		}
		checkTime = checkTime + CHECK_INTERVAL;		

		//june.chen, 2011-01-19, we should stop checking loop during WPS process, because it will cause WPS status page stall.
		//if(check_somefile_is_exist("/var/wpsLoopDetect") == 0)	{
		if(check_somefile_is_exist("/var/wpsLoopDetect") == 0)	{
			//printf("*********Bridge loop detect *************\n");
			for(i = 0; i < 4; i++){
				pp = popen(cmd_loop, "r"); 	

				if(pp == NULL) {
					printf("[%s %d]popen fail!\n", __func__, __LINE__);
					return -1;
				}

				fscanf(pp, "%d", &tp.br_loop);

				if(tp.br_loop /*&& tp.disTrue == UMEDIA_FALSE*/){
					tp.disablePort = cur_dis_port % 4;
					//tp.portmap[tp.disablePort] = '1';

					if(tp.initialized == 1){
						reEnableDisPort();
					}
					else{
						tp.initialized = 1;
					}	

					sprintf(cmd_sys, "echo %d > /proc/str9100/switch_port_disable", tp.disablePort);
					system(cmd_sys);	

					sprintf(cmd_sys,"echo 0 > %s\n", SYSFS_BRIDGE_LOOP);	
					system(cmd_sys);
					printf("Network Loop !! disable port %d \n", tp.disablePort);

					cur_dis_port = (cur_dis_port + 1) % 4;
					tp.tp_change = UMEDIA_TRUE;
					tp_time_out = 0;
					//force sending a brocast packet to check if block the right port.
					system("arping -c 1 localhost");
				}
				else{
					pclose(pp); 
					break;
				}
	
				pclose(pp);	
				//usleep(1000);
			}
		
		}
		else{
			//june.chen, 2011-01-19, during WPS it will cause loop, but we won't handle it druing WPS so just clear loop flag
			sprintf(cmd_sys,"echo 0 > %s\n", SYSFS_BRIDGE_LOOP);	
			system(cmd_sys);
		}
		sleep(CHECK_INTERVAL);
	}

	return 0;
}


