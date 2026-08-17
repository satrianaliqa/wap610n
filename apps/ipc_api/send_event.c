/*
 * sysd.c - Sysetm Daemon
 * Jacky.Yang 29-Nov-2007
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>

#define FIREWALL_RELOAD_KEY	9999

int main (int argc, char *argv[])
{
	int firewallReloadID;
	int msgSize=1024;
	
	struct msgbuffer {
		long mtype;
		char mtext[msgSize];
	}msgInfo;
	firewallReloadID = msgget(FIREWALL_RELOAD_KEY, IPC_CREAT | 0666);
	//firewallReloadID = msgget(FIREWALL_RELOAD_KEY, IPC_PRIVATE | 0666);
	
	if(firewallReloadID == -1){
		printf("send_event: Create firewall reload message queue fail!\n");
		perror("msgget");
	}
	
	if (argc == 2) {
		//printf("jacky - argc=%d\n", argc);
		if (!strcmp(argv[1], "firewall_reload"))
		{
			printf("send_event: Sent firewall reload message to goahead!\n");
			msgInfo.mtype = 1;
			memset(msgInfo.mtext, '\0', msgSize);
			strcpy(msgInfo.mtext, "firewall_reload");
			msgsnd(firewallReloadID, &msgInfo, msgSize, IPC_NOWAIT);
		}
		else if (!strcmp(argv[1], "openDebug"))
		{
			printf("send_event: Sent open debug message to goahead!\n");
			msgInfo.mtype = 1;
			memset(msgInfo.mtext, '\0', msgSize);
			strcpy(msgInfo.mtext, "openDebug");
			msgsnd(firewallReloadID, &msgInfo, msgSize, IPC_NOWAIT);
		}
		else if (!strcmp(argv[1], "closeDebug"))
		{
			printf("send_event: Sent close debug message to goahead!\n");
			msgInfo.mtype = 1;
			memset(msgInfo.mtext, '\0', msgSize);
			strcpy(msgInfo.mtext, "closeDebug");
			msgsnd(firewallReloadID, &msgInfo, msgSize, IPC_NOWAIT);
		}
		else if (!strcmp(argv[1], "stopWPS"))
		{
			printf("send_event: Sent Stop WPS process message to goahead!\n");
			msgInfo.mtype = 1;
			memset(msgInfo.mtext, '\0', msgSize);
			strcpy(msgInfo.mtext, "stopWPS");
			msgsnd(firewallReloadID, &msgInfo, msgSize, IPC_NOWAIT);
		}
	}
	else if (argc == 3) {
		if (!strcmp(argv[1], "WNLAC"))
		{
			if (!strcmp(argv[2], "5G"))
			{
				printf("send_event: Sent WNLAC switch to 5G message to goahead!\n");
				msgInfo.mtype = 1;
				memset(msgInfo.mtext, '\0', msgSize);
				strcpy(msgInfo.mtext, "WNLAC_5G");
				msgsnd(firewallReloadID, &msgInfo, msgSize, IPC_NOWAIT);
			}
			else if (!strcmp(argv[2], "2.4G"))
			{
				printf("send_event: Sent WNLAC switch to 2.4G message to goahead!\n");
				msgInfo.mtype = 1;
				memset(msgInfo.mtext, '\0', msgSize);
				strcpy(msgInfo.mtext, "WNLAC_2.4G");
				msgsnd(firewallReloadID, &msgInfo, msgSize, IPC_NOWAIT);
			}
		}
	}
	else if (argc == 5){
		/* Jacky.Yang 28-Mar-2008, 
 		 * IPC command: send_event <start/stop> <nvram tag name> <rule name> <firewall tag>
		 * IPC command: send_event start SinglePortForwardRules jacky222 SinglePort_
		 * IPC command: send_event stop SinglePortForwardRules jacky222 SinglePort_
		 */
		//printf("jacky - argc=%d\n", argc);
		if (!strcmp(argv[1], "start") || !strcmp(argv[1], "stop")) {
			printf("send_event: Sent schedule message to goahead!\n");
			msgInfo.mtype = 1;
			memset(msgInfo.mtext, '\0', msgSize);
			snprintf(msgInfo.mtext, msgSize, "%s,%s,%s,%s", argv[1], argv[2], argv[3], argv[4]);
			msgsnd(firewallReloadID, &msgInfo, msgSize, 0);
		}
	}
}