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

static void closeInheritedFds(void)
{
	int fd;

	for (fd = 3; fd < 256; fd++) {
		close(fd);
	}
}

void childFinishAndExit(void)
{
	pid_t ppid = getppid();

	waitOtherProcess = 0;
	unlink("/var/old_web_pid");
	if (ppid > 1) {
		kill(ppid, SIGHUP);
	}
	_exit(0);
}

void resetToDefault(webs_t wp, char_t *path, char_t *query)
{
	char_t *restoreTag;
	pid_t pid;

	if (checkLoadDefault() == 1) {
		websRedirect(wp, T("/admin/upgrade.asp"));
		return;
	}

	if (waitOtherProcess != 0) {
		websError(wp, 503, T("Another configuration operation is in progress"));
		return;
	}

	restoreTag = websGetVar(wp, T("restoreTag"), T(""));

	if (strcmp(restoreTag, "AP") != 0) {
		websError(wp, 400, T("Invalid restore mode"));
		return;
	}

	waitOtherProcess = 1;
	pid = fork();
	if (pid < 0) {
		waitOtherProcess = 0;
		websError(wp, 500, T("Unable to start factory reset"));
		return;
	} else if (pid != 0) {
		debug_printf("resetToDefault: 1. pid=%d\n", getpid());
		websRedirect(wp, T("/reboot_page.asp"));
		return;
	} else {
		closeInheritedFds();
		debug_printf("resetToDefault: 2. pid=%d\n", getpid());
		waitRedirectPage();
		if (system("/root/mtlk/etc/mtlk_restore_defaults.sh 1 0 > /dev/null 2>&1") != 0) {
			debug_printf("resetToDefault: factory reset failed\n");
			if (getppid() > 1) {
				kill(getppid(), SIGHUP);
			}
			_exit(1);
		}
		system("echo 2 > /dev/gpio2");
		system("sync");
		system("sleep 3 ; reboot&");
		_exit(0);
	}
}

static int isValidDigits(const char *s)
{
	const char *p;

	if (s == NULL || *s == '\0') {
		return 0;
	}
	for (p = s; *p != '\0'; p++) {
		if (*p < '0' || *p > '9') {
			return 0;
		}
	}
	return 1;
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
	long tval;
	pid_t pid;

	if (checkLoadDefault() == 1) {
		websRedirect(wp, T("/admin/upgrade.asp"));
		return;
	}

	if (waitOtherProcess != 0) {
		websError(wp, 503, T("Another configuration operation is in progress"));
		return;
	}

	rebootTag = websGetVar(wp, T("rebootTag"), T(""));
	if (rebootTag != NULL && strcmp(rebootTag, "true") == 0) {
		waitOtherProcess = 1;
		pid = fork();
		if (pid < 0) {
			waitOtherProcess = 0;
			websError(wp, 500, T("Unable to start reboot"));
			return;
		} else if (pid != 0) {
			debug_printf("management(reboot): 1. pid=%d\n", getpid());
			websRedirect(wp, T("/reboot_page.asp"));
			return;
		} else {
			closeInheritedFds();
			debug_printf("management(reboot): 2. pid=%d\n", getpid());
			system("echo 2 > /dev/gpio2");
			system("sync");
			system("sleep 8 ; reboot&");
			_exit(0);
		}
	} else {
		/* Validate parameters before forking background apply */
		admPass = websGetVar(wp, T("submit_admPassword"), T(""));
		timeOut = websGetVar(wp, T("timeOut"), T(""));
		wirelessMgmt = websGetVar(wp, T("wirelessMgmt"), T(""));

		if (admPass != NULL && admPass[0] != '\0') {
			if (strlen(admPass) > 64 || strpbrk(admPass, "\r\n") != NULL) {
				websError(wp, 400, T("Invalid administrator password"));
				return;
			}
		}
		if (timeOut != NULL && timeOut[0] != '\0') {
			if (!isValidDigits(timeOut)) {
				websError(wp, 400, T("Invalid authentication timeout"));
				return;
			}
			tval = atol(timeOut);
			if (tval < 0 || tval > 86400) {
				websError(wp, 400, T("Authentication timeout out of range"));
				return;
			}
		}
		if (wirelessMgmt != NULL && wirelessMgmt[0] != '\0') {
			if (strcmp(wirelessMgmt, "0") != 0 && strcmp(wirelessMgmt, "1") != 0) {
				websError(wp, 400, T("Invalid wireless management setting"));
				return;
			}
		}

		/* When rebootTag is empty or "false", save settings */
		saveOldWebPid(getpid(), "management.asp");
		waitOtherProcess = 1;
		pid = fork();
		if (pid < 0) {
			waitOtherProcess = 0;
			unlink("/var/old_web_pid");
			websError(wp, 500, T("Unable to save settings"));
			return;
		} else if (pid != 0) {
			debug_printf("management(save): 1. pid=%d\n", getpid());
			websRedirect(wp, T("/wait_page.asp"));
			return;
		} else {
			closeInheritedFds();
			debug_printf("management(save): 2. pid=%d\n", getpid());
			waitRedirectPage();
			stopWPS_PBC();
			memset(tmp, 0, sizeof(tmp));

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
	char stateStr[16];
	char progressStr[16];
	char resp[64];
	char_t *st;
	char_t *prog;

	debug_printf("updateBurnProgress: Into updateBurnProgress function.\n");
	memset(stateStr, 0, sizeof(stateStr));
	st = WebImageUpdateState();
	snprintf(stateStr, sizeof(stateStr), "%s", st ? st : "0");

	memset(progressStr, 0, sizeof(progressStr));
	prog = WebImageUpdateProgress();
	snprintf(progressStr, sizeof(progressStr), "%s", prog ? prog : "0");

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
