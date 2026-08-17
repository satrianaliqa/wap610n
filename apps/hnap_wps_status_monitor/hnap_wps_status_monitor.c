#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include<sys/stat.h>
#include <sys/types.h>

#define CONFIG_VALE_MAXLEN	512

int gotSignal=0;

void signal_handler(int sig)
{
	if(sig == SIGUSR1){
		printf("Got a SIGUSER1 from external process, prepare to terminate wps status monitor..\n");
		gotSignal=1;
	}
}

int get_station_link_status(void)
{
	uint8_t tmpBuf[512] = {0};
	uint8_t ifName[16] = {0};
	int link_status=0, nwid, crypt, frag, retry, misc, missed_beacon, we;
	uint8_t link[16]={0}, level[16]={0}, noise[16]={0};
	int paramCount = 0;
	FILE *fptr = popen("cat /proc/net/wireless", "r");

	//printf("is_station_wlan_link_up..\n");
	while(fgets(tmpBuf, 512, fptr) != NULL){
		if(strstr(tmpBuf, "Inter-") || strstr(tmpBuf, "face")){
			continue;
		}
		
		paramCount = sscanf(tmpBuf, "%s %d %s %s %s  %d %d %d %d %d %d %d %d", ifName, &link_status, link, level, noise, &nwid, &crypt, &frag, &retry, &misc, &missed_beacon);
		//printf("paramCount=%d, link_status=%d\n", paramCount, link_status);
		if(paramCount == 11)
			printf("=====> link status of wlan is %d\n", link_status);
	}
	
	pclose(fptr);

	return link_status==3?1:0;
}

int expose_wps_status(int statusCode)
{
	char command[128]={0};

	sprintf(command, "echo %d > /tmp/hnap_wps_status", statusCode);
	system(command);
}

/*int get_config(char *config_value, char *config_name, char *config_file)
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

int recover_wildcardssid(void)
{
	FILE *fptr;
	char command[256]={0};
	char configValue[CONFIG_VALE_MAXLEN]={0};

	get_config(configValue, "NonProc_ESSID", "/mnt/jffs2/wlan0.conf");	

	system("sed -i '/Wildcard_ESSID/d' /mnt/jffs2/wlan0.conf");
	sprintf(command, "echo \"Wildcard_ESSID = %s\" >> /mnt/jffs2/wlan0.conf", configValue);
	printf("Command for recover wildcard ssid = %s\n", command);
	system(command);

	system("/bin/cp /mnt/jffs2/wlan0.conf /tmp/wlan0.conf");

	system("/bin/config_umount.sh");
	system("/bin/config_mount.sh");

	return 0;
}*/

