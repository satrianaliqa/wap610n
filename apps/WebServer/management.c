#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <crypt.h>
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

void syncRootShadowPassword(const char *plainPass)
{
	static const char saltChars[] = "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
	char line[256];
	char existingHash[128];
	char salt[16];
	char *hashed;
	const char *tailFields;
	FILE *inFp;
	FILE *outFp;
	FILE *rndFp;
	int outFd;
	int replaced;
	int i;
	unsigned char rndBuf[8];
	size_t passLen;

	if (plainPass == NULL || plainPass[0] == '\0') {
		return;
	}
	passLen = strlen(plainPass);
	if (passLen > 64 || strpbrk(plainPass, ":\r\n") != NULL) {
		return;
	}

	existingHash[0] = '\0';
	tailFields = ":10933:0:99999:7:::";
	inFp = fopen("/etc/shadow", "r");
	if (inFp != NULL) {
		while (fgets(line, sizeof(line), inFp) != NULL) {
			if (strncmp(line, "root:", 5) == 0) {
				char *p1 = line + 5;
				char *p2 = strchr(p1, ':');
				size_t hlen;
				if (p2 != NULL) {
					hlen = (size_t)(p2 - p1);
					if (hlen > 0 && hlen < sizeof(existingHash)) {
						memcpy(existingHash, p1, hlen);
						existingHash[hlen] = '\0';
					}
				}
				break;
			}
		}
		fclose(inFp);
	}

	if (existingHash[0] == '$') {
		char *check = crypt(plainPass, existingHash);
		if (check != NULL && strcmp(check, existingHash) == 0) {
			return;
		}
	}

	memset(rndBuf, 0, sizeof(rndBuf));
	rndFp = fopen("/dev/urandom", "rb");
	if (rndFp != NULL) {
		if (fread(rndBuf, 1, sizeof(rndBuf), rndFp) != sizeof(rndBuf)) {
			rndBuf[0] = '\0';
		}
		fclose(rndFp);
	}
	if (rndBuf[0] == '\0') {
		unsigned long seed = (unsigned long)time(NULL) ^ ((unsigned long)getpid() << 16);
		for (i = 0; i < 8; i++) {
			seed = seed * 1103515245UL + 12345UL;
			rndBuf[i] = (unsigned char)((seed >> 16) & 0xFFU);
		}
	}

	salt[0] = '$';
	salt[1] = '1';
	salt[2] = '$';
	for (i = 0; i < 8; i++) {
		salt[3 + i] = saltChars[rndBuf[i] % (sizeof(saltChars) - 1)];
	}
	salt[11] = '$';
	salt[12] = '\0';

	hashed = crypt(plainPass, salt);
	if (hashed == NULL || hashed[0] != '$') {
		return;
	}

	outFd = open("/etc/shadow.tmp", O_WRONLY | O_CREAT | O_TRUNC, 0600);
	if (outFd < 0) {
		return;
	}
	outFp = fdopen(outFd, "w");
	if (outFp == NULL) {
		close(outFd);
		unlink("/etc/shadow.tmp");
		return;
	}

	replaced = 0;
	inFp = fopen("/etc/shadow", "r");
	if (inFp != NULL) {
		while (fgets(line, sizeof(line), inFp) != NULL) {
			if (!replaced && strncmp(line, "root:", 5) == 0) {
				char *p1 = line + 5;
				char *p2 = strchr(p1, ':');
				if (p2 != NULL) {
					size_t tlen = strlen(p2);
					while (tlen > 0 && (p2[tlen - 1] == '\n' || p2[tlen - 1] == '\r')) {
						p2[--tlen] = '\0';
					}
					fprintf(outFp, "root:%s%s\n", hashed, p2);
				} else {
					fprintf(outFp, "root:%s%s\n", hashed, tailFields);
				}
				replaced = 1;
			} else {
				fputs(line, outFp);
			}
		}
		fclose(inFp);
	}
	if (!replaced) {
		fprintf(outFp, "root:%s%s\n", hashed, tailFields);
	}

	fflush(outFp);
	fsync(fileno(outFp));
	fclose(outFp);
	chmod("/etc/shadow.tmp", 0600);
	if (rename("/etc/shadow.tmp", "/etc/shadow") != 0) {
		unlink("/etc/shadow.tmp");
	} else {
		chmod("/etc/shadow", 0600);
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

static int readSysConfValue(const char *path, const char *key, char *out, size_t outSize)
{
	FILE *fp;
	char line[256];
	size_t keyLen;
	int found = 0;

	if (path == NULL || key == NULL || out == NULL || outSize == 0) {
		return 0;
	}
	keyLen = strlen(key);
	fp = fopen(path, "r");
	if (fp == NULL) {
		return 0;
	}
	while (fgets(line, sizeof(line), fp) != NULL) {
		char *p = line;
		while (*p == ' ' || *p == '\t') {
			p++;
		}
		if (strncmp(p, key, keyLen) == 0) {
			char *eq = p + keyLen;
			while (*eq == ' ' || *eq == '\t') {
				eq++;
			}
			if (*eq == '=') {
				char *val = eq + 1;
				size_t len;
				while (*val == ' ' || *val == '\t') {
					val++;
				}
				snprintf(out, outSize, "%s", val);
				len = strlen(out);
				while (len > 0 && (out[len - 1] == '\n' || out[len - 1] == '\r' ||
				                   out[len - 1] == ' ' || out[len - 1] == '\t')) {
					out[--len] = '\0';
				}
				found = 1;
			}
		}
	}
	fclose(fp);
	return found;
}

void websPreserveCustomSysConf(const char *wirelessMgmtOverride)
{
	static const char * const keys[7] = {
		"SSH_Enabled",
		"Telnet_Enabled",
		"WirelessMgmtEnabled",
		"UPnP_Enabled",
		"SSHJ_Enabled",
		"SSHJ_User",
		"SSHJ_Host"
	};
	static const char * const defaults[7] = {
		"1",
		"1",
		"1",
		"0",
		"0",
		"",
		"wap610n"
	};
	char vals[7][64];
	int i;
	FILE *fp;

	for (i = 0; i < 7; i++) {
		if (i == 2 && wirelessMgmtOverride != NULL && wirelessMgmtOverride[0] != '\0') {
			snprintf(vals[i], sizeof(vals[i]), "%s", wirelessMgmtOverride);
		} else if (!readSysConfValue("/tmp/sys.conf", keys[i], vals[i], sizeof(vals[i]))) {
			snprintf(vals[i], sizeof(vals[i]), "%s", defaults[i]);
		}
	}

	fp = fopen("/mnt/jffs2/sys.conf", "a");
	if (fp != NULL) {
		for (i = 0; i < 7; i++) {
			char existing[64];
			if (!readSysConfValue("/mnt/jffs2/sys.conf", keys[i], existing, sizeof(existing))) {
				fprintf(fp, "%s = %s\n", keys[i], vals[i]);
			}
		}
		fclose(fp);
	}
}

static void preserveCustomSysConf(const char *wirelessMgmtOverride)
{
	websPreserveCustomSysConf(wirelessMgmtOverride);
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
				syncRootShadowPassword(tmp);
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
			preserveCustomSysConf(wirelessMgmt);
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
