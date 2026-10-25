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
	int paramCount = 0;

	if (checkLoadDefault() == 1) {
		websRedirect(wp, T("/admin/upgrade.asp"));
		return;
	}

	rebootTag = websGetVar(wp, T("rebootTag"), T(""));
	if (rebootTag != NULL && strcmp(rebootTag, "true") == 0) {
		if (vfork() != 0) {
			debug_printf("management(reboot): 1. pid=%d\n", getpid());
			websRedirect(wp, T("/reboot_page.asp"));
		} else {
			debug_printf("management(reboot): 2. pid=%d\n", getpid());
			system("echo 2 > /dev/gpio2");
			system("sleep 8 ; reboot&");
			_exit(0);
		}
	} else {
		/* When rebootTag is empty or "false", save settings */
		saveOldWebPid(getpid(), "management.asp");
		if (vfork() != 0) {
			debug_printf("management(save): 1. pid=%d\n", getpid());
			websRedirect(wp, T("/wait_page.asp"));
			doSystem("/root/mtlk/web/killchildweb.sh&");
			waitOtherProcess = 1;
		} else {
			debug_printf("management(save): 2. pid=%d\n", getpid());
			waitRedirectPage();
			stopWPS_PBC();
			memset(tmp, 0, sizeof(tmp));

			admPass = websGetVar(wp, T("submit_admPassword"), T(""));
			timeOut = websGetVar(wp, T("timeOut"), T(""));
			wirelessMgmt = websGetVar(wp, T("wirelessMgmt"), T(""));

			if (admPass != NULL && admPass[0] != '\0') {
				snprintf(tmp, sizeof(tmp), "%s", admPass);
				MT_Set_Param("AdminPassword", tmp, 1);
				paramNames[paramCount++] = "AdminPassword";
			}
			if (timeOut != NULL && timeOut[0] != '\0') {
				MT_Set_Param("AuthenticationTimeout", timeOut, 1);
				paramNames[paramCount++] = "AuthenticationTimeout";
			}
			if (wirelessMgmt != NULL && wirelessMgmt[0] != '\0') {
				MT_Set_Param("WirelessMgmtEnabled", wirelessMgmt, 1);
				paramNames[paramCount++] = "WirelessMgmtEnabled";
			}
			if (paramCount > 0) {
				MT_WriteConfFiles(paramCount, paramNames);
			}
			submitToFlash();
			startWPS_PBC();
			childFinishAndExit();
		}
	}
}

void updateBurnProgress(webs_t wp, char_t *path, char_t *query)
{
	char stateStr[8];
	char progressStr[8];
	char errStr[512];
	char resp[64];
	char_t *err;

	debug_printf("updateBurnProgress: Into updateBurnProgress function.\n");
	memset(stateStr, 0, sizeof(stateStr));
	strcpy(stateStr, WebImageUpdateState());

	memset(progressStr, 0, sizeof(progressStr));
	strcpy(progressStr, WebImageUpdateProgress());

	err = WebImageUpdateErr();
	memset(errStr, 0, 512);
	if (err) strcpy(errStr, err);

	snprintf(resp, sizeof(resp), "%s,%s", stateStr, progressStr);
	wp->flags &= ~WEBS_KEEP_ALIVE;
	websWrite(wp, T("HTTP/1.0 200 OK\r\n"));
	websWrite(wp, T("Content-Type: text/plain\r\n"));
	websWrite(wp, T("Connection: close\r\n"));
	websWrite(wp, T("Pragma: no-cache\r\n"));
	websWrite(wp, T("Cache-Control: no-cache\r\n"));
	websWrite(wp, T("Content-Length: %d\r\n\r\n"), (int)strlen(resp));
	websWrite(wp, T("%s"), resp);
	websDone(wp, 200);
}

int checkHW(int eid, webs_t wp, int argc, char_t **argv)
{
	/* Model name for WAP610N; eliminate failing popen subshell */
	return websWrite(wp, T("WAP610N"));
}

void formDefineManagement(void)
{
	websAspDefine(T("checkHW"), checkHW);
	websFormDefine(T("management"), management);
	websFormDefine(T("resetToDefault"), resetToDefault);
	websFormDefine(T("updateBurnProgress"), updateBurnProgress);
}
