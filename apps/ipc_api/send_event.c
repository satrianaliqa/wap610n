/*
 * send_event.c - System Daemon IPC Event Sender
 * Jacky.Yang 29-Nov-2007
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>

#define FIREWALL_RELOAD_KEY	9999
#define MSG_SIZE		1024

int main(int argc, char *argv[])
{
	int firewallReloadID;
	int send_rc = 0;

	struct msgbuffer {
		long mtype;
		char mtext[MSG_SIZE];
	} msgInfo;

	firewallReloadID = msgget(FIREWALL_RELOAD_KEY, IPC_CREAT | 0600);
	if (firewallReloadID == -1) {
		printf("send_event: Create firewall reload message queue fail!\n");
		perror("msgget");
		return 1;
	}

	if (argc == 2) {
		if (!strcmp(argv[1], "firewall_reload")) {
			printf("send_event: Sent firewall reload message to goahead!\n");
			msgInfo.mtype = 1;
			memset(msgInfo.mtext, '\0', MSG_SIZE);
			snprintf(msgInfo.mtext, MSG_SIZE, "firewall_reload");
			send_rc = msgsnd(firewallReloadID, &msgInfo, MSG_SIZE, IPC_NOWAIT);
		} else if (!strcmp(argv[1], "openDebug")) {
			printf("send_event: Sent open debug message to goahead!\n");
			msgInfo.mtype = 1;
			memset(msgInfo.mtext, '\0', MSG_SIZE);
			snprintf(msgInfo.mtext, MSG_SIZE, "openDebug");
			send_rc = msgsnd(firewallReloadID, &msgInfo, MSG_SIZE, IPC_NOWAIT);
		} else if (!strcmp(argv[1], "closeDebug")) {
			printf("send_event: Sent close debug message to goahead!\n");
			msgInfo.mtype = 1;
			memset(msgInfo.mtext, '\0', MSG_SIZE);
			snprintf(msgInfo.mtext, MSG_SIZE, "closeDebug");
			send_rc = msgsnd(firewallReloadID, &msgInfo, MSG_SIZE, IPC_NOWAIT);
		} else if (!strcmp(argv[1], "stopWPS")) {
			printf("send_event: Sent Stop WPS process message to goahead!\n");
			msgInfo.mtype = 1;
			memset(msgInfo.mtext, '\0', MSG_SIZE);
			snprintf(msgInfo.mtext, MSG_SIZE, "stopWPS");
			send_rc = msgsnd(firewallReloadID, &msgInfo, MSG_SIZE, IPC_NOWAIT);
		}
	} else if (argc == 3) {
		if (!strcmp(argv[1], "WNLAC")) {
			if (!strcmp(argv[2], "5G")) {
				printf("send_event: Sent WNLAC switch to 5G message to goahead!\n");
				msgInfo.mtype = 1;
				memset(msgInfo.mtext, '\0', MSG_SIZE);
				snprintf(msgInfo.mtext, MSG_SIZE, "WNLAC_5G");
				send_rc = msgsnd(firewallReloadID, &msgInfo, MSG_SIZE, IPC_NOWAIT);
			} else if (!strcmp(argv[2], "2.4G")) {
				printf("send_event: Sent WNLAC switch to 2.4G message to goahead!\n");
				msgInfo.mtype = 1;
				memset(msgInfo.mtext, '\0', MSG_SIZE);
				snprintf(msgInfo.mtext, MSG_SIZE, "WNLAC_2.4G");
				send_rc = msgsnd(firewallReloadID, &msgInfo, MSG_SIZE, IPC_NOWAIT);
			}
		}
	} else if (argc == 5) {
		if (!strcmp(argv[1], "start") || !strcmp(argv[1], "stop")) {
			printf("send_event: Sent schedule message to goahead!\n");
			msgInfo.mtype = 1;
			memset(msgInfo.mtext, '\0', MSG_SIZE);
			snprintf(msgInfo.mtext, MSG_SIZE, "%s,%s,%s,%s", argv[1], argv[2], argv[3], argv[4]);
			send_rc = msgsnd(firewallReloadID, &msgInfo, MSG_SIZE, IPC_NOWAIT);
		}
	}

	if (send_rc == -1) {
		perror("msgsnd");
		return 1;
	}
	return 0;
}