int main(int argc, char *argv[])
{
	struct stat statBuf;
	FILE *fptr;
	char buf[128]={0}, *bufptr;
	int result;
	int wps_status=0;
	int delay_exit;
	int retry=0, i=0;
	int last_status=0;

	struct sigaction sa;   
	memset(&sa, 0, sizeof(sa));   
    	sa.sa_handler = &signal_handler;   
    	sigaction(SIGUSR1, &sa, NULL);   

	printf("Start wps monitor thread..\n");
	sprintf(buf, "echo %d > /var/run/hnap_wps_status_monitor", getpid());
	system(buf);
	system("echo 1 > /var/wpsRunning");

	//waitting WPS is running...
	while(1){
		//printf(">>>>>>>>>> %d\n", retry);
		result = stat("/tmp/wps_current_status", &statBuf);
		if(result != -1){
			break;
		}
		sleep(1);
		retry++;
		if(retry>60){
			printf("ERROR: Can not found WPS progress be started..\n");
			return 0;
		}
		if(gotSignal){
			goto End;
		}		
	}

	while(1){
		if(gotSignal){
			goto End;
		}
		result = stat("/tmp/wps_current_status", &statBuf);
		if(result != -1){
			retry=0;
			fptr = popen("cat /tmp/wps_current_status", "r");
			if(fptr==NULL){
				sleep(1);
				retry++;
				if(retry>=5){
					system("rm /var/wpsRunning");
					printf("EXIT.., because /tmp/wps_current_status can not be open\n");
					break;
				}else{
					printf("%dth can not open /tmp/wps_current_status..\n", retry);
					continue;
				}
			}
			memset(buf, 0, 128);
			if(fgets(buf, 128, fptr)!=NULL){
				pclose(fptr);
				//printf("The value of wps_current_status is \"%s\"\n", buf);
				bufptr = strchr(buf, '=');
				bufptr = bufptr+1;
				wps_status = atoi(bufptr);
				expose_wps_status(wps_status);
				printf("Status Code = %d\n", wps_status); //Ricky Trace				
				if(wps_status==3){
					//expose_wps_status(wps_status);
					system("rm /var/wpsRunning");
					break;
				}else if(wps_status==10 || wps_status==11 || wps_status==12 || wps_status==13){
					//When status is ERROR, I stop to monitor status
					system("rm /var/wpsRunning");
					//recover_wildcardssid();
					printf("Got WPS ERROR event %d .. \n", wps_status);
					break;
				}
			}
			if(fptr!=NULL){
				pclose(fptr);
			}

			/*result = stat("/tmp/wps_last_code", &statBuf);
			if(result == -1){
				if(errno==2){
					break;
				}
			}*/
		}else{
			retry++;
			if(retry>10){
				system("rm /var/wpsRunning");
				printf("EXIT.., because /tmp/wps_current_status can not be found\n");
				break;
			}else{
				printf("%dth can not found /tmp/wps_current_status..\n", retry);
				sleep(3);
				continue;
			}
		}

		sleep(1);
	}

	if(wps_status==99 || wps_status==10 || wps_status==11 || 
		wps_status==12 || wps_status==13){
		//If latest status is CONNECTED or ERROR, 
		//delay a while for give a opportunity to HNAP client to got it 
		delay_exit=wps_status!=99?120:60;
		while(1){
			if(gotSignal){
				goto End;
			}
		
			printf("%d\n", delay_exit);
			delay_exit--;
			if(!delay_exit)
				break;
			sleep(1);
		}
	}else if(wps_status==3){
		delay_exit=60; //Waitting 60 seconds for connect to AP
		while(1){
			if(gotSignal){
				goto End;
			}
			
			if(wps_status==3 && get_station_link_status()){
				wps_status = 99;
				expose_wps_status(wps_status);
				//delay a while for give a opportunity to HNAP client to got CONNECTED status 	
				delay_exit=60;
			}
			printf("%d\n", delay_exit);
			delay_exit--;
			if(!delay_exit)
				break;
			sleep(1);

		}
		delay_exit=60; //Waitting 60 seconds for indicate fail connect to AP after WPS successful
		printf("Fail connect to AP within 60 seconds after WPS successful\n");
		wps_status = 14;
		expose_wps_status(wps_status);
		while(1){
			if(gotSignal){
				goto End;
			}

			printf("%d\n", delay_exit);
			delay_exit--;
			if(!delay_exit)
				break;
			sleep(1);

		}
	}else if(wps_status==1 || wps_status==2){
		printf("EXIT in SEARCHING or REGISTERING..\n");
		retry=0;
		while(1){
			if(gotSignal){
				goto End;
			}			
			sleep(5);
			result = stat("/tmp/wps_last_code", &statBuf);
			if(result==-1){
				retry++;
				if(retry>10){
					printf("EXIT.., because /tmp/wps_last_code can not be found\n");
					break;
				}else{
					printf("%dth can not found /tmp/wps_last_code..\n", retry);
					continue;
				}
			}else{
				retry=0;
				fptr = popen("cat /tmp/wps_last_code", "r");
				if(fptr!=NULL){
					memset(buf, 0, 128);
					if(fgets(buf, 128, fptr)!=-1){
						//printf("value in wps_current_status = %s\n", buf);
						bufptr = strchr(buf, '=');
						bufptr = bufptr+1;
						last_status=wps_status;
						wps_status = atoi(bufptr);
						printf("last_status=%d, wps_status=%d\n", last_status, wps_status);
						if(wps_status==3 || wps_status==4){
							if(wps_status==3){
								printf("got a SESSION OVERLAP event ..\n");
								wps_status=12;
								expose_wps_status(wps_status);
								//recover_wildcardssid();
							}else if(last_status==2 && wps_status==4){
								printf("got ERROR_SESSION_TIMEOUT ..\n");
								wps_status=11;
								expose_wps_status(wps_status);
								//recover_wildcardssid();
							}else if(last_status==1 && wps_status==4){
								printf("got ERROR_WALK_TIMEOUT event ..\n");
								wps_status=10;
								expose_wps_status(wps_status);
								//recover_wildcardssid();
							}
							
							delay_exit=120;
							while(1){
								if(gotSignal){
									goto End;
								}											
								printf("%d\n", delay_exit);
								delay_exit--;
								if(!delay_exit)
									break;
								sleep(1);
							}
						}
					}
					pclose(fptr);
				}
				break;
			}	
		}
	}

End:
	if(fptr!=NULL)
		pclose(fptr);
	
	wps_status = 0;
	expose_wps_status(wps_status);

	system("rm /tmp/wps_last_code");
	result = stat("/var/wpsRunning", &statBuf);
	if(result!=-1){
		system("rm /var/wpsRunning");
	}
	system("rm /var/run/hnap_wps_status_monitor");
	printf("Terminate WPS monitor thread..\n");
	
	return 0;
}

