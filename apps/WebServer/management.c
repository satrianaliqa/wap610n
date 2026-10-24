#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "uemf.h"
#include "wsIntrn.h"
#include "mt_api.h"
#include "utils.h"
#include "management.h"

extern int waitOtherProcess;

void childFinishAndExit(void)
{
	waitOtherProcess = 0;
	unlink("/var/old_web_pid");
	kill(getppid(), SIGHUP);
	_exit(0);
}

void resetToDefault(webs_t wp, char_t *path, char_t *query)
{
	char filePath[32];
	char line[64];
	char projectName[16];
	char_t *restoreTag;
	FILE *fp;
	char *p;
	char *start;
	char *end;

	if (checkLoadDefault() == 1) {
		websRedirect(wp, T("/admin/upgrade.asp"));
		return;
	}

	strcpy(filePath, "/root/mtlk/web/fw_version.txt");
	memset(projectName, 0, sizeof(projectName));
	restoreTag = websGetVar(wp, T("restoreTag"), T(""));

	fp = fopen(filePath, "r");
	if (fp == NULL) {
		printf("Can't open %s file\n", filePath);
		return;
	}

	while (fgets(line, 64, fp) != NULL) {
		p = strstr(line, "ProjectName=");
		if (p != NULL) {
			start = p + 13;
			end = strchr(p + 14, '"');
			if (end != NULL) {
				memset(projectName, 0, 16);
				strncpy(projectName, start, end - start);
			}
		}
	}
	fclose(fp);

	if (strcmp(restoreTag, "AP") == 0) {
		system("echo killconfig > /dev/mtdblock4");
	} else if (strcmp(restoreTag, "STA") == 0) {
		if (strcmp(projectName, "WET610N") == 0 || strcmp(projectName, "WAP610N") == 0) {
			printf("ProjectName:%s\n", projectName);
		} else {
			printf("ProjectName:%s\n", "NULL");
		}
		system("echo killconfig > /dev/mtdblock4");
	}

	if (vfork() != 0) {
		debug_printf("resetToDefault: 1. pid=%d\n", getpid());
		websRedirect(wp, T("/reboot_page.asp"));
	} else {
		debug_printf("resetToDefault: 2. pid=%d\n", getpid());
		waitRedirectPage();
		system("echo 2 > /dev/gpio2");
		system("sleep 3 ; reboot&");
		_exit(0);
	}
}

void management(webs_t wp, char_t *path, char_t *query)
{
	char_t *rebootTag;
	char_t *paramNames[3];
	char tmp[512];
	char_t *admPass;
	char_t *timeOut;
	char_t *wirelessMgmt;

	if (checkLoadDefault() == 1) {
		websRedirect(wp, T("/admin/upgrade.asp"));
		return;
	}

	rebootTag = websGetVar(wp, T("rebootTag"), T(""));
	if (strcmp(rebootTag, "true") == 0) {
		saveOldWebPid(getpid(), "management.asp");
		if (vfork() != 0) {
			debug_printf("management(true): 1. pid=%d\n", getpid());
			websRedirect(wp, T("/wait_page.asp"));
			doSystem("/root/mtlk/web/killchildweb.sh&");
			waitOtherProcess = 1;
		} else {
			debug_printf("management(true): 2. pid=%d\n", getpid());
			waitRedirectPage();
			stopWPS_PBC();
			memset(tmp, 0, sizeof(tmp));
			paramNames[0] = "AdminPassword";
			paramNames[1] = "AuthenticationTimeout";
			paramNames[2] = "WirelessMgmtEnabled";

			admPass = websGetVar(wp, T("submit_admPassword"), T(""));
			timeOut = websGetVar(wp, T("timeOut"), T(""));
			wirelessMgmt = websGetVar(wp, T("wirelessMgmt"), T(""));

			snprintf(tmp, 512, "%s", admPass);
			MT_Set_Param("AdminPassword", tmp, 1);
			MT_Set_Param("AuthenticationTimeout", timeOut, 1);
			MT_Set_Param("WirelessMgmtEnabled", wirelessMgmt, 1);
			MT_WriteConfFiles(3, paramNames);
			submitToFlash();
			startWPS_PBC();
			childFinishAndExit();
		}
	} else if (strcmp(rebootTag, "false") == 0) {
		if (vfork() != 0) {
			debug_printf("management(false): 1. pid=%d\n", getpid());
			websRedirect(wp, T("/reboot_page.asp"));
		} else {
			debug_printf("management(false): 1. pid=%d\n", getpid());
			system("echo 2 > /dev/gpio2");
			system("sleep 8 ; reboot&");
			_exit(0);
		}
	}
}

void updateBurnProgress(webs_t wp, char_t *path, char_t *query)
{
	char stateStr[8];
	char progressStr[8];
	char errStr[512];
	char_t *err;

	debug_printf("updateBurnProgress: Into updateBurnProgress function.\n");
	memset(stateStr, 0, sizeof(stateStr));
	strcpy(stateStr, WebImageUpdateState());

	memset(progressStr, 0, sizeof(progressStr));
	strcpy(progressStr, WebImageUpdateProgress());

	err = WebImageUpdateErr();
	memset(errStr, 0, 512);
	strcpy(errStr, err);

	websWrite(wp, T("HTTP/1.1 200 OK\nContent-type: text/plain\nPragma: no-cache\n\n"));
	websWrite(wp, T("%s,%s"), stateStr, progressStr);
	websDone(wp, 200);
}

int checkHW(int eid, webs_t wp, int argc, char_t **argv)
{
	char hwName[16];
	char buf[1024];
	FILE *fp;

	memset(hwName, 0, 12);
	fp = popen("echo \"read_phy\" >> /proc/str9100/reg_debug ; cat /proc/str9100/reg_debug", "r");
	if (fp != NULL) {
		fgets(buf, 1024, fp);
		fgets(buf, 1024, fp);
		pclose(fp);
		if (strstr(buf, "reg3=0xc852") != NULL) {
			strcpy(hwName, "Linksys WES610N");
		} else {
			strcpy(hwName, "Linksys WET610N");
		}
	} else {
		strcpy(hwName, "Linksys WET610N");
	}
	return websWrite(wp, T("%s"), hwName);
}

void formDefineManagement(void)
{
	websAspDefine(T("checkHW"), checkHW);
	websFormDefine(T("management"), management);
	websFormDefine(T("resetToDefault"), resetToDefault);
	websFormDefine(T("updateBurnProgress"), updateBurnProgress);
}
