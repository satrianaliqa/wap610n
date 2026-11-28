/*
 * main.c -- Main program for the GoAhead WebServer (LINUX version)
 *
 * Copyright (c) GoAhead Software Inc., 1995-2000. All Rights Reserved.
 *
 * See the file "license.txt" for usage and redistribution license requirements
 *
 * $Id: main.c,v 1.5 2003/09/11 14:03:46 bporter Exp $
 */

/******************************** Description *********************************/

/*
 *	Main program for for the GoAhead WebServer. This is a demonstration
 *	main program to initialize and configure the web server.
 */

/********************************* Includes ***********************************/

#include	"../uemf.h"
#include	"../wsIntrn.h"
#include	<stdarg.h>
#include	<signal.h>
#include	<unistd.h> 
#include	<sys/types.h>
#include	<sys/wait.h>
#include	<sys/ioctl.h>
#include	<net/if.h>
#include	<fcntl.h>
#include	<sys/stat.h>
#include	<sys/vfs.h>
#include	<time.h>
#include	<dirent.h>
#include	<ctype.h>
#include	"../mt_validation.h"
#include	"../mt_task.h"
#include	"../wireless.h"
#include	"../wps.h"
#include	"../management.h"
#include	"../utils.h"

#include <sys/ipc.h>
#include <sys/msg.h>

#ifdef WEBS_SSL_SUPPORT
#include	"../websSSL.h"
#endif

#ifdef USER_MANAGEMENT_SUPPORT
#include	"../um.h"
void	formDefineUserMgmt(void);
#endif

#include "../hnap_interfaces.h" //Add for HNAP support - Ricky Cao
#include "../device_discover.h" //Add for treat broadcast device discover packet - Ricky Cao
#define FIREWALL_RELOAD_KEY	9999
	
/*********************************** Locals ***********************************/
/*
 *	Change configuration here
 */

static char_t		*rootWeb = T("web");			/* Root web directory */
static char_t		*password = T("");				/* Security password */
static int			retries = 5;					/* Server port retries */
static int			finished;						/* Finished flag */

/****************************** Forward Declarations **************************/

static int 	initWebs();
static int  websHomePageHandler(webs_t wp, char_t *urlPrefix, char_t *webDir,
				int arg, char_t *url, char_t *path, char_t *query);
static int  websApiHandler(webs_t wp, char_t *urlPrefix, char_t *webDir,
				int arg, char_t *url, char_t *path, char_t *query);
static void readLiveWlanIoctl(char *essidOut, size_t essidSz,
				char *chOut, size_t chSz, char *bandOut, size_t bandSz,
				char *bssidOut, size_t bssidSz);
extern void defaultErrorHandler(int etype, char_t *msg);
extern void defaultTraceHandler(int level, char_t *buf);
extern void upldForm(webs_t wp, char_t *path, char_t *query);
extern void formDumpFirmware(webs_t wp, char_t *path, char_t *query);
extern int getWLSBridgePort(void);
int isWaitOtherProcessBusy(void);
void setWaitOtherProcess(int busy);

static void websDiag(const char *message)
{
	int fd;

	fd = open("/dev/kmsg", O_WRONLY | O_NONBLOCK);
	if (fd >= 0) {
		write(fd, "<4>wap610n-webs: ",
			sizeof("<4>wap610n-webs: ") - 1);
		write(fd, message, strlen(message));
		write(fd, "\n", 1);
		close(fd);
	}
	printf("wap610n-webs: %s\n", message);
}

/*
 * Wrap vfork() used by precompiled proprietary object blobs (e.g. wireless.o).
 * On uClibc/Linux, vfork() suspends the parent thread and shares address space,
 * which freezes GoAhead HTTP serving, corrupts heap via lang_free(), and leaks
 * open HTTP sockets into background daemons. Replace with fork() + FD close.
 */
pid_t __wrap_vfork(void)
{
	return fork();
}

static void safeCopyFormVar(webs_t wp, char_t *name, char *dst, size_t dstSize)
{
	char_t *val;

	if (dst == NULL || dstSize == 0) {
		return;
	}
	dst[0] = '\0';
	val = websGetVar(wp, name, T(""));
	if (val != NULL && val[0] != '\0') {
		snprintf(dst, dstSize, "%s", val);
		dst[dstSize - 1] = '\0';
	}
}

static int isAllHexDigits(const char *s)
{
	const char *p;

	if (s == NULL || *s == '\0') {
		return 0;
	}
	for (p = s; *p != '\0'; p++) {
		if (!((*p >= '0' && *p <= '9') ||
		      (*p >= 'a' && *p <= 'f') ||
		      (*p >= 'A' && *p <= 'F'))) {
			return 0;
		}
	}
	return 1;
}

static void formatWepKeyParam(const char *inKey, char *outKey, size_t outSize)
{
	size_t len;

	if (outKey == NULL || outSize == 0) {
		return;
	}
	outKey[0] = '\0';
	if (inKey == NULL || inKey[0] == '\0') {
		return;
	}
	len = strlen(inKey);
	if ((len == 10 || len == 26) && isAllHexDigits(inKey)) {
		snprintf(outKey, outSize, "0x%s", inKey);
	} else {
		snprintf(outKey, outSize, "%s", inKey);
	}
	outKey[outSize - 1] = '\0';
}

static int readConfValueExact(const char *path, const char *key, char *out, size_t outSize)
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

static void preserveCustomSysConfKeys(void)
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
		if (!readConfValueExact("/tmp/sys.conf", keys[i], vals[i], sizeof(vals[i]))) {
			snprintf(vals[i], sizeof(vals[i]), "%s", defaults[i]);
		}
	}
	fp = fopen("/mnt/jffs2/sys.conf", "a");
	if (fp != NULL) {
		for (i = 0; i < 7; i++) {
			char existing[64];
			if (!readConfValueExact("/mnt/jffs2/sys.conf", keys[i], existing, sizeof(existing))) {
				fprintf(fp, "%s = %s\n", keys[i], vals[i]);
			}
		}
		fclose(fp);
	}
}

static int safeAspWaitToRedirect(int eid, webs_t wp, int argc, char_t **argv)
{
	(void)eid;
	(void)argc;
	(void)argv;
	return websWrite(wp, T("%s"), (access("/var/old_web_pid", F_OK) == 0) ? T("1") : T("0"));
}

static int safeAspGetWirelessChannel(int eid, webs_t wp, int argc, char_t **argv)
{
	char chStr[24];

	(void)eid;
	(void)argc;
	(void)argv;
	chStr[0] = '\0';
	readLiveWlanIoctl(NULL, 0, chStr, sizeof(chStr), NULL, 0, NULL, 0);
	if (chStr[0] == '\0') {
		MT_Get_Param("Channel", chStr, sizeof(chStr) - 1);
		chStr[sizeof(chStr) - 1] = '\0';
	}
	if (chStr[0] == '\0') {
		snprintf(chStr, sizeof(chStr), "0");
	}
	return websWrite(wp, T("%s"), chStr);
}

static int safeAspGetBSSID(int eid, webs_t wp, int argc, char_t **argv)
{
	FILE *fp;
	char buf[512];
	char macStr[24];
	int nread;
	char *p;
	size_t i;

	(void)eid;
	(void)argc;
	(void)argv;
	macStr[0] = '\0';
	fp = popen("/sbin/iwconfig wlan0 2>/dev/null", "r");
	if (fp != NULL) {
		nread = (int)fread(buf, 1, sizeof(buf) - 1, fp);
		pclose(fp);
		if (nread > 0) {
			buf[nread] = '\0';
			p = strstr(buf, "Access Point:");
			if (p != NULL) {
				p += 13;
				while (*p == ' ' || *p == '\t') {
					p++;
				}
				if (strncmp(p, "Not-Associated", 14) != 0) {
					i = 0;
					while (*p != '\0' && *p != ' ' && *p != '\t' &&
					       *p != '\r' && *p != '\n' && i < sizeof(macStr) - 1) {
						macStr[i++] = *p++;
					}
					macStr[i] = '\0';
				}
			}
		}
	}
	if (macStr[0] == '\0') {
		fp = fopen("/sys/class/net/wlan0/address", "r");
		if (fp != NULL) {
			if (fgets(macStr, sizeof(macStr), fp) != NULL) {
				size_t len = strlen(macStr);
				while (len > 0 && (macStr[len - 1] == '\n' || macStr[len - 1] == '\r')) {
					macStr[--len] = '\0';
				}
			}
			fclose(fp);
		}
	}
	if (macStr[0] == '\0') {
		snprintf(macStr, sizeof(macStr), "00:00:00:00:00:00");
	}
	return websWrite(wp, T("%s"), macStr);
}

static int isSafeConfigText(const char *s, size_t maxLen)
{
	size_t len = 0;

	if (s == NULL) {
		return 0;
	}
	while (s[len] != '\0') {
		unsigned char c = (unsigned char)s[len];
		if (c < 0x20 || c == 0x7f) {
			return 0;
		}
		len++;
		if (len > maxLen) {
			return 0;
		}
	}
	return 1;
}

static int isValidIdentStr(const char *s, size_t minLen, size_t maxLen)
{
	size_t len = 0;

	if (s == NULL) {
		return 0;
	}
	while (s[len] != '\0') {
		unsigned char c = (unsigned char)s[len];
		if (!((c >= 'A' && c <= 'Z') ||
		      (c >= 'a' && c <= 'z') ||
		      (c >= '0' && c <= '9') ||
		      c == '-' || c == '_')) {
			return 0;
		}
		len++;
		if (len > maxLen) {
			return 0;
		}
	}
	return (len >= minLen);
}

static int isValidHostName(const char *s)
{
	size_t len = 0;

	if (s == NULL) {
		return 0;
	}
	while (s[len] != '\0') {
		unsigned char c = (unsigned char)s[len];
		if (!((c >= 'A' && c <= 'Z') ||
		      (c >= 'a' && c <= 'z') ||
		      (c >= '0' && c <= '9') ||
		      c == '-')) {
			return 0;
		}
		len++;
		if (len > 32) {
			return 0;
		}
	}
	return (len >= 1);
}

static int isValidIpv4(const char *s)
{
	int dots = 0;
	const char *p;

	if (s == NULL || *s == '\0') {
		return 0;
	}
	p = s;
	while (*p != '\0') {
		int val = 0;
		int digits = 0;
		while (*p >= '0' && *p <= '9') {
			val = val * 10 + (*p - '0');
			digits++;
			if (digits > 3 || val > 255) {
				return 0;
			}
			p++;
		}
		if (digits == 0) {
			return 0;
		}
		if (*p == '.') {
			dots++;
			if (dots > 3) {
				return 0;
			}
			p++;
			if (*p == '\0') {
				return 0;
			}
		} else if (*p != '\0') {
			return 0;
		}
	}
	return (dots == 3);
}

static int isValidDigitsRange(const char *s, long minVal, long maxVal)
{
	size_t len = 0;
	long val;

	if (s == NULL || *s == '\0') {
		return 0;
	}
	while (s[len] != '\0') {
		if (s[len] < '0' || s[len] > '9') {
			return 0;
		}
		len++;
		if (len > 10) {
			return 0;
		}
	}
	val = atol(s);
	return (val >= minVal && val <= maxVal);
}

static int isValidMacAddr(const char *s)
{
	int i;

	if (s == NULL || strlen(s) != 17) {
		return 0;
	}
	for (i = 0; i < 17; i++) {
		if ((i % 3) == 2) {
			if (s[i] != ':') {
				return 0;
			}
		} else {
			char c = s[i];
			if (!((c >= '0' && c <= '9') ||
			      (c >= 'a' && c <= 'f') ||
			      (c >= 'A' && c <= 'F'))) {
				return 0;
			}
		}
	}
	return 1;
}

static int safeAspGetParamJs(int eid, webs_t wp, int argc, char_t **argv)
{
	char_t *paramName = NULL;
	char rawVal[256];
	char escVal[512];
	size_t i;
	size_t j = 0;

	(void)eid;
	if (ejArgs(argc, argv, T("%s"), &paramName) < 1 ||
	    paramName == NULL || paramName[0] == '\0') {
		return websWrite(wp, T(""));
	}
	memset(rawVal, 0, sizeof(rawVal));
	MT_Get_Param(paramName, rawVal, sizeof(rawVal) - 1);
	rawVal[sizeof(rawVal) - 1] = '\0';
	for (i = 0; rawVal[i] != '\0' && j + 7 < sizeof(escVal); i++) {
		unsigned char c = (unsigned char)rawVal[i];
		if (c == '\\' || c == '"' || c == '\'') {
			escVal[j++] = '\\';
			escVal[j++] = (char)c;
		} else if (c == '\r') {
			escVal[j++] = '\\';
			escVal[j++] = 'r';
		} else if (c == '\n') {
			escVal[j++] = '\\';
			escVal[j++] = 'n';
		} else if (c == '<') {
			memcpy(&escVal[j], "\\x3c", 4);
			j += 4;
		} else if (c == '>') {
			memcpy(&escVal[j], "\\x3e", 4);
			j += 4;
		} else if (c >= 0x20 && c != 0x7f) {
			escVal[j++] = (char)c;
		}
	}
	escVal[j] = '\0';
	return websWrite(wp, T("%s"), escVal);
}

static void safeFormNetworkBasic(webs_t wp, char_t *path, char_t *query)
{
	char ipConfigMethod[16];
	char hostName[64];
	char lanIP[32];
	char lanSubnet[32];
	char gateway[32];
	char priDns[32];
	char secDns[32];
	char langSelection[16];
	char *paramNames[10];
	int paramCount;
	pid_t pid;
	int fd;

	(void)path;
	(void)query;

	if (checkLoadDefault() == 1) {
		websRedirect(wp, T("/admin/upgrade.asp"));
		return;
	}
	if (isWaitOtherProcessBusy() != 0) {
		websError(wp, 503, T("Another configuration operation is in progress"));
		return;
	}

	safeCopyFormVar(wp, T("ipConfigMethod"), ipConfigMethod, sizeof(ipConfigMethod));
	safeCopyFormVar(wp, T("hostName"), hostName, sizeof(hostName));
	safeCopyFormVar(wp, T("lanIP"), lanIP, sizeof(lanIP));
	safeCopyFormVar(wp, T("lanSubnet"), lanSubnet, sizeof(lanSubnet));
	safeCopyFormVar(wp, T("gateway"), gateway, sizeof(gateway));
	safeCopyFormVar(wp, T("priDns"), priDns, sizeof(priDns));
	safeCopyFormVar(wp, T("secDns"), secDns, sizeof(secDns));
	safeCopyFormVar(wp, T("langSelection"), langSelection, sizeof(langSelection));

	if ((strcmp(ipConfigMethod, "0") != 0 && strcmp(ipConfigMethod, "1") != 0) ||
	    !isValidHostName(hostName)) {
		websError(wp, 400, T("Invalid network configuration parameter"));
		return;
	}
	if (langSelection[0] != '\0' && !isValidIdentStr(langSelection, 1, 15)) {
		websError(wp, 400, T("Invalid language selection"));
		return;
	}
	if (strcmp(ipConfigMethod, "1") == 0) {
		if (!isValidIpv4(lanIP) || !isValidIpv4(lanSubnet)) {
			websError(wp, 400, T("Invalid static IP or subnet mask"));
			return;
		}
		if ((gateway[0] != '\0' && !isValidIpv4(gateway)) ||
		    (priDns[0] != '\0' && !isValidIpv4(priDns)) ||
		    (secDns[0] != '\0' && !isValidIpv4(secDns))) {
			websError(wp, 400, T("Invalid gateway or DNS address"));
			return;
		}
	} else {
		if ((lanIP[0] != '\0' && !isValidIpv4(lanIP)) ||
		    (lanSubnet[0] != '\0' && !isValidIpv4(lanSubnet)) ||
		    (gateway[0] != '\0' && !isValidIpv4(gateway)) ||
		    (priDns[0] != '\0' && !isValidIpv4(priDns)) ||
		    (secDns[0] != '\0' && !isValidIpv4(secDns))) {
			websError(wp, 400, T("Invalid network address parameter"));
			return;
		}
	}

	saveOldWebPid(getpid(), "sta_network.asp");
	setWaitOtherProcess(1);
	pid = fork();
	if (pid < 0) {
		setWaitOtherProcess(0);
		unlink("/var/old_web_pid");
		websError(wp, 500, T("Unable to save network settings"));
		return;
	}
	if (pid != 0) {
		websRedirect(wp, T("/wait_page.asp"));
		return;
	}

	for (fd = 3; fd < 256; fd++) {
		close(fd);
	}
	waitRedirectPage();
	stopWPS_PBC();

	paramCount = 0;
	MT_Set_Param("ip_config_method", ipConfigMethod, 1);
	paramNames[paramCount++] = "ip_config_method";
	MT_Set_Param("HostName", hostName, 1);
	paramNames[paramCount++] = "HostName";
	if (lanIP[0] != '\0') {
		MT_Set_Param("ip_lan", lanIP, 1);
		paramNames[paramCount++] = "ip_lan";
	}
	if (lanSubnet[0] != '\0') {
		MT_Set_Param("subnet_lan", lanSubnet, 1);
		paramNames[paramCount++] = "subnet_lan";
	}
	if (gateway[0] != '\0') {
		MT_Set_Param("default_gw", gateway, 1);
		paramNames[paramCount++] = "default_gw";
	}
	if (priDns[0] != '\0') {
		MT_Set_Param("DHCPDDNS1", priDns, 1);
		paramNames[paramCount++] = "DHCPDDNS1";
	}
	if (secDns[0] != '\0') {
		MT_Set_Param("DHCPDDNS2", secDns, 1);
		paramNames[paramCount++] = "DHCPDDNS2";
	}
	if (langSelection[0] != '\0') {
		MT_Set_Param("Language", langSelection, 1);
		paramNames[paramCount++] = "Language";
	}
	MT_Set_Param("auto_network_type", "0", 1);
	paramNames[paramCount++] = "auto_network_type";

	doSystem("if [ -x /bin/config_lock.sh ]; then /bin/config_lock.sh %d >/dev/null 2>&1; fi", (int)getpid());
	MT_WriteConfFiles(paramCount, paramNames);
	preserveCustomSysConfKeys();
	doSystem("cp -f /mnt/jffs2/sys.conf /tmp/sys.conf >/dev/null 2>&1; "
	         "[ -s /mnt/jffs2/wlan0.conf ] && cp -f /mnt/jffs2/wlan0.conf /tmp/wlan0.conf >/dev/null 2>&1; "
	         "if [ -x /bin/config_unlock.sh ]; then /bin/config_unlock.sh %d >/dev/null 2>&1; fi",
	         (int)getpid());
	submitToFlash();

	{
		FILE *hf = fopen("/etc/hostname", "w");
		if (hf != NULL) {
			fprintf(hf, "%s\n", hostName);
			fclose(hf);
		}
		doSystem("/bin/hostname -F /etc/hostname >/dev/null 2>&1");
	}

	if (strcmp(ipConfigMethod, "0") == 0) {
		doSystem("killall udhcpd udhcpc >/dev/null 2>&1; /etc/udhcpc/dhcp.tcl startup &");
	} else {
		doSystem("killall udhcpc >/dev/null 2>&1");
		if (lanIP[0] != '\0' && lanSubnet[0] != '\0') {
			doSystem("ifconfig br0 %s netmask %s", lanIP, lanSubnet);
		}
		if (gateway[0] != '\0' && strcmp(gateway, "0.0.0.0") != 0 && strcmp(gateway, lanIP) != 0) {
			doSystem("route del default >/dev/null 2>&1; route add default gw %s dev br0", gateway);
		}
		{
			FILE *rf = fopen("/tmp/resolv.conf", "w");
			if (rf != NULL) {
				if (priDns[0] != '\0' && strcmp(priDns, "0.0.0.0") != 0 && strcmp(priDns, lanIP) != 0) {
					fprintf(rf, "nameserver %s\n", priDns);
				}
				if (secDns[0] != '\0' && strcmp(secDns, "0.0.0.0") != 0 && strcmp(secDns, lanIP) != 0) {
					fprintf(rf, "nameserver %s\n", secDns);
				}
				fclose(rf);
			}
		}
	}

	startWPS_PBC();
	childFinishAndExit();
}

static void safeFormWirelessBasic(webs_t wp, char_t *path, char_t *query)
{
	char netType[16];
	char nowMode[24];
	char essid[64];
	char freqBand[8];
	char country[16];
	char channel[16];
	char channelBonding[8];
	char upperLower[8];
	char bssid[32];
	char secMode[8];
	char wepKeyRaw[4][64];
	char wepKeyFmt[4][72];
	char defaultWEPKey[8];
	char wepKeyLen[16];
	char wepAuth[8];
	char wpaPersMode[8];
	char wpaPersEnc[8];
	char wpaPSK[128];
	char wpaEntMode[8];
	char wpaEntEnc[8];
	char wpaRadIP[32];
	char wpaRadPort[16];
	char wpaRadKey[64];
	char radRekey[16];
	char *paramNames[28];
	int paramCount;
	int i;
	int isApMode;
	pid_t pid;
	int fd;

	(void)path;
	(void)query;

	if (checkLoadDefault() == 1) {
		websRedirect(wp, T("/admin/upgrade.asp"));
		return;
	}
	if (isWaitOtherProcessBusy() != 0) {
		websError(wp, 503, T("Another configuration operation is in progress"));
		return;
	}

	memset(netType, 0, sizeof(netType));
	MT_Get_Param("network_type", netType, sizeof(netType) - 1);
	netType[sizeof(netType) - 1] = '\0';

	safeCopyFormVar(wp, T("nowMode"), nowMode, sizeof(nowMode));
	safeCopyFormVar(wp, T("submit_ESSID"), essid, sizeof(essid));
	safeCopyFormVar(wp, T("frequencyBand"), freqBand, sizeof(freqBand));
	safeCopyFormVar(wp, T("country"), country, sizeof(country));
	safeCopyFormVar(wp, T("channel"), channel, sizeof(channel));
	safeCopyFormVar(wp, T("channelBounding"), channelBonding, sizeof(channelBonding));
	safeCopyFormVar(wp, T("upperLower"), upperLower, sizeof(upperLower));
	safeCopyFormVar(wp, T("BSSID"), bssid, sizeof(bssid));
	safeCopyFormVar(wp, T("securityMode"), secMode, sizeof(secMode));
	safeCopyFormVar(wp, T("WEPKey0"), wepKeyRaw[0], sizeof(wepKeyRaw[0]));
	safeCopyFormVar(wp, T("WEPKey1"), wepKeyRaw[1], sizeof(wepKeyRaw[1]));
	safeCopyFormVar(wp, T("WEPKey2"), wepKeyRaw[2], sizeof(wepKeyRaw[2]));
	safeCopyFormVar(wp, T("WEPKey3"), wepKeyRaw[3], sizeof(wepKeyRaw[3]));
	safeCopyFormVar(wp, T("defaultWEPKey"), defaultWEPKey, sizeof(defaultWEPKey));
	safeCopyFormVar(wp, T("WEPKeyLength"), wepKeyLen, sizeof(wepKeyLen));
	safeCopyFormVar(wp, T("WEPAuth"), wepAuth, sizeof(wepAuth));
	safeCopyFormVar(wp, T("WPAPersonalMode"), wpaPersMode, sizeof(wpaPersMode));
	safeCopyFormVar(wp, T("WPAPersonalEncryption"), wpaPersEnc, sizeof(wpaPersEnc));
	safeCopyFormVar(wp, T("WPAPSK"), wpaPSK, sizeof(wpaPSK));
	safeCopyFormVar(wp, T("WPAEnterpriseMode"), wpaEntMode, sizeof(wpaEntMode));
	safeCopyFormVar(wp, T("WPAEnterpriseEncryption"), wpaEntEnc, sizeof(wpaEntEnc));
	safeCopyFormVar(wp, T("WPARadiusIP"), wpaRadIP, sizeof(wpaRadIP));
	safeCopyFormVar(wp, T("WPARadiusPort"), wpaRadPort, sizeof(wpaRadPort));
	safeCopyFormVar(wp, T("WPARadiusKey"), wpaRadKey, sizeof(wpaRadKey));
	safeCopyFormVar(wp, T("RadiusReKeyInterval"), radRekey, sizeof(radRekey));

	if ((essid[0] != '\0' && !isSafeConfigText(essid, 32)) ||
	    (bssid[0] != '\0' && !isValidMacAddr(bssid)) ||
	    (freqBand[0] != '\0' && !isValidDigitsRange(freqBand, 0, 2)) ||
	    (country[0] != '\0' && !isValidIdentStr(country, 1, 8)) ||
	    (channel[0] != '\0' && !isValidDigitsRange(channel, 0, 196)) ||
	    (channelBonding[0] != '\0' && !isValidDigitsRange(channelBonding, 0, 1)) ||
	    (upperLower[0] != '\0' && !isValidDigitsRange(upperLower, 0, 1)) ||
	    (secMode[0] != '\0' && !isValidDigitsRange(secMode, 0, 5)) ||
	    (wpaPSK[0] != '\0' && !isSafeConfigText(wpaPSK, 64)) ||
	    (wpaRadIP[0] != '\0' && !isValidIpv4(wpaRadIP)) ||
	    (wpaRadKey[0] != '\0' && !isSafeConfigText(wpaRadKey, 63))) {
		websError(wp, 400, T("Invalid wireless configuration parameter"));
		return;
	}
	for (i = 0; i < 4; i++) {
		if (wepKeyRaw[i][0] != '\0' && !isSafeConfigText(wepKeyRaw[i], 32)) {
			websError(wp, 400, T("Invalid WEP key"));
			return;
		}
	}
	if ((defaultWEPKey[0] != '\0' && !isValidDigitsRange(defaultWEPKey, 0, 3)) ||
	    (wepKeyLen[0] != '\0' && !isValidDigitsRange(wepKeyLen, 0, 128)) ||
	    (wepAuth[0] != '\0' && !isValidDigitsRange(wepAuth, 0, 3)) ||
	    (wpaPersMode[0] != '\0' && !isValidDigitsRange(wpaPersMode, 0, 4)) ||
	    (wpaPersEnc[0] != '\0' && !isValidDigitsRange(wpaPersEnc, 0, 4)) ||
	    (wpaEntMode[0] != '\0' && !isValidDigitsRange(wpaEntMode, 0, 4)) ||
	    (wpaEntEnc[0] != '\0' && !isValidDigitsRange(wpaEntEnc, 0, 4)) ||
	    (wpaRadPort[0] != '\0' && !isValidDigitsRange(wpaRadPort, 1, 65535)) ||
	    (radRekey[0] != '\0' && !isValidDigitsRange(radRekey, 0, 86400))) {
		websError(wp, 400, T("Invalid wireless numeric parameter"));
		return;
	}

	isApMode = (atoi(netType) == 2 || strcmp(nowMode, "apMode") == 0) ? 1 : 0;

	saveOldWebPid(getpid(), "wireless_basic.asp");
	setWaitOtherProcess(1);
	pid = fork();
	if (pid < 0) {
		setWaitOtherProcess(0);
		unlink("/var/old_web_pid");
		websError(wp, 500, T("Unable to save wireless settings"));
		return;
	}
	if (pid != 0) {
		websRedirect(wp, T("/wait_page.asp"));
		return;
	}

	for (fd = 3; fd < 256; fd++) {
		close(fd);
	}
	waitRedirectPage();
	stopWPS_PBC();

	paramCount = 0;
	if (essid[0] != '\0') {
		MT_Set_Param("NonProc_ESSID", essid, 1);
		paramNames[paramCount++] = "NonProc_ESSID";
		MT_Set_Param("Wildcard_ESSID", essid, 1);
		paramNames[paramCount++] = "Wildcard_ESSID";
	}
	if (!isApMode && bssid[0] != '\0') {
		MT_Set_Param("BSSID", bssid, 1);
		paramNames[paramCount++] = "BSSID";
	}
	if (freqBand[0] != '\0') {
		MT_Set_Param("FrequencyBand", freqBand, 1);
		paramNames[paramCount++] = "FrequencyBand";
		if (strcmp(freqBand, "0") == 0) {
			MT_Set_Param("NetworkMode", "14", 1);
			paramNames[paramCount++] = "NetworkMode";
		} else if (strcmp(freqBand, "1") == 0) {
			MT_Set_Param("NetworkMode", "23", 1);
			paramNames[paramCount++] = "NetworkMode";
		}
	}
	if (country[0] != '\0') {
		MT_Set_Param("Country", country, 1);
		paramNames[paramCount++] = "Country";
	}
	if (channel[0] != '\0') {
		MT_Set_Param("Channel", channel, 1);
		paramNames[paramCount++] = "Channel";
	}
	if (channelBonding[0] != '\0') {
		MT_Set_Param("ChannelBonding", channelBonding, 1);
		paramNames[paramCount++] = "ChannelBonding";
		MT_Set_Param("IsHTEnabled", "1", 1);
		paramNames[paramCount++] = "IsHTEnabled";
	}
	if (upperLower[0] != '\0') {
		MT_Set_Param("UpperLowerChannelBonding", upperLower, 1);
		paramNames[paramCount++] = "UpperLowerChannelBonding";
	}

	if (secMode[0] != '\0') {
		MT_Set_Param("NonProcSecurityMode", secMode, 1);
		paramNames[paramCount++] = "NonProcSecurityMode";

		if (strcmp(secMode, "2") == 0) {
			static char * const wepParamNames[4] = {
				"WepKeys_DefaultKey0",
				"WepKeys_DefaultKey1",
				"WepKeys_DefaultKey2",
				"WepKeys_DefaultKey3"
			};
			for (i = 0; i < 4; i++) {
				formatWepKeyParam(wepKeyRaw[i], wepKeyFmt[i], sizeof(wepKeyFmt[i]));
				if (wepKeyFmt[i][0] != '\0') {
					MT_Set_Param(wepParamNames[i], wepKeyFmt[i], 1);
					paramNames[paramCount++] = wepParamNames[i];
				}
			}
			if (defaultWEPKey[0] != '\0') {
				MT_Set_Param("WepTxKeyIdx", defaultWEPKey, 1);
				paramNames[paramCount++] = "WepTxKeyIdx";
			}
			if (wepKeyLen[0] != '\0') {
				MT_Set_Param("NonProc_WepKeyLength", wepKeyLen, 1);
				paramNames[paramCount++] = "NonProc_WepKeyLength";
			}
			if (wepAuth[0] != '\0') {
				MT_Set_Param("NonProc_Authentication", wepAuth, 1);
				paramNames[paramCount++] = "NonProc_Authentication";
				MT_Set_Param("Authentication", wepAuth, 1);
				paramNames[paramCount++] = "Authentication";
			}
			MT_Set_Param("WepEncryption", "1", 1);
			paramNames[paramCount++] = "WepEncryption";
		} else {
			MT_Set_Param("WepEncryption", "0", 1);
			paramNames[paramCount++] = "WepEncryption";
			MT_Set_Param("Authentication", "0", 1);
			paramNames[paramCount++] = "Authentication";
		}

		if (strcmp(secMode, "3") == 0) {
			if (wpaPersMode[0] == '\0') {
				snprintf(wpaPersMode, sizeof(wpaPersMode),
					(strcmp(wpaPersEnc, "2") == 0) ? "3" : "2");
			}
			MT_Set_Param("NonProc_WPA_Personal_Mode", wpaPersMode, 1);
			paramNames[paramCount++] = "NonProc_WPA_Personal_Mode";
			if (wpaPersEnc[0] != '\0') {
				MT_Set_Param("NonProc_WPA_Personal_Encapsulation", wpaPersEnc, 1);
				paramNames[paramCount++] = "NonProc_WPA_Personal_Encapsulation";
			}
			if (wpaPSK[0] != '\0') {
				MT_Set_Param("NonProc_WPA_Personal_PSK", wpaPSK, 1);
				paramNames[paramCount++] = "NonProc_WPA_Personal_PSK";
			}
		} else if (strcmp(secMode, "4") == 0) {
			if (wpaEntMode[0] != '\0') {
				MT_Set_Param("NonProc_WPA_Enterprise_Mode", wpaEntMode, 1);
				paramNames[paramCount++] = "NonProc_WPA_Enterprise_Mode";
			}
			if (wpaEntEnc[0] != '\0') {
				MT_Set_Param("NonProc_WPA_Enterprise_Encapsulation", wpaEntEnc, 1);
				paramNames[paramCount++] = "NonProc_WPA_Enterprise_Encapsulation";
			}
			if (wpaRadIP[0] != '\0') {
				MT_Set_Param("NonProc_WPA_Enterprise_Radius_IP", wpaRadIP, 1);
				paramNames[paramCount++] = "NonProc_WPA_Enterprise_Radius_IP";
			}
			if (wpaRadPort[0] != '\0') {
				MT_Set_Param("NonProc_WPA_Enterprise_Radius_Port", wpaRadPort, 1);
				paramNames[paramCount++] = "NonProc_WPA_Enterprise_Radius_Port";
			}
			if (wpaRadKey[0] != '\0') {
				MT_Set_Param("NonProc_WPA_Enterprise_Radius_Key", wpaRadKey, 1);
				paramNames[paramCount++] = "NonProc_WPA_Enterprise_Radius_Key";
			}
			if (radRekey[0] != '\0') {
				MT_Set_Param("NonProc_WPA_Enterprise_Radius_ReKey_Interval", radRekey, 1);
				paramNames[paramCount++] = "NonProc_WPA_Enterprise_Radius_ReKey_Interval";
			}
		}
	}

	MT_Set_Param("unconfigured", "0", 1);
	paramNames[paramCount++] = "unconfigured";
	MT_Set_Param("forceWlanDisconnect", "0", 1);
	paramNames[paramCount++] = "forceWlanDisconnect";
	if (country[0] != '\0') {
		MT_Set_Param("EEPROMCountryValid", "1", 1);
		paramNames[paramCount++] = "EEPROMCountryValid";
	}

	doSystem("if [ -x /bin/config_lock.sh ]; then /bin/config_lock.sh %d >/dev/null 2>&1; fi", (int)getpid());
	MT_WriteConfFiles(paramCount, paramNames);
	preserveCustomSysConfKeys();
	doSystem("[ -s /mnt/jffs2/sys.conf ] && cp -f /mnt/jffs2/sys.conf /tmp/sys.conf >/dev/null 2>&1; "
	         "cp -f /mnt/jffs2/wlan0.conf /tmp/wlan0.conf >/dev/null 2>&1; "
	         "if [ -x /bin/config_unlock.sh ]; then /bin/config_unlock.sh %d >/dev/null 2>&1; fi",
	         (int)getpid());
	submitToFlash();

	if (isApMode) {
		int needBounce = (country[0] != '\0' || freqBand[0] != '\0' ||
		                  channelBonding[0] != '\0' || upperLower[0] != '\0' ||
		                  channel[0] != '\0') ? 1 : 0;
		if (needBounce) {
			doSystem("/sbin/ifconfig wlan0 down >/dev/null 2>&1");
		}
		if (country[0] != '\0') {
			FILE *pf = fopen("/proc/sys/dev/mtlk/wlan0/Country", "w");
			if (pf != NULL) {
				fprintf(pf, "%s\n", country);
				fclose(pf);
			}
			doSystem("/root/mtlk/etc/mtpriv wlan0 Country %s >/dev/null 2>&1", country);
		}
		if (freqBand[0] != '\0') {
			doSystem("/root/mtlk/etc/mtpriv wlan0 FrequencyBand %s >/dev/null 2>&1", freqBand);
		}
		if (channelBonding[0] != '\0') {
			FILE *pf = fopen("/proc/sys/dev/mtlk/wlan0/ChannelBonding", "w");
			if (pf != NULL) {
				fprintf(pf, "%s\n", channelBonding);
				fclose(pf);
			}
			doSystem("/root/mtlk/etc/mtpriv wlan0 ChannelBonding %s >/dev/null 2>&1", channelBonding);
		}
		if (upperLower[0] != '\0') {
			FILE *pf = fopen("/proc/sys/dev/mtlk/wlan0/UpperLowerChannelBonding", "w");
			if (pf != NULL) {
				fprintf(pf, "%s\n", upperLower);
				fclose(pf);
			}
			doSystem("/root/mtlk/etc/mtpriv wlan0 UpperLowerChannelBonding %s >/dev/null 2>&1", upperLower);
		}
		if (channel[0] != '\0') {
			if (strcmp(channel, "0") == 0) {
				doSystem("/sbin/iwconfig wlan0 channel auto >/dev/null 2>&1");
			} else {
				doSystem("/sbin/iwconfig wlan0 channel %s >/dev/null 2>&1", channel);
			}
		}
		if (needBounce) {
			doSystem("/sbin/ifconfig wlan0 up >/dev/null 2>&1");
		}
	} else if (country[0] != '\0') {
		FILE *pf = fopen("/proc/sys/dev/mtlk/wlan0/Country", "w");
		if (pf != NULL) {
			fprintf(pf, "%s\n", country);
			fclose(pf);
		}
		doSystem("/root/mtlk/etc/mtpriv wlan0 Country %s >/dev/null 2>&1", country);
	}
	doSystem("cd /root/mtlk/web/ && /root/mtlk/web/init_security.tcl reactivate >/dev/null 2>&1");
	startWPS_PBC();
	childFinishAndExit();
}

/* Safe non-blocking form overrides to eliminate legacy U-Media blocking bugs */
static void safeFormGetCurrentWPSStatus(webs_t wp, char_t *path, char_t *query)
{
	FILE *fp = fopen("/var/WPS_History", "r");
	char line[64];
	char statusStr[64] = "-1,-1,0,0,\n";

	if (fp != NULL) {
		if (fgets(line, sizeof(line), fp) != NULL) {
			int ts = 0, stat = -1;
			if (sscanf(line, "%d,%d", &ts, &stat) == 2) {
				snprintf(statusStr, sizeof(statusStr), "%d,-1,0,0,\n", stat);
			}
		}
		fclose(fp);
	}

	wp->flags &= ~WEBS_KEEP_ALIVE;
	websWrite(wp, T("HTTP/1.0 200 OK\r\nContent-Type: text/plain\r\nConnection: close\r\nPragma: no-cache\r\nCache-Control: no-cache\r\n\r\n%s"), statusStr);
	websDone(wp, 200);
}

static void safeFormWirelessTypeSet(webs_t wp, char_t *path, char_t *query)
{
	char_t *cfgType = websGetVar(wp, T("wirelessConfigType"), T("manual"));

	(void)path;
	(void)query;
	if (cfgType == NULL || (strcmp(cfgType, "manual") != 0 && strcmp(cfgType, "wps") != 0)) {
		websError(wp, 400, T("Invalid wireless configuration type"));
		return;
	}
	MT_Set_Param("wirelessConfigType", cfgType, 1);
	websRedirect(wp, T("/wireless/security.asp"));
}
#ifdef B_STATS
static void printMemStats(int handle, char_t *fmt, ...);
static void memLeaks();
#endif

static volatile sig_atomic_t ReParseOnLoad = 0;

void CatchSIG(int s)
{
	(void)s;
	ReParseOnLoad = 1;
}

//Jacky.Yang 11-Feb-2009, for limitation apply process.
int waitOtherProcess = 0;
time_t waitOtherProcessSince = 0;
int websMainPid = 0;

int isWaitOtherProcessBusy(void)
{
	if (waitOtherProcess != 0 && waitOtherProcessSince > 0 &&
	    (time(NULL) - waitOtherProcessSince) > 120) {
		waitOtherProcess = 0;
		waitOtherProcessSince = 0;
		unlink("/var/old_web_pid");
	}
	return waitOtherProcess;
}

void setWaitOtherProcess(int busy)
{
	waitOtherProcess = busy ? 1 : 0;
	waitOtherProcessSince = busy ? time(NULL) : 0;
}

static void syncShadowFromSysConf(void)
{
	char admPass[128];

	memset(admPass, 0, sizeof(admPass));
	MT_Get_Param(MTSECURITY_USER_PASSWD_VAR_NAME, admPass, sizeof(admPass) - 1);
	admPass[sizeof(admPass) - 1] = '\0';
	if (admPass[0] != '\0') {
		syncRootShadowPassword(admPass);
	}
}

/*********************************** Code *************************************/

/*
 *	Main -- entry point from LINUX
 */

int main(int argc, char** argv)
{
	struct sigaction setup_action;
	sigset_t block_mask;
	int firewallReloadID;
	struct msgbuffer {
		long mtype;
		char mtext[1025];
	} msgInfo;

	//Jacky.Yang 15-May-209, set open debug message value;
	//openDebug=1;
	
	websMainPid = (int)getpid();
	websDiag("startup");

	firewallReloadID = msgget(FIREWALL_RELOAD_KEY, IPC_CREAT | 0666);
	if(firewallReloadID == -1){
		printf("goahead: Create firewall reload message queue fail!\n");
		perror("msgget");
	}
	//Jacky.Yang 25-Mar-2008,

	//Jacky.Yang 27-Nov-2008, drop it
	//MTtask_t* pStatTask = NULL;
	if (!MT_ParseArguments(argc, argv)) {
		websDiag("argument parsing failed");
		return MT_ShowHelp();
	}


	sigemptyset (&block_mask);
	sigaddset (&block_mask, SIGHUP);
    	//sigaddset (&block_mask, SIGQUIT);
    	//sigaddset (&block_mask, SIGTERM);
    	setup_action.sa_handler = CatchSIG;
    	setup_action.sa_mask = block_mask;
    	setup_action.sa_flags = 0;
    	sigaction (SIGHUP, &setup_action, NULL);
/*
 *	Initialize the memory allocator. Allow use of malloc and start 
 *	with a 60K heap.  For each page request approx 8KB is allocated.
 *	60KB allows for several concurrent page requests.  If more space
 *	is required, malloc will be used for the overflow.
 */
	bopen(NULL, (60 * 1024), B_USE_MALLOC);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);

/*
 *	Initialize the web server
 */
	if (initWebs() < 0) {
		websDiag("web server initialization failed");
		printf("Failed to initialize web server. Aborting.\n");
		return -1;
	}

#ifdef WEBS_SSL_SUPPORT
	websSSLOpen();
#endif

	device_discover_open(); //Add to discover device on differ subnet for Linksys Direct TV 

	if (!MT_Read_All_Params())
	{
		websDiag("configuration parameters could not be loaded");
		printf("Cannot read parameters. Aborting.\n");
		return -1;
	}
	syncShadowFromSysConf();

	websInitRealm();//Add for WET610N - Ricky Cao on 2008/09/03

/*
 *	Basic event loop. SocketReady returns true when a socket is ready for
 *	service. SocketSelect will block until an event occurs. SocketProcess
 *	will actually do the servicing.
 */
 	//Jacky.Yang 27-Nov-2008, drop it
	//pStatTask = MT_PrepareStatisticsTask(); // Will do nothing without #define USE_MT_STAT_TASK
	
	//Jacky.Yang 6-Feb-2009, multi-lang init
	//system("/root/mtlk/etc/Multi-Lang&");
	if (lang_init())
		printf("goahead.c: multi-lang init success.\n");
	else
		printf("goahead.c: multi-lang init fail.\n");
	
	while (!finished) {
		if (websMainPid > 0 && (int)getpid() != websMainPid) {
			/* Guard against leaked forked child ever entering main event loop */
			_exit(0);
		}
		if (socketReady(-1) || socketSelect(-1, 1000)) {
			socketProcess(-1);
		}
		websCgiCleanup();
		emfSchedProcess();
		
		/* Check IPC message queue for debug toggle */
		if (firewallReloadID != -1) {
			int mlen = msgrcv(firewallReloadID, &msgInfo, (size_t)(sizeof(msgInfo.mtext) - 1), 0, IPC_NOWAIT | MSG_NOERROR);
			if (mlen >= 0) {
				msgInfo.mtext[mlen] = '\0';
				printf("goahead: Get message from send_event, the message is %s\n", msgInfo.mtext);
				if (!strcmp(msgInfo.mtext, "openDebug")) {
					printf("goahead: Open Debug message.\n");
					openDebug = 1;
				}
				else if (!strcmp(msgInfo.mtext, "closeDebug")) {
					printf("goahead: Close Debug message.\n");
					openDebug = 0;
				}
			}
		}
		if (ReParseOnLoad == 1) {
			ReParseOnLoad = 0;
			debug_printf("CatchSIG: Reload flash content in the Web Server.\n");
			loadConfigFromFile("/mnt/jffs2/wlan0.conf");
			loadConfigFromFile("/mnt/jffs2/sys.conf");
			MT_ParseOnLoad();
			syncShadowFromSysConf();
			websInitRealm();
			lang_free();
			lang_init();
			setWaitOtherProcess(0);
		}
		//Jacky.Yang 27-Nov-2008, drop it
		//MT_TaskScheduler(pStatTask); // Will do nothing without #define USE_MT_STAT_TASK
	}

#ifdef WEBS_SSL_SUPPORT
	websSSLClose();
#endif

#ifdef USER_MANAGEMENT_SUPPORT
	umClose();
#endif

/*
 *	Close the socket module, report memory leaks and close the memory allocator
 */
	websCloseServer();
	socketClose();
	MT_Free_Params();
#ifdef B_STATS
	memLeaks();
#endif
	bclose();
	return 0;
}

/******************************************************************************/
/*
 *	Initialize the web server.
 */

static int initWebs()
{
	struct in_addr	intaddr = {0};
	char			host[128] = "localhost", dir[128], webdir[128];
	char			*cp;
	char_t			wbuf[128];

/*
 *	Initialize the socket subsystem
 */
	socketOpen();

#ifdef USER_MANAGEMENT_SUPPORT
/*
 *	Initialize the User Management database
 */
	umOpen();
	umRestore(T("umconfig.txt"));
#endif

/*
 *	Define the local Ip address, host name, default home page and the 
 *	root web directory.
 */
	/*if (gethostname(host, sizeof(host)) < 0) {
		error(E_L, E_LOG, T("Can't get hostname"));
		return -1;
	}
	sprintf(host, "localhost"); // TEMP TEMP TEMP - DON'T KNOW HOW TO RESOLVE HOST NAME !
	if ((hp = gethostbyname(host)) == NULL) {
		error(E_L, E_LOG, T("Can't get host address"));
		return -1;
	}
	memcpy((char *) &intaddr, (char *) hp->h_addr_list[0],
		(size_t) hp->h_length);
*/


/*
 *	Set ../web as the root web. Modify this to suit your needs
 */
	
	if (getcwd(dir, sizeof(dir)) == NULL) {
		strncpy(dir, "/root/mtlk", sizeof(dir) - 1);
		dir[sizeof(dir) - 1] = '\0';
	} else if ((cp = strrchr(dir, '/'))) {
		*cp = '\0';
	}
	snprintf(webdir, sizeof(webdir), "%s/%s", dir, rootWeb);
	webdir[sizeof(webdir) - 1] = '\0';

	websSetDefaultDir(webdir);

/*
 *	Configure the web server options before opening the web server
 */
	//Jacky.Yang 27-Nov-2008, modify it.
	websSetDefaultDir(MT_WebRootDir);
	//websSetDefaultDir(webdir);
	cp = inet_ntoa(intaddr);
	ascToUni(wbuf, cp, min(strlen(cp) + 1, sizeof(wbuf)));
	websSetIpaddr(wbuf);
	ascToUni(wbuf, host, min(strlen(host) + 1, sizeof(wbuf)));
	websSetHost(wbuf);

/*
 *	Configure the web server options before opening the web server
 */
	//Jacky.Yang 5-Jun-2008, Modify default page
	//websSetDefaultPage(T("default.asp"));
	//websSetDefaultPage(T("/station/wireless_basic.asp"));
	websSetDefaultPage(T("index.html"));
	websSetPassword(password);

/* 
 *	Open the web server on the given port. If that port is taken, try
 *	the next sequential port for up to "retries" attempts.
 */
	//Jacky.Yang 27-Nov-2008
	//websOpenServer(port, retries);
	if (websOpenServer(websPort, retries) < 0) {
		websDiag("HTTP listen socket could not be opened");
		printf("Failed to open HTTP listen socket.\n");
		return -1;
	}
	websDiag("HTTP listener initialized");

/*
 * 	First create the URL handlers. Note: handlers are called in sorted order
 *	with the longest path handler examined first. Here we define the security 
 *	handler, forms handler and the default web page handler.
 */
	websUrlHandlerDefine(T(""), NULL, 0, websSecurityHandler, 
		WEBS_HANDLER_FIRST);
	websUrlHandlerDefine(T("/goform"), NULL, 0, websFormHandler, 0);
	websUrlHandlerDefine(T("/cgi-bin"), NULL, 0, websCgiHandler, 0);
	websUrlHandlerDefine(T("/virtualtmp"), MT_WebVirtualTmpDir, 0, websVirtualDirHandler, 0); 

#ifdef MT_USE_STAT_TASK
	websUrlHandlerDefine(T("/mt_stat"), NULL, 0, websStatisticsHandler, 0); 
#endif
	websUrlHandlerDefine(T("/HNAP1"), NULL, 0, websHNAPHandler, 0); //Ricky Test	
	websUrlHandlerDefine(T("/api"), NULL, 0, websApiHandler, 0);
	websUrlHandlerDefine(T(""), NULL, 0, websDefaultHandler, 
		WEBS_HANDLER_LAST); 

	// Define all ASP functions and CGI forms
	MT_DefineAPIFuncs();
	MT_DefineValidationFuncs();

	//Jacky.Yang 30-May-2008, Define U-Media API
	formDefineWireless();
	formDefineManagement();
	formDefineUtils();
	formDefineWPS();
	websFormDefine(T("dumpFirmware"), formDumpFirmware);
	websFormDefine(T("getCurrentWPSStatus"), safeFormGetCurrentWPSStatus);
	websFormDefine(T("wirelessTypeSet"), safeFormWirelessTypeSet);
	websFormDefine(T("networkBasic"), safeFormNetworkBasic);
	websFormDefine(T("wirelessBasic"), safeFormWirelessBasic);
	websAspDefine(T("waitToRedirect"), safeAspWaitToRedirect);
	websAspDefine(T("getWirelessChannel"), safeAspGetWirelessChannel);
	websAspDefine(T("getBSSID"), safeAspGetBSSID);
	websAspDefine(T("getParamJs"), safeAspGetParamJs);

/*
 *	Create the Form handlers for the User Management pages
 */
#ifdef USER_MANAGEMENT_SUPPORT
	formDefineUserMgmt();
#endif

/*
 *	Create a handler for the default home page
 */
	websUrlHandlerDefine(T("/"), NULL, 0, websHomePageHandler, 0); 
	return 0;
}

/******************************************************************************/
/*
 *	Home page handler
 */

static int syncConfPair(const char *jffsPath, const char *tmpPath,
	time_t *lastJM, off_t *lastJS, time_t *lastTM, off_t *lastTS)
{
	struct stat stJ, stT;
	int hasJ, hasT;
	int changed = 0;

	memset(&stJ, 0, sizeof(stJ));
	memset(&stT, 0, sizeof(stT));
	hasJ = (stat(jffsPath, &stJ) == 0);
	hasT = (stat(tmpPath, &stT) == 0);

	if (hasJ && (stJ.st_mtime != *lastJM || stJ.st_size != *lastJS)) {
		changed = 1;
	}
	if (hasT && (stT.st_mtime != *lastTM || stT.st_size != *lastTS)) {
		changed = 1;
	}
	if (!changed) {
		return 0;
	}

	if (hasJ && hasT) {
		if (stT.st_mtime > stJ.st_mtime) {
			loadConfigFromFile((char_t *)jffsPath);
			loadConfigFromFile((char_t *)tmpPath);
		} else {
			loadConfigFromFile((char_t *)tmpPath);
			loadConfigFromFile((char_t *)jffsPath);
		}
	} else if (hasJ) {
		loadConfigFromFile((char_t *)jffsPath);
	} else if (hasT) {
		loadConfigFromFile((char_t *)tmpPath);
	}

	*lastJM = hasJ ? stJ.st_mtime : 0;
	*lastJS = hasJ ? stJ.st_size : 0;
	*lastTM = hasT ? stT.st_mtime : 0;
	*lastTS = hasT ? stT.st_size : 0;
	return 1;
}

static void syncConfigFromDiskIfChanged(void)
{
	static time_t sysJM = 0, sysTM = 0, wlJM = 0, wlTM = 0;
	static off_t sysJS = -1, sysTS = -1, wlJS = -1, wlTS = -1;
	int c1, c2;

	c1 = syncConfPair("/mnt/jffs2/sys.conf", "/tmp/sys.conf", &sysJM, &sysJS, &sysTM, &sysTS);
	c2 = syncConfPair("/mnt/jffs2/wlan0.conf", "/tmp/wlan0.conf", &wlJM, &wlJS, &wlTM, &wlTS);
	if (c1 || c2) {
		MT_ParseOnLoad();
		if (c1) {
			syncShadowFromSysConf();
			websInitRealm();
		}
	}
}

static int isSafeLocalRedirectPath(const char *target)
{
	const unsigned char *p;

	if (target == NULL || target[0] != '/') {
		return 0;
	}
	if (target[1] == '/' || target[1] == '\\') {
		return 0;
	}
	for (p = (const unsigned char *)target; *p != '\0'; p++) {
		if (*p < 0x20 || *p == 0x7f || *p == '\\' ||
		    *p == '\"' || *p == '<' || *p == '>') {
			return 0;
		}
	}
	return 1;
}

static void sendUiModeRedirect(webs_t wp, const char *target, const char *mode)
{
	const char *safeTarget = isSafeLocalRedirectPath(target) ? target : "/network/ap_network.asp";

	wp->flags &= ~WEBS_KEEP_ALIVE;
	websWrite(wp, T("HTTP/1.0 302 Redirect\r\n"));
	websWrite(wp, T("Server: %s\r\n"), WEBS_NAME);
	websWrite(wp, T("Location: %s\r\n"), (char_t *)safeTarget);
	websWrite(wp, T("Set-Cookie: ui=%s; Path=/\r\n"), (char_t *)mode);
	websWrite(wp, T("Pragma: no-cache\r\n"));
	websWrite(wp, T("Cache-Control: no-store\r\n"));
	websWrite(wp, T("Connection: close\r\n\r\n"));
	websDone(wp, 302);
}

static int websHomePageHandler(webs_t wp, char_t *urlPrefix, char_t *webDir,
	int arg, char_t *url, char_t *path, char_t *query)
{
	static const struct {
		const char *aspPath;
		const char *target;
	} legacyMap[] = {
		{ "/default.asp", "/index.html" },
		{ "/index.asp", "/index.html" },
		{ "/network/ap_network.asp", "/index.html#network" },
		{ "/network/sta_network.asp", "/index.html#network" },
		{ "/wireless/security.asp", "/index.html#wireless" },
		{ "/wireless/wireless_basic.asp", "/index.html#wireless" },
		{ "/wireless/wireless_advanced.asp", "/index.html#wireless" },
		{ "/wireless/wmm.asp", "/index.html#wireless" },
		{ "/wireless/wps_status.asp", "/index.html#wireless" },
		{ "/station/wireless_basic.asp", "/index.html#wireless" },
		{ "/station/wireless_advanced.asp", "/index.html#wireless" },
		{ "/station/wmm.asp", "/index.html#wireless" },
		{ "/station/wps_status.asp", "/index.html#wireless" },
		{ "/wireless/site_survey.asp", "/index.html#survey" },
		{ "/station/site_survey.asp", "/index.html#survey" },
		{ "/admin/management.asp", "/index.html#system" },
		{ "/admin/upgrade.asp", "/index.html#system" },
		{ "/admin/factory_defaults.asp", "/index.html#system" },
		{ "/status/device_status.asp", "/index.html#status" },
		{ "/status/sta_link_status.asp", "/index.html#status" },
		{ "/status/about.asp", "/index.html#status" }
	};
	size_t i;
	int isClassic = 0;

	(void)urlPrefix;
	(void)webDir;
	(void)arg;
	(void)url;

	syncConfigFromDiskIfChanged();

	if (query != NULL && strstr(query, "modern=1") != NULL) {
		sendUiModeRedirect(wp, "/index.html", "modern");
		return 1;
	}
	if (query != NULL && strstr(query, "classic=1") != NULL) {
		const char *dest = "/network/ap_network.asp";
		if (path != NULL && *path != '\0' && strcmp(path, "/") != 0 &&
		    strcmp(path, "/index.html") != 0 && strcmp(path, "/default.asp") != 0 &&
		    strcmp(path, "/index.asp") != 0) {
			dest = path;
		}
		sendUiModeRedirect(wp, dest, "classic");
		return 1;
	}
	if (wp != NULL && wp->cookie != NULL && strstr(wp->cookie, "ui=classic") != NULL) {
		isClassic = 1;
	}

	if (path == NULL || *path == '\0' || strcmp(path, "/") == 0) {
		websRedirect(wp, isClassic ? T("/network/ap_network.asp") : T("/index.html"));
		return 1;
	}
	if (isClassic) {
		return 0;
	}
	for (i = 0; i < sizeof(legacyMap) / sizeof(legacyMap[0]); i++) {
		if (strcmp(path, legacyMap[i].aspPath) == 0) {
			websRedirect(wp, (char_t *)legacyMap[i].target);
			return 1;
		}
	}
	return 0;
}

/******************************************************************************/
/*
 *	Zero-fork C89 JSON telemetry endpoints (/api/status.json, /api/wifi.json)
 */

static void jsonEscapeCopy(char *dst, size_t dstSize, const char *src)
{
	size_t i = 0;
	if (dstSize == 0) {
		return;
	}
	if (src == NULL) {
		dst[0] = '\0';
		return;
	}
	while (*src != '\0' && i + 2 < dstSize) {
		unsigned char c = (unsigned char)*src++;
		if (c == '\"' || c == '\\') {
			dst[i++] = '\\';
			dst[i++] = (char)c;
		} else if (c == '\t') {
			dst[i++] = '\\';
			dst[i++] = 't';
		} else if (c == '\r') {
			dst[i++] = '\\';
			dst[i++] = 'r';
		} else if (c == '\n') {
			dst[i++] = '\\';
			dst[i++] = 'n';
		} else if (c >= 0x20 && c != 0x7f) {
			dst[i++] = (char)c;
		}
	}
	dst[i] = '\0';
}

static void sendJsonResponse(webs_t wp, int status, const char *body)
{
	size_t bodyLen = strlen(body);
	const char *statusText = "OK";

	if (status == 404) {
		statusText = "Not Found";
	} else if (status == 500) {
		statusText = "Internal Server Error";
	} else if (status != 200) {
		statusText = "Error";
	}
	wp->flags &= ~WEBS_KEEP_ALIVE;
	websWrite(wp, T("HTTP/1.0 %d %s\r\n"), status, statusText);
	websWrite(wp, T("Server: %s\r\n"), WEBS_NAME);
	websWrite(wp, T("Connection: close\r\n"));
	websWrite(wp, T("Cache-Control: no-store\r\n"));
	websWrite(wp, T("Content-Type: application/json\r\n"));
	websWrite(wp, T("Content-Length: %lu\r\n\r\n"), (unsigned long)bodyLen);
	websWriteBlock(wp, (char_t *)body, (int)bodyLen);
	websDone(wp, status);
}

typedef struct {
	unsigned char mac_addr[6];
	unsigned char port_no;
	unsigned char is_local;
	unsigned int ageing_timer_value;
	unsigned int unused;
} api_br_fdb_t;

typedef struct {
	char ip[20];
	char mac[20];
} api_arp_entry_t;

static void getIfaceIpv4AndMask(const char *ifname, char *ipOut, size_t ipSize,
	char *maskOut, size_t maskSize)
{
	int fd;
	struct ifreq ifr;

	if (ipOut != NULL && ipSize > 0) {
		ipOut[0] = '\0';
	}
	if (maskOut != NULL && maskSize > 0) {
		maskOut[0] = '\0';
	}
	if (ifname == NULL || ifname[0] == '\0') {
		return;
	}
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		return;
	}
	if (ipOut != NULL && ipSize > 0) {
		memset(&ifr, 0, sizeof(ifr));
		strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);
		if (ioctl(fd, SIOCGIFADDR, &ifr) == 0) {
			struct sockaddr_in *sin = (struct sockaddr_in *)&ifr.ifr_addr;
			snprintf(ipOut, ipSize, "%s", inet_ntoa(sin->sin_addr));
			ipOut[ipSize - 1] = '\0';
		}
	}
	if (maskOut != NULL && maskSize > 0) {
		memset(&ifr, 0, sizeof(ifr));
		strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);
		if (ioctl(fd, SIOCGIFNETMASK, &ifr) == 0) {
			struct sockaddr_in *sin = (struct sockaddr_in *)&ifr.ifr_netmask;
			snprintf(maskOut, maskSize, "%s", inet_ntoa(sin->sin_addr));
			maskOut[maskSize - 1] = '\0';
		}
	}
	close(fd);
}

static void getIfaceMac(const char *ifname, char *macOut, size_t macSize)
{
	char pathBuf[64];
	FILE *fp;
	size_t len;

	if (macOut == NULL || macSize == 0) {
		return;
	}
	macOut[0] = '\0';
	if (ifname == NULL || ifname[0] == '\0') {
		return;
	}
	snprintf(pathBuf, sizeof(pathBuf), "/sys/class/net/%s/address", ifname);
	fp = fopen(pathBuf, "r");
	if (fp != NULL) {
		if (fgets(macOut, (int)macSize, fp) != NULL) {
			len = strlen(macOut);
			while (len > 0 && (macOut[len - 1] == '\n' || macOut[len - 1] == '\r' ||
			                   macOut[len - 1] == ' ' || macOut[len - 1] == '\t')) {
				macOut[--len] = '\0';
			}
		}
		fclose(fp);
	}
}

static void getParamEsc(const char *name, char *escOut, size_t escSize)
{
	char raw[256];

	if (escOut == NULL || escSize == 0) {
		return;
	}
	escOut[0] = '\0';
	if (name == NULL || name[0] == '\0') {
		return;
	}
	memset(raw, 0, sizeof(raw));
	MT_Get_Param((char *)name, raw, sizeof(raw) - 1);
	raw[sizeof(raw) - 1] = '\0';
	jsonEscapeCopy(escOut, escSize, raw);
}

static size_t jsonAppend(char *buf, size_t bufSize, size_t pos, const char *fmt, ...)
{
	va_list ap;
	int n;

	if (buf == NULL || bufSize == 0 || pos >= bufSize - 1) {
		return (bufSize > 0) ? (bufSize - 1) : 0;
	}
	va_start(ap, fmt);
	n = vsnprintf(buf + pos, bufSize - pos, fmt, ap);
	va_end(ap);
	if (n < 0) {
		return pos;
	}
	if ((size_t)n >= bufSize - pos) {
		return bufSize - 1;
	}
	return pos + (size_t)n;
}

#define API_SIOCGIWFREQ  0x8B05
#define API_SIOCGIWAP    0x8B15
#define API_SIOCGIWESSID 0x8B1B
#define API_SIOCGIWRTS   0x8B23

struct api_iw_point {
	void *pointer;
	unsigned short length;
	unsigned short flags;
};

struct api_iw_param {
	int value;
	unsigned char fixed;
	unsigned char disabled;
	unsigned short flags;
};

struct api_iw_freq {
	int m;
	short e;
	unsigned char i;
	unsigned char flags;
};

struct api_iwreq {
	char if_name[IFNAMSIZ];
	union {
		struct api_iw_point essid;
		struct api_iw_param rts;
		struct api_iw_freq freq;
		struct sockaddr ap_addr;
		char pad[16];
	} u;
};

typedef struct {
	unsigned char chaddr[16];
	unsigned int yiaddr;
	unsigned int expires;
} api_dhcp_lease_t;

typedef struct {
	char *data;
	size_t len;
	size_t cap;
	int oom;
} api_dynbuf_t;

static void dynInit(api_dynbuf_t *db, size_t initCap)
{
	db->data = (char *)malloc(initCap);
	db->len = 0;
	db->cap = (db->data != NULL) ? initCap : 0;
	db->oom = (db->data == NULL) ? 1 : 0;
	if (db->data != NULL) {
		db->data[0] = '\0';
	}
}

static int dynEnsure(api_dynbuf_t *db, size_t extra)
{
	size_t need;
	size_t newCap;
	char *nb;

	if (db->oom) {
		return 0;
	}
	if (extra > ((size_t)-1) - db->len - 1) {
		db->oom = 1;
		return 0;
	}
	need = db->len + extra + 1;
	if (need <= db->cap) {
		return 1;
	}
	newCap = db->cap ? db->cap : 4096;
	while (newCap < need) {
		if (newCap > 262144) {
			db->oom = 1;
			return 0;
		}
		newCap *= 2;
	}
	nb = (char *)realloc(db->data, newCap);
	if (nb == NULL) {
		db->oom = 1;
		return 0;
	}
	db->data = nb;
	db->cap = newCap;
	return 1;
}

static void dynAppendRaw(api_dynbuf_t *db, const char *s, size_t n)
{
	if (n == 0 || !dynEnsure(db, n)) {
		return;
	}
	memcpy(db->data + db->len, s, n);
	db->len += n;
	db->data[db->len] = '\0';
}

#define DYN_LIT(db, lit) dynAppendRaw((db), (lit), sizeof(lit) - 1)

static void dynAppendFmt(api_dynbuf_t *db, const char *fmt, ...)
{
	va_list ap;
	int n;
	size_t avail;

	if (db->oom) {
		return;
	}
	avail = db->cap - db->len;
	va_start(ap, fmt);
	n = vsnprintf(db->data + db->len, avail, fmt, ap);
	va_end(ap);
	if (n < 0) {
		return;
	}
	if ((size_t)n >= avail) {
		if (!dynEnsure(db, (size_t)n + 1)) {
			return;
		}
		avail = db->cap - db->len;
		va_start(ap, fmt);
		n = vsnprintf(db->data + db->len, avail, fmt, ap);
		va_end(ap);
		if (n < 0 || (size_t)n >= avail) {
			return;
		}
	}
	db->len += (size_t)n;
}

static void dynAppendJsonStr(api_dynbuf_t *db, const char *s)
{
	const unsigned char *p;

	dynAppendRaw(db, "\"", 1);
	if (s != NULL) {
		p = (const unsigned char *)s;
		while (*p != '\0' && !db->oom) {
			unsigned char c = *p++;
			if (c == '\"') {
				dynAppendRaw(db, "\\\"", 2);
			} else if (c == '\\') {
				dynAppendRaw(db, "\\\\", 2);
			} else if (c == '\t') {
				dynAppendRaw(db, "\\t", 2);
			} else if (c == '\r') {
				/* skip CR */
			} else if (c == '\n') {
				dynAppendRaw(db, "\\n", 2);
			} else if (c >= 0x20 && c != 0x7f) {
				char ch = (char)c;
				dynAppendRaw(db, &ch, 1);
			}
		}
	}
	dynAppendRaw(db, "\"", 1);
}

static void getParamOrDef(const char *key, const char *def, char *out, size_t outSize)
{
	if (out == NULL || outSize == 0) {
		return;
	}
	memset(out, 0, outSize);
	if (key != NULL) {
		MT_Get_Param((char *)key, out, outSize - 1);
		out[outSize - 1] = '\0';
	}
	if (out[0] == '\0' && def != NULL) {
		snprintf(out, outSize, "%s", def);
		out[outSize - 1] = '\0';
	}
}

static void readKvFromFile(const char *path, const char *key, const char *def,
	char *out, size_t outSize)
{
	FILE *fp;
	char line[512];
	size_t keyLen;
	int found = 0;

	if (out == NULL || outSize == 0) {
		return;
	}
	out[0] = '\0';
	if (path != NULL && key != NULL) {
		keyLen = strlen(key);
		fp = fopen(path, "r");
		if (fp != NULL) {
			while (fgets(line, sizeof(line), fp) != NULL) {
				char *p = line;
				char *eq;
				while (*p == ' ' || *p == '\t') {
					p++;
				}
				if (strncmp(p, key, keyLen) == 0) {
					eq = p + keyLen;
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
						out[outSize - 1] = '\0';
						len = strlen(out);
						while (len > 0 && (out[len - 1] == '\r' || out[len - 1] == '\n' ||
						                   out[len - 1] == ' ' || out[len - 1] == '\t')) {
							out[--len] = '\0';
						}
						found = 1;
					}
				}
			}
			fclose(fp);
		}
	}
	if ((!found || out[0] == '\0') && def != NULL) {
		snprintf(out, outSize, "%s", def);
		out[outSize - 1] = '\0';
	}
}

static void readKvFromSysConf(const char *key, const char *def, char *out, size_t outSize)
{
	struct stat stT, stJ;
	int hasT, hasJ;
	const char *order[4];
	int nOrder = 0;
	int i;

	if (out == NULL || outSize == 0) {
		return;
	}
	out[0] = '\0';
	hasT = (stat("/tmp/sys.conf", &stT) == 0);
	hasJ = (stat("/mnt/jffs2/sys.conf", &stJ) == 0);
	if (hasT && (!hasJ || stT.st_mtime > stJ.st_mtime)) {
		order[nOrder++] = "/tmp/sys.conf";
		if (hasJ) order[nOrder++] = "/mnt/jffs2/sys.conf";
	} else {
		if (hasJ) order[nOrder++] = "/mnt/jffs2/sys.conf";
		if (hasT) order[nOrder++] = "/tmp/sys.conf";
	}
	order[nOrder++] = "/etc/sys.conf";
	order[nOrder++] = "/root/mtlk/etc/sys.conf";
	for (i = 0; i < nOrder; i++) {
		char tmp[256];
		readKvFromFile(order[i], key, "", tmp, sizeof(tmp));
		if (tmp[0] != '\0') {
			snprintf(out, outSize, "%s", tmp);
			out[outSize - 1] = '\0';
			return;
		}
	}
	if (def != NULL) {
		snprintf(out, outSize, "%s", def);
		out[outSize - 1] = '\0';
	}
}

static int readFileTrimmed(const char *path, char *out, size_t outSize, int trimAllSpaces)
{
	FILE *fp;
	size_t n;

	if (out == NULL || outSize == 0) {
		return 0;
	}
	out[0] = '\0';
	if (path == NULL) {
		return 0;
	}
	fp = fopen(path, "r");
	if (fp == NULL) {
		return 0;
	}
	n = fread(out, 1, outSize - 1, fp);
	fclose(fp);
	out[n] = '\0';
	while (n > 0 && (out[n - 1] == '\n' || out[n - 1] == '\r' ||
	                 (trimAllSpaces && (out[n - 1] == ' ' || out[n - 1] == '\t')))) {
		out[--n] = '\0';
	}
	if (trimAllSpaces && out[0] != '\0') {
		char *p = out;
		while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') {
			p++;
		}
		if (p != out) {
			memmove(out, p, strlen(p) + 1);
		}
	}
	return (out[0] != '\0');
}

static void appendJsonLinesFile(api_dynbuf_t *db, const char *path)
{
	FILE *fp;
	char line[512];
	int first = 1;

	dynAppendRaw(db, "[", 1);
	fp = fopen(path, "r");
	if (fp != NULL) {
		while (fgets(line, sizeof(line), fp) != NULL && !db->oom) {
			char *p = line;
			size_t len;
			while (*p == ' ' || *p == '\t') {
				p++;
			}
			if (*p == '\0' || *p == '\n' || *p == '\r' || *p == '#') {
				continue;
			}
			len = strlen(p);
			while (len > 0 && (p[len - 1] == '\n' || p[len - 1] == '\r')) {
				p[--len] = '\0';
			}
			if (len == 0) {
				continue;
			}
			if (!first) {
				dynAppendRaw(db, ",", 1);
			}
			first = 0;
			dynAppendJsonStr(db, p);
		}
		fclose(fp);
	}
	dynAppendRaw(db, "]", 1);
}

static void appendActiveLeasesJson(api_dynbuf_t *db)
{
	FILE *fp;
	api_dhcp_lease_t lease;
	int first = 1;

	dynAppendRaw(db, "[", 1);
	fp = fopen("/tmp/udhcpd.leases", "rb");
	if (fp != NULL) {
		while (fread(&lease, sizeof(lease), 1, fp) == 1 && !db->oom) {
			char macStr[24];
			char ipStr[24];
			char expStr[48];
			struct in_addr ina;
			unsigned long expSec;

			snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
				lease.chaddr[0], lease.chaddr[1], lease.chaddr[2],
				lease.chaddr[3], lease.chaddr[4], lease.chaddr[5]);
			ina.s_addr = (in_addr_t)lease.yiaddr;
			snprintf(ipStr, sizeof(ipStr), "%s", inet_ntoa(ina));
			ipStr[sizeof(ipStr) - 1] = '\0';
			expSec = (unsigned long)ntohl(lease.expires);
			if (expSec == 0UL) {
				snprintf(expStr, sizeof(expStr), "expired");
			} else {
				unsigned long d = expSec / 86400UL;
				unsigned long rem = expSec % 86400UL;
				unsigned long h = rem / 3600UL;
				unsigned long m = (rem % 3600UL) / 60UL;
				unsigned long s = rem % 60UL;
				if (d > 0UL) {
					snprintf(expStr, sizeof(expStr), "%lu days %02lu:%02lu:%02lu", d, h, m, s);
				} else {
					snprintf(expStr, sizeof(expStr), "%02lu:%02lu:%02lu", h, m, s);
				}
			}
			if (!first) {
				dynAppendRaw(db, ",", 1);
			}
			first = 0;
			dynAppendFmt(db, "{\"mac\":\"%s\",\"ip\":\"%s\",\"expires\":\"%s\"}",
				macStr, ipStr, expStr);
		}
		fclose(fp);
	}
	dynAppendRaw(db, "]", 1);
}

static int isPidFileAlive(const char *path)
{
	FILE *fp;
	char buf[32];
	long pid = 0;

	fp = fopen(path, "r");
	if (fp == NULL) {
		return 0;
	}
	memset(buf, 0, sizeof(buf));
	if (fgets(buf, sizeof(buf) - 1, fp) != NULL) {
		char *p = buf;
		while (*p >= '0' && *p <= '9') {
			pid = pid * 10L + (*p - '0');
			p++;
		}
		if (*p != '\0' && *p != '\n' && *p != '\r' && *p != ' ' && *p != '\t') {
			pid = 0;
		}
	}
	fclose(fp);
	if (pid <= 1L) {
		return 0;
	}
	return (kill((pid_t)pid, 0) == 0) ? 1 : 0;
}

static void scanLiveProcAndTcp(int *hasUdhcpd, int *hasDropbear, int *hasTelnetd,
	int *hasUpnpd, int *hasDdns, int *hasWatchdog,
	int *hasSshjWatchdog, int *hasSshjClient,
	int *listenSsh, int *listenTelnet, int *estabRemoteSsh)
{
	DIR *dir;
	struct dirent *ent;
	FILE *fp;
	char line[256];

	*hasUdhcpd = 0;
	*hasDropbear = 0;
	*hasTelnetd = 0;
	*hasUpnpd = 0;
	*hasDdns = 0;
	*hasWatchdog = 0;
	*hasSshjWatchdog = 0;
	*hasSshjClient = 0;
	*listenSsh = 0;
	*listenTelnet = 0;
	*estabRemoteSsh = 0;

	dir = opendir("/proc");
	if (dir != NULL) {
		while ((ent = readdir(dir)) != NULL) {
			const char *dn = ent->d_name;
			char cmdPath[64];
			char cmdBuf[256];
			size_t i, n;
			int allDigits = 1;

			if (dn[0] == '\0') {
				continue;
			}
			for (i = 0; dn[i] != '\0'; i++) {
				if (!isdigit((unsigned char)dn[i])) {
					allDigits = 0;
					break;
				}
			}
			if (!allDigits) {
				continue;
			}
			snprintf(cmdPath, sizeof(cmdPath), "/proc/%s/cmdline", dn);
			fp = fopen(cmdPath, "rb");
			if (fp != NULL) {
				n = fread(cmdBuf, 1, sizeof(cmdBuf) - 1, fp);
				fclose(fp);
				if (n > 0) {
					for (i = 0; i < n; i++) {
						if (cmdBuf[i] == '\0') {
							cmdBuf[i] = ' ';
						}
					}
					cmdBuf[n] = '\0';
					if (strstr(cmdBuf, "udhcpd") != NULL) *hasUdhcpd = 1;
					if (strstr(cmdBuf, "dropbear") != NULL) *hasDropbear = 1;
					if (strstr(cmdBuf, "telnetd") != NULL) *hasTelnetd = 1;
					if (strstr(cmdBuf, "upnpd") != NULL) *hasUpnpd = 1;
					if (strstr(cmdBuf, "ddns_updater.sh") != NULL) *hasDdns = 1;
					if (strstr(cmdBuf, "net_watchdog.sh") != NULL) *hasWatchdog = 1;
					if (strstr(cmdBuf, "sshj_tunnel.sh") != NULL) *hasSshjWatchdog = 1;
					if ((strstr(cmdBuf, "ssh") != NULL || strstr(cmdBuf, "dbclient") != NULL) &&
					    strstr(cmdBuf, "ssh-j.com") != NULL) {
						*hasSshjClient = 1;
					}
				}
			}
		}
		closedir(dir);
	}

	fp = fopen("/proc/net/tcp", "r");
	if (fp != NULL) {
		if (fgets(line, sizeof(line), fp) != NULL) {
			while (fgets(line, sizeof(line), fp) != NULL) {
				unsigned int locPort = 0, remPort = 0, st = 0;
				if (sscanf(line, "%*d: %*x:%x %*x:%x %x", &locPort, &remPort, &st) == 3) {
					if (st == 0x0AU && locPort == 0x0016U) {
						*listenSsh = 1;
					} else if (st == 0x0AU && locPort == 0x0017U) {
						*listenTelnet = 1;
					} else if (st == 0x01U && remPort == 0x0016U) {
						*estabRemoteSsh = 1;
					}
				}
			}
		}
		fclose(fp);
	}
}

static void readLiveWlanRts(char *out, size_t outSize)
{
	int fd;
	struct api_iwreq wrq;

	if (out == NULL || outSize == 0) {
		return;
	}
	out[0] = '\0';
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd >= 0) {
		memset(&wrq, 0, sizeof(wrq));
		strncpy(wrq.if_name, "wlan0", IFNAMSIZ - 1);
		if (ioctl(fd, API_SIOCGIWRTS, &wrq) == 0 && !wrq.u.rts.disabled && wrq.u.rts.value > 0) {
			snprintf(out, outSize, "%d", wrq.u.rts.value);
			out[outSize - 1] = '\0';
		}
		close(fd);
	}
	if (out[0] == '\0') {
		getParamOrDef("RTSThreshold", "1600", out, outSize);
	}
}

static int countFileLines(const char *path)
{
	FILE *fp;
	char line[256];
	int count = 0;

	fp = fopen(path, "r");
	if (fp == NULL) {
		return 0;
	}
	while (fgets(line, sizeof(line), fp) != NULL) {
		size_t len = strlen(line);
		if (len > 0 && line[len - 1] == '\n') {
			count++;
		}
	}
	fclose(fp);
	return count;
}

static int readProcFirstLine(const char *path, char *out, size_t outSize)
{
	FILE *fp;
	char buf[128];
	char *p;
	size_t len;

	if (out == NULL || outSize == 0) {
		return 0;
	}
	out[0] = '\0';
	if (path == NULL) {
		return 0;
	}
	fp = fopen(path, "r");
	if (fp == NULL) {
		return 0;
	}
	if (fgets(buf, sizeof(buf), fp) != NULL) {
		p = buf;
		while (*p == ' ' || *p == '\t') {
			p++;
		}
		snprintf(out, outSize, "%s", p);
		out[outSize - 1] = '\0';
		len = strlen(out);
		while (len > 0 && (out[len - 1] == '\n' || out[len - 1] == '\r' ||
		                   out[len - 1] == ' ' || out[len - 1] == '\t')) {
			out[--len] = '\0';
		}
	}
	fclose(fp);
	return (out[0] != '\0');
}

static void readLiveDefaultGw(char *out, size_t outSize)
{
	FILE *fp;
	char line[256];

	if (out == NULL || outSize == 0) {
		return;
	}
	out[0] = '\0';
	fp = fopen("/proc/net/route", "r");
	if (fp == NULL) {
		return;
	}
	if (fgets(line, sizeof(line), fp) != NULL) {
		while (fgets(line, sizeof(line), fp) != NULL) {
			char ifn[32];
			unsigned long dstHex = 1, gwHex = 0;
			unsigned int flags = 0;
			if (sscanf(line, "%31s %lx %lx %x", ifn, &dstHex, &gwHex, &flags) == 4) {
				if (dstHex == 0 && (flags & 1) && gwHex != 0) {
					struct in_addr ina;
					ina.s_addr = (in_addr_t)gwHex;
					snprintf(out, outSize, "%s", inet_ntoa(ina));
					out[outSize - 1] = '\0';
					break;
				}
			}
		}
	}
	fclose(fp);
}

static void readLiveResolvDns(char *d1, size_t d1Sz, char *d2, size_t d2Sz)
{
	FILE *fp;
	char line[128];

	if (d1 != NULL && d1Sz > 0) d1[0] = '\0';
	if (d2 != NULL && d2Sz > 0) d2[0] = '\0';
	fp = fopen("/tmp/resolv.conf", "r");
	if (fp == NULL) {
		fp = fopen("/etc/resolv.conf", "r");
	}
	if (fp == NULL) {
		return;
	}
	while (fgets(line, sizeof(line), fp) != NULL) {
		char ip[32];
		if (sscanf(line, " nameserver %31s", ip) == 1) {
			if (d1 != NULL && d1[0] == '\0') {
				snprintf(d1, d1Sz, "%s", ip);
				d1[d1Sz - 1] = '\0';
			} else if (d2 != NULL && d2[0] == '\0' && (d1 == NULL || strcmp(d1, ip) != 0)) {
				snprintf(d2, d2Sz, "%s", ip);
				d2[d2Sz - 1] = '\0';
				break;
			}
		}
	}
	fclose(fp);
}

static void readLiveWlanIoctl(char *essidOut, size_t essidSz,
	char *chOut, size_t chSz, char *bandOut, size_t bandSz,
	char *bssidOut, size_t bssidSz)
{
	int fd;
	struct api_iwreq wrq;
	char essidBuf[36];

	if (essidOut != NULL && essidSz > 0) essidOut[0] = '\0';
	if (chOut != NULL && chSz > 0) chOut[0] = '\0';
	if (bandOut != NULL && bandSz > 0) bandOut[0] = '\0';
	if (bssidOut != NULL && bssidSz > 0) bssidOut[0] = '\0';

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		return;
	}

	if (essidOut != NULL && essidSz > 0) {
		memset(&wrq, 0, sizeof(wrq));
		memset(essidBuf, 0, sizeof(essidBuf));
		strncpy(wrq.if_name, "wlan0", IFNAMSIZ - 1);
		wrq.u.essid.pointer = essidBuf;
		wrq.u.essid.length = (unsigned short)(sizeof(essidBuf) - 1);
		if (ioctl(fd, API_SIOCGIWESSID, &wrq) == 0 && essidBuf[0] != '\0') {
			size_t len = (wrq.u.essid.length < sizeof(essidBuf)) ? wrq.u.essid.length : (sizeof(essidBuf) - 1);
			essidBuf[len] = '\0';
			snprintf(essidOut, essidSz, "%s", essidBuf);
			essidOut[essidSz - 1] = '\0';
		}
	}

	if ((chOut != NULL && chSz > 0) || (bandOut != NULL && bandSz > 0)) {
		memset(&wrq, 0, sizeof(wrq));
		strncpy(wrq.if_name, "wlan0", IFNAMSIZ - 1);
		if (ioctl(fd, API_SIOCGIWFREQ, &wrq) == 0 && wrq.u.freq.m > 0) {
			long val = (long)wrq.u.freq.m;
			int exp = (int)wrq.u.freq.e;
			int ch = 0;
			int band = -1;

			if (exp == 0 && val >= 1 && val <= 200) {
				ch = (int)val;
				band = (ch > 14) ? 0 : 1;
			} else {
				while (exp > 6 && val < 10000000L) { val *= 10L; exp--; }
				while (exp < 6 && val > 0L) { val /= 10L; exp++; }
				if (val >= 2412L && val <= 2484L) {
					ch = (val == 2484L) ? 14 : (int)((val - 2407L) / 5L);
					band = 1;
				} else if (val >= 5000L && val <= 5900L) {
					ch = (int)((val - 5000L) / 5L);
					band = 0;
				}
			}
			if (ch > 0 && chOut != NULL && chSz > 0) {
				snprintf(chOut, chSz, "%d", ch);
				chOut[chSz - 1] = '\0';
			}
			if (band >= 0 && bandOut != NULL && bandSz > 0) {
				snprintf(bandOut, bandSz, "%d", band);
				bandOut[bandSz - 1] = '\0';
			}
		}
	}

	if (bssidOut != NULL && bssidSz > 0) {
		memset(&wrq, 0, sizeof(wrq));
		strncpy(wrq.if_name, "wlan0", IFNAMSIZ - 1);
		if (ioctl(fd, API_SIOCGIWAP, &wrq) == 0) {
			const unsigned char *m = (const unsigned char *)wrq.u.ap_addr.sa_data;
			if ((m[0] | m[1] | m[2] | m[3] | m[4] | m[5]) != 0) {
				snprintf(bssidOut, bssidSz, "%02X:%02X:%02X:%02X:%02X:%02X",
					m[0], m[1], m[2], m[3], m[4], m[5]);
				bssidOut[bssidSz - 1] = '\0';
			}
		}
	}

	close(fd);
}

static void readFwDate(char *out, size_t outSize)
{
	FILE *fp;
	char line[256];
	char *p;
	size_t len;

	if (out == NULL || outSize == 0) {
		return;
	}
	out[0] = '\0';
	fp = fopen("/root/mtlk/web/fw_version.txt", "r");
	if (fp != NULL) {
		while (fgets(line, sizeof(line), fp) != NULL) {
			if (strncmp(line, "ProjectFirmwareVersionDate=", 27) == 0) {
				p = line + 27;
				while (*p == ' ' || *p == '\t' || *p == '"') {
					p++;
				}
				snprintf(out, outSize, "%s", p);
				len = strlen(out);
				while (len > 0 && (out[len - 1] == '\n' || out[len - 1] == '\r' ||
				                   out[len - 1] == '"' || out[len - 1] == ' ')) {
					out[--len] = '\0';
				}
				break;
			}
		}
		fclose(fp);
	}
}

static int websApiHandler(webs_t wp, char_t *urlPrefix, char_t *webDir,
	int arg, char_t *url, char_t *path, char_t *query)
{
	char body[8192];
	FILE *fp;
	char line[256];

	(void)urlPrefix;
	(void)webDir;
	(void)arg;
	(void)url;
	(void)query;

	if (path == NULL) {
		return 0;
	}

	syncConfigFromDiskIfChanged();

	if (strcmp(path, "/api/status.json") == 0) {
		unsigned long uptimeSec = 0;
		unsigned long memTotal = 0, memFree = 0, buffers = 0, cached = 0;
		unsigned long commitLimit = 0, committedAs = 0;
		int tcpEstab = 0, tcpListen = 0, tcpCloseWait = 0;
		int busy;
		char hostRaw[64], hostEsc[128], netType[16], netTypeEsc[32], verRaw[96], verEsc[192];
		char loadStr[64], loadEsc[96];
		char ipRaw[32], maskRaw[32], macRaw[32], gwRaw[32], d1Raw[32], d2Raw[32];
		char ipEsc[48], maskEsc[48], macEsc[48], gwEsc[48], d1Esc[48], d2Esc[48];

		memset(hostRaw, 0, sizeof(hostRaw));
		memset(netType, 0, sizeof(netType));
		memset(verRaw, 0, sizeof(verRaw));
		memset(loadStr, 0, sizeof(loadStr));
		memset(ipRaw, 0, sizeof(ipRaw));
		memset(maskRaw, 0, sizeof(maskRaw));
		memset(macRaw, 0, sizeof(macRaw));
		memset(gwRaw, 0, sizeof(gwRaw));
		memset(d1Raw, 0, sizeof(d1Raw));
		memset(d2Raw, 0, sizeof(d2Raw));
		if (gethostname(hostRaw, sizeof(hostRaw) - 1) != 0 ||
		    hostRaw[0] == '\0' || strcmp(hostRaw, "(none)") == 0) {
			MT_Get_Param("HostName", hostRaw, sizeof(hostRaw) - 1);
			hostRaw[sizeof(hostRaw) - 1] = '\0';
		}
		MT_Get_Param("network_type", netType, sizeof(netType) - 1);
		netType[sizeof(netType) - 1] = '\0';
		readLiveDefaultGw(gwRaw, sizeof(gwRaw));
		if (gwRaw[0] == '\0') {
			MT_Get_Param("default_gw", gwRaw, sizeof(gwRaw) - 1);
			gwRaw[sizeof(gwRaw) - 1] = '\0';
		}
		readLiveResolvDns(d1Raw, sizeof(d1Raw), d2Raw, sizeof(d2Raw));
		if (d1Raw[0] == '\0') {
			MT_Get_Param("DHCPDDNS1", d1Raw, sizeof(d1Raw) - 1);
			d1Raw[sizeof(d1Raw) - 1] = '\0';
		}
		if (d2Raw[0] == '\0') {
			MT_Get_Param("DHCPDDNS2", d2Raw, sizeof(d2Raw) - 1);
			d2Raw[sizeof(d2Raw) - 1] = '\0';
		}
		jsonEscapeCopy(hostEsc, sizeof(hostEsc), hostRaw);
		jsonEscapeCopy(netTypeEsc, sizeof(netTypeEsc), netType);
		jsonEscapeCopy(gwEsc, sizeof(gwEsc), gwRaw);
		jsonEscapeCopy(d1Esc, sizeof(d1Esc), d1Raw);
		jsonEscapeCopy(d2Esc, sizeof(d2Esc), d2Raw);

		getIfaceIpv4AndMask("br0", ipRaw, sizeof(ipRaw), maskRaw, sizeof(maskRaw));
		if (ipRaw[0] == '\0') {
			MT_Get_Param("ip_lan", ipRaw, sizeof(ipRaw) - 1);
			ipRaw[sizeof(ipRaw) - 1] = '\0';
		}
		if (maskRaw[0] == '\0') {
			MT_Get_Param("subnet_lan", maskRaw, sizeof(maskRaw) - 1);
			maskRaw[sizeof(maskRaw) - 1] = '\0';
		}
		getIfaceMac("br0", macRaw, sizeof(macRaw));
		if (macRaw[0] == '\0') {
			MT_Get_Param("mac_lan", macRaw, sizeof(macRaw) - 1);
			macRaw[sizeof(macRaw) - 1] = '\0';
		}
		jsonEscapeCopy(ipEsc, sizeof(ipEsc), ipRaw);
		jsonEscapeCopy(maskEsc, sizeof(maskEsc), maskRaw);
		jsonEscapeCopy(macEsc, sizeof(macEsc), macRaw);

		busy = (isWaitOtherProcessBusy() != 0 || access("/var/old_web_pid", F_OK) == 0) ? 1 : 0;

		fp = fopen("/proc/uptime", "r");
		if (fp != NULL) {
			if (fgets(line, sizeof(line), fp) != NULL) {
				sscanf(line, "%lu", &uptimeSec);
			}
			fclose(fp);
		}

		fp = fopen("/proc/loadavg", "r");
		if (fp != NULL) {
			if (fgets(line, sizeof(line), fp) != NULL) {
				char l1[16], l5[16], l15[16];
				if (sscanf(line, "%15s %15s %15s", l1, l5, l15) == 3) {
					snprintf(loadStr, sizeof(loadStr), "%s %s %s", l1, l5, l15);
				}
			}
			fclose(fp);
		}
		jsonEscapeCopy(loadEsc, sizeof(loadEsc), loadStr);

		fp = fopen("/proc/meminfo", "r");
		if (fp != NULL) {
			while (fgets(line, sizeof(line), fp) != NULL) {
				unsigned long val = 0;
				if (sscanf(line, "MemTotal: %lu", &val) == 1) memTotal = val;
				else if (sscanf(line, "MemFree: %lu", &val) == 1) memFree = val;
				else if (sscanf(line, "Buffers: %lu", &val) == 1) buffers = val;
				else if (sscanf(line, "Cached: %lu", &val) == 1) cached = val;
				else if (sscanf(line, "CommitLimit: %lu", &val) == 1) commitLimit = val;
				else if (sscanf(line, "Committed_AS: %lu", &val) == 1) committedAs = val;
			}
			fclose(fp);
		}

		fp = fopen("/proc/net/tcp", "r");
		if (fp != NULL) {
			if (fgets(line, sizeof(line), fp) != NULL) {
				while (fgets(line, sizeof(line), fp) != NULL) {
					unsigned int st = 0;
					if (sscanf(line, "%*d: %*x:%*x %*x:%*x %x", &st) == 1) {
						if (st == 1) tcpEstab++;
						else if (st == 10) tcpListen++;
						else if (st == 8) tcpCloseWait++;
					}
				}
			}
			fclose(fp);
		}

		fp = fopen("/etc/version", "r");
		if (fp != NULL) {
			if (fgets(verRaw, sizeof(verRaw), fp) != NULL) {
				size_t len = strlen(verRaw);
				while (len > 0 && (verRaw[len - 1] == '\n' || verRaw[len - 1] == '\r')) {
					verRaw[--len] = '\0';
				}
			}
			fclose(fp);
		}
		jsonEscapeCopy(verEsc, sizeof(verEsc), verRaw);

		snprintf(body, sizeof(body),
			"{\"model\":\"WAP610N\",\"hostname\":\"%s\",\"network_type\":\"%s\","
			"\"version\":\"%s\",\"uptime_sec\":%lu,\"busy\":%d,\"load\":\"%s\","
			"\"ip\":\"%s\",\"mask\":\"%s\",\"mac\":\"%s\",\"gateway\":\"%s\","
			"\"dns1\":\"%s\",\"dns2\":\"%s\","
			"\"mem_kb\":{\"total\":%lu,\"free\":%lu,\"buffers\":%lu,\"cached\":%lu,"
			"\"commit_limit\":%lu,\"committed_as\":%lu},"
			"\"tcp\":{\"established\":%d,\"listen\":%d,\"close_wait\":%d}}\n",
			hostEsc, netTypeEsc, verEsc, uptimeSec, busy, loadEsc,
			ipEsc, maskEsc, macEsc, gwEsc, d1Esc, d2Esc,
			memTotal, memFree, buffers, cached, commitLimit, committedAs,
			tcpEstab, tcpListen, tcpCloseWait);
		body[sizeof(body) - 1] = '\0';
		sendJsonResponse(wp, 200, body);
		return 1;
	}

	if (strcmp(path, "/api/config.json") == 0) {
		static const char * const paramMap[][2] = {
			{"network_type", "network_type"},
			{"ip_config_method", "ip_config_method"},
			{"hostname", "HostName"},
			{"ip_lan", "ip_lan"},
			{"subnet_lan", "subnet_lan"},
			{"default_gw", "default_gw"},
			{"dns1", "DHCPDDNS1"},
			{"dns2", "DHCPDDNS2"},
			{"mac_lan", "mac_lan"},
			{"language", "Language"},
			{"frequency_band", "FrequencyBand"},
			{"country", "Country"},
			{"channel_bonding", "ChannelBonding"},
			{"upper_lower", "UpperLowerChannelBonding"},
			{"essid", "NonProc_ESSID"},
			{"security_mode", "NonProcSecurityMode"},
			{"wep_key_len", "NonProc_WepKeyLength"},
			{"wep_tx_idx", "WepTxKeyIdx"},
			{"wep_auth", "NonProc_Authentication"},
			{"wep_key0", "WepKeys_DefaultKey0"},
			{"wep_key1", "WepKeys_DefaultKey1"},
			{"wep_key2", "WepKeys_DefaultKey2"},
			{"wep_key3", "WepKeys_DefaultKey3"},
			{"wpa_pers_mode", "NonProc_WPA_Personal_Mode"},
			{"wpa_pers_enc", "NonProc_WPA_Personal_Encapsulation"},
			{"wpa_psk", "NonProc_WPA_Personal_PSK"},
			{"wpa_ent_mode", "NonProc_WPA_Enterprise_Mode"},
			{"wpa_ent_enc", "NonProc_WPA_Enterprise_Encapsulation"},
			{"wpa_rad_ip", "NonProc_WPA_Enterprise_Radius_IP"},
			{"wpa_rad_port", "NonProc_WPA_Enterprise_Radius_Port"},
			{"wpa_rad_key", "NonProc_WPA_Enterprise_Radius_Key"},
			{"wpa_rad_rekey", "NonProc_WPA_Enterprise_Radius_ReKey_Interval"},
			{"wps_active", "NonProc_WPS_ActivateWPS"},
			{"wps_pin", "NonProc_WPS_DevicePIN"},
			{"radar_detect", "11hRadarDetect"},
			{"aocs_restrict_ch", "AocsRestrictCh"},
			{"ldpc", "AdvancedCoding"},
			{"hidden_ssid", "HiddenSSID"},
			{"erp_prot", "ERPProtectionType"},
			{"obss_prot", "OverlappingBSSProtection"},
			{"n_prot", "11nProtectionType"},
			{"ap_forwarding", "APforwarding"},
			{"reliable_mcast", "ReliableMulticast"},
			{"tx_power", "PowerSelection"},
			{"use_11q_map", "Use11QMap"},
			{"eth_down_up", "EthDownUp"},
			{"auth_timeout", "AuthenticationTimeout"},
			{"wireless_mgmt", "WirelessMgmtEnabled"}
		};
		size_t pos = 0;
		size_t idx;
		size_t total = sizeof(paramMap) / sizeof(paramMap[0]);
		char esc[256];
		char cfgChRaw[24], cfgChEsc[48];
		char chRaw[24], chEsc[48];
		char bssidRaw[32], bssidEsc[64];
		char fwDateRaw[96], fwDateEsc[192];
		char liveEssid[64], liveBand[16], liveCountry[16], liveCountryEsc[32], liveHost[64];
		char liveD1[32], liveD2[32], liveGw[32], liveIp[32], liveMask[32];

		memset(cfgChRaw, 0, sizeof(cfgChRaw));
		memset(chRaw, 0, sizeof(chRaw));
		memset(bssidRaw, 0, sizeof(bssidRaw));
		memset(fwDateRaw, 0, sizeof(fwDateRaw));
		memset(liveEssid, 0, sizeof(liveEssid));
		memset(liveBand, 0, sizeof(liveBand));
		memset(liveCountry, 0, sizeof(liveCountry));
		memset(liveHost, 0, sizeof(liveHost));
		memset(liveD1, 0, sizeof(liveD1));
		memset(liveD2, 0, sizeof(liveD2));
		memset(liveGw, 0, sizeof(liveGw));
		memset(liveIp, 0, sizeof(liveIp));
		memset(liveMask, 0, sizeof(liveMask));
		readLiveWlanIoctl(liveEssid, sizeof(liveEssid), chRaw, sizeof(chRaw),
			liveBand, sizeof(liveBand), bssidRaw, sizeof(bssidRaw));
		readProcFirstLine("/proc/sys/dev/mtlk/wlan0/Country", liveCountry, sizeof(liveCountry));
		if (gethostname(liveHost, sizeof(liveHost) - 1) != 0 || strcmp(liveHost, "(none)") == 0) {
			liveHost[0] = '\0';
		}
		getIfaceIpv4AndMask("br0", liveIp, sizeof(liveIp), liveMask, sizeof(liveMask));
		readLiveDefaultGw(liveGw, sizeof(liveGw));
		readLiveResolvDns(liveD1, sizeof(liveD1), liveD2, sizeof(liveD2));

		pos = jsonAppend(body, sizeof(body), pos, "{");
		for (idx = 0; idx < total; idx++) {
			char procBuf[128];
			char procPath[128];
			const char *k = paramMap[idx][0];
			const char *p = paramMap[idx][1];
			getParamEsc(p, esc, sizeof(esc));
			if (strcmp(k, "country") == 0 && liveCountry[0] != '\0') {
				jsonEscapeCopy(esc, sizeof(esc), liveCountry);
			} else if (strcmp(k, "essid") == 0 && liveEssid[0] != '\0') {
				jsonEscapeCopy(esc, sizeof(esc), liveEssid);
			} else if (strcmp(k, "frequency_band") == 0 && liveBand[0] != '\0') {
				jsonEscapeCopy(esc, sizeof(esc), liveBand);
			} else if (strcmp(k, "hostname") == 0 && liveHost[0] != '\0') {
				jsonEscapeCopy(esc, sizeof(esc), liveHost);
			} else if (strcmp(k, "ip_lan") == 0 && liveIp[0] != '\0') {
				jsonEscapeCopy(esc, sizeof(esc), liveIp);
			} else if (strcmp(k, "subnet_lan") == 0 && liveMask[0] != '\0') {
				jsonEscapeCopy(esc, sizeof(esc), liveMask);
			} else if (strcmp(k, "default_gw") == 0 && liveGw[0] != '\0') {
				jsonEscapeCopy(esc, sizeof(esc), liveGw);
			} else if (strcmp(k, "dns1") == 0 && esc[0] == '\0' && liveD1[0] != '\0') {
				jsonEscapeCopy(esc, sizeof(esc), liveD1);
			} else if (strcmp(k, "dns2") == 0 && esc[0] == '\0' && liveD2[0] != '\0') {
				jsonEscapeCopy(esc, sizeof(esc), liveD2);
			} else if (strcmp(k, "channel_bonding") == 0 || strcmp(k, "upper_lower") == 0 ||
			           strcmp(k, "hidden_ssid") == 0 || strcmp(k, "ap_forwarding") == 0 ||
			           strcmp(k, "tx_power") == 0 || strcmp(k, "ldpc") == 0 ||
			           strcmp(k, "erp_prot") == 0 || strcmp(k, "n_prot") == 0 ||
			           strcmp(k, "obss_prot") == 0 || strcmp(k, "reliable_mcast") == 0 ||
			           strcmp(k, "radar_detect") == 0 || strcmp(k, "use_11q_map") == 0) {
				snprintf(procPath, sizeof(procPath), "/proc/sys/dev/mtlk/wlan0/%s", p);
				if (readProcFirstLine(procPath, procBuf, sizeof(procBuf))) {
					jsonEscapeCopy(esc, sizeof(esc), procBuf);
				}
			}
			pos = jsonAppend(body, sizeof(body), pos, "%s\"%s\":\"%s\"",
				(idx == 0) ? "" : ",", k, esc);
		}
		MT_Get_Param("Channel", cfgChRaw, sizeof(cfgChRaw) - 1);
		cfgChRaw[sizeof(cfgChRaw) - 1] = '\0';
		if (chRaw[0] == '\0') {
			snprintf(chRaw, sizeof(chRaw), "%s", cfgChRaw);
			chRaw[sizeof(chRaw) - 1] = '\0';
		}
		if (bssidRaw[0] == '\0') {
			getIfaceMac("wlan0", bssidRaw, sizeof(bssidRaw));
			if (bssidRaw[0] == '\0') {
				MT_Get_Param("BSSID", bssidRaw, sizeof(bssidRaw) - 1);
				bssidRaw[sizeof(bssidRaw) - 1] = '\0';
			}
		}
		if (liveCountry[0] == '\0') {
			MT_Get_Param("Country", liveCountry, sizeof(liveCountry) - 1);
			liveCountry[sizeof(liveCountry) - 1] = '\0';
		}
		jsonEscapeCopy(cfgChEsc, sizeof(cfgChEsc), cfgChRaw);
		jsonEscapeCopy(chEsc, sizeof(chEsc), chRaw);
		jsonEscapeCopy(bssidEsc, sizeof(bssidEsc), bssidRaw);
		jsonEscapeCopy(liveCountryEsc, sizeof(liveCountryEsc), liveCountry);
		readFwDate(fwDateRaw, sizeof(fwDateRaw));
		jsonEscapeCopy(fwDateEsc, sizeof(fwDateEsc), fwDateRaw);
		pos = jsonAppend(body, sizeof(body), pos,
			",\"channel\":\"%s\",\"live_channel\":\"%s\",\"live_country\":\"%s\",\"bssid\":\"%s\",\"fw_date\":\"%s\"}\n",
			cfgChEsc, chEsc, liveCountryEsc, bssidEsc, fwDateEsc);
		body[sizeof(body) - 1] = '\0';
		if (pos >= sizeof(body) - 1) {
			sendJsonResponse(wp, 500, "{\"error\":\"config_json_truncated\"}\n");
			return 1;
		}
		sendJsonResponse(wp, 200, body);
		return 1;
	}

	if (strcmp(path, "/api/wmm.json") == 0) {
		static const char * const acNames[4] = {"AC_BK", "AC_BE", "AC_VI", "AC_VO"};
		static const char * const acFields[16] = {
			"UseAggregation", "AcceptAggregation", "MaxNumOfPackets", "MaxNumOfBytes",
			"TimeoutInterval", "MinSizeOfPacketInAggr", "ADDBATimeout", "AggregationWindowSize",
			"CWmin", "CWmax", "AIFSN", "TXOP",
			"CWmin_AP", "CWmax_AP", "AIFSN_AP", "TXOP_AP"
		};
		char curWmmEsc[32], esc[64], pname[64];
		size_t pos = 0;
		size_t i, j;

		getParamEsc("CurrentWMM", curWmmEsc, sizeof(curWmmEsc));
		pos = jsonAppend(body, sizeof(body), pos, "{\"current_wmm\":\"%s\",\"ac\":{", curWmmEsc);
		for (i = 0; i < 4; i++) {
			pos = jsonAppend(body, sizeof(body), pos, "%s\"%s\":{",
				(i == 0) ? "" : ",", acNames[i]);
			for (j = 0; j < 16; j++) {
				snprintf(pname, sizeof(pname), "%s_%s", acNames[i], acFields[j]);
				getParamEsc(pname, esc, sizeof(esc));
				pos = jsonAppend(body, sizeof(body), pos, "%s\"%s\":\"%s\"",
					(j == 0) ? "" : ",", acFields[j], esc);
			}
			pos = jsonAppend(body, sizeof(body), pos, "}");
		}
		pos = jsonAppend(body, sizeof(body), pos, "}}\n");
		body[sizeof(body) - 1] = '\0';
		sendJsonResponse(wp, 200, body);
		return 1;
	}

	if (strcmp(path, "/api/clients.json") == 0) {
		api_arp_entry_t arpList[64];
		int arpCount = 0;
		api_dhcp_lease_t lease;
		struct {
			char mac[20];
			char ip[20];
			char name[64];
		} stLeases[32];
		int stCount = 0;
		api_br_fdb_t recs[32];
		size_t nread, i;
		size_t pos = 0;
		int count = 0;
		int wlsPort = -1;

		fp = fopen("/proc/net/arp", "r");
		if (fp != NULL) {
			if (fgets(line, sizeof(line), fp) != NULL) {
				while (arpCount < 64 && fgets(line, sizeof(line), fp) != NULL) {
					char ipBuf[20], macBuf[20];
					unsigned int flags = 0;
					if (sscanf(line, "%19s 0x%*x 0x%x %19s", ipBuf, &flags, macBuf) == 3) {
						if (flags != 0 && strcmp(macBuf, "00:00:00:00:00:00") != 0) {
							snprintf(arpList[arpCount].ip, sizeof(arpList[arpCount].ip), "%s", ipBuf);
							snprintf(arpList[arpCount].mac, sizeof(arpList[arpCount].mac), "%s", macBuf);
							arpCount++;
						}
					}
				}
			}
			fclose(fp);
		}

		fp = fopen("/tmp/udhcpd.leases", "rb");
		if (fp != NULL) {
			while (arpCount < 64 && fread(&lease, sizeof(lease), 1, fp) == 1) {
				char macBuf[20];
				struct in_addr ina;
				int a, found = 0;
				snprintf(macBuf, sizeof(macBuf), "%02x:%02x:%02x:%02x:%02x:%02x",
					lease.chaddr[0], lease.chaddr[1], lease.chaddr[2],
					lease.chaddr[3], lease.chaddr[4], lease.chaddr[5]);
				for (a = 0; a < arpCount; a++) {
					if (strcasecmp(arpList[a].mac, macBuf) == 0) {
						found = 1;
						break;
					}
				}
				if (!found && lease.yiaddr != 0) {
					ina.s_addr = (in_addr_t)lease.yiaddr;
					snprintf(arpList[arpCount].ip, sizeof(arpList[arpCount].ip), "%s", inet_ntoa(ina));
					snprintf(arpList[arpCount].mac, sizeof(arpList[arpCount].mac), "%s", macBuf);
					arpCount++;
				}
			}
			fclose(fp);
		}

		fp = fopen("/mnt/jffs2/dhcp_static.conf", "r");
		if (fp != NULL) {
			while (stCount < 32 && fgets(line, sizeof(line), fp) != NULL) {
				char *p1, *p2, *p3;
				size_t len = strlen(line);
				while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
					line[--len] = '\0';
				}
				if (line[0] == '\0' || line[0] == '#') {
					continue;
				}
				p1 = line;
				p2 = strchr(p1, '|');
				if (p2 == NULL) continue;
				*p2++ = '\0';
				p3 = strchr(p2, '|');
				if (p3 != NULL) {
					*p3++ = '\0';
				} else {
					p3 = "";
				}
				snprintf(stLeases[stCount].mac, sizeof(stLeases[stCount].mac), "%s", p1);
				snprintf(stLeases[stCount].ip, sizeof(stLeases[stCount].ip), "%s", p2);
				snprintf(stLeases[stCount].name, sizeof(stLeases[stCount].name), "%s", p3);
				stCount++;
			}
			fclose(fp);
		}

		pos = jsonAppend(body, sizeof(body), pos, "{\"clients\":[");
		fp = fopen("/sys/class/net/br0/brforward", "rb");
		if (fp != NULL) {
			wlsPort = getWLSBridgePort();
			while ((nread = fread(recs, sizeof(api_br_fdb_t), 32, fp)) > 0) {
				for (i = 0; i < nread; i++) {
					char macStr[20], nameEsc[128];
					const char *ipStr = "";
					const char *nameRaw = "";
					const char *ifaceStr;
					int a;
					if (recs[i].is_local) {
						continue;
					}
					if (pos + 180 >= sizeof(body)) {
						break;
					}
					snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
						recs[i].mac_addr[0], recs[i].mac_addr[1], recs[i].mac_addr[2],
						recs[i].mac_addr[3], recs[i].mac_addr[4], recs[i].mac_addr[5]);
					for (a = 0; a < arpCount; a++) {
						if (strcasecmp(arpList[a].mac, macStr) == 0) {
							ipStr = arpList[a].ip;
							break;
						}
					}
					for (a = 0; a < stCount; a++) {
						if (strcasecmp(stLeases[a].mac, macStr) == 0) {
							if (ipStr[0] == '\0') ipStr = stLeases[a].ip;
							nameRaw = stLeases[a].name;
							break;
						}
					}
					jsonEscapeCopy(nameEsc, sizeof(nameEsc), nameRaw);
					ifaceStr = (wlsPort > 0 && (int)recs[i].port_no == wlsPort) ? "Wi-Fi" : "LAN";
					pos = jsonAppend(body, sizeof(body), pos,
						"%s{\"mac\":\"%s\",\"ip\":\"%s\",\"name\":\"%s\",\"iface\":\"%s\"}",
						(count == 0) ? "" : ",", macStr, ipStr, nameEsc, ifaceStr);
					count++;
				}
				if (pos + 180 >= sizeof(body)) {
					break;
				}
			}
			fclose(fp);
		}
		pos = jsonAppend(body, sizeof(body), pos, "]}\n");
		body[sizeof(body) - 1] = '\0';
		sendJsonResponse(wp, 200, body);
		return 1;
	}

	if (strcmp(path, "/api/wifi.json") == 0) {
		char essidRaw[64], essidEsc[128];
		char channel[16], channelEsc[32];
		char netMode[16], netModeEsc[32];
		char bonding[16], bondingEsc[32];
		char auth[32], authEsc[64];
		char secMode[16], secModeEsc[32];
		char rfBand[16], rfBandEsc[32];
		char wpsMode[16], wpsModeEsc[32];
		unsigned long wlanRxB = 0, wlanRxP = 0, wlanTxB = 0, wlanTxP = 0;
		unsigned long ethRxB = 0, ethRxP = 0, ethTxB = 0, ethTxP = 0;

		memset(essidRaw, 0, sizeof(essidRaw));
		memset(channel, 0, sizeof(channel));
		memset(netMode, 0, sizeof(netMode));
		memset(bonding, 0, sizeof(bonding));
		memset(auth, 0, sizeof(auth));
		memset(secMode, 0, sizeof(secMode));
		memset(rfBand, 0, sizeof(rfBand));
		memset(wpsMode, 0, sizeof(wpsMode));

		MT_Get_Param("NonProc_ESSID", essidRaw, sizeof(essidRaw) - 1);
		if (essidRaw[0] == '\0') {
			MT_Get_Param("ESSID", essidRaw, sizeof(essidRaw) - 1);
		}
		MT_Get_Param("Channel", channel, sizeof(channel) - 1);
		MT_Get_Param("NetworkMode", netMode, sizeof(netMode) - 1);
		MT_Get_Param("ChannelBonding", bonding, sizeof(bonding) - 1);
		MT_Get_Param("Authentication", auth, sizeof(auth) - 1);
		MT_Get_Param("NonProcSecurityMode", secMode, sizeof(secMode) - 1);
		MT_Get_Param("FrequencyBand", rfBand, sizeof(rfBand) - 1);
		if (rfBand[0] == '\0') {
			MT_Get_Param("RF_Band", rfBand, sizeof(rfBand) - 1);
		}
		{
			char liveEssid[64], liveCh[24], liveBand[16], liveBond[16];
			readLiveWlanIoctl(liveEssid, sizeof(liveEssid), liveCh, sizeof(liveCh),
				liveBand, sizeof(liveBand), NULL, 0);
			if (liveEssid[0] != '\0') {
				snprintf(essidRaw, sizeof(essidRaw), "%s", liveEssid);
			}
			if (liveCh[0] != '\0') {
				snprintf(channel, sizeof(channel), "%s", liveCh);
			}
			if (liveBand[0] != '\0') {
				snprintf(rfBand, sizeof(rfBand), "%s", liveBand);
			}
			if (readProcFirstLine("/proc/sys/dev/mtlk/wlan0/ChannelBonding", liveBond, sizeof(liveBond))) {
				snprintf(bonding, sizeof(bonding), "%s", liveBond);
			}
		}
		MT_Get_Param("NonProc_WPS_ActivateWPS", wpsMode, sizeof(wpsMode) - 1);
		if (wpsMode[0] == '\0') {
			MT_Get_Param("WPS_Mode", wpsMode, sizeof(wpsMode) - 1);
		}
		jsonEscapeCopy(essidEsc, sizeof(essidEsc), essidRaw);
		jsonEscapeCopy(channelEsc, sizeof(channelEsc), channel);
		jsonEscapeCopy(netModeEsc, sizeof(netModeEsc), netMode);
		jsonEscapeCopy(bondingEsc, sizeof(bondingEsc), bonding);
		jsonEscapeCopy(authEsc, sizeof(authEsc), auth);
		jsonEscapeCopy(secModeEsc, sizeof(secModeEsc), secMode);
		jsonEscapeCopy(rfBandEsc, sizeof(rfBandEsc), rfBand);
		jsonEscapeCopy(wpsModeEsc, sizeof(wpsModeEsc), wpsMode);

		fp = fopen("/proc/net/dev", "r");
		if (fp != NULL) {
			while (fgets(line, sizeof(line), fp) != NULL) {
				char *p = strchr(line, ':');
				if (p != NULL) {
					unsigned long rxB = 0, rxP = 0, txB = 0, txP = 0;
					*p = '\0';
					if (sscanf(p + 1, "%lu %lu %*u %*u %*u %*u %*u %*u %lu %lu",
							&rxB, &rxP, &txB, &txP) == 4) {
						if (strstr(line, "wlan0") != NULL) {
							wlanRxB = rxB; wlanRxP = rxP; wlanTxB = txB; wlanTxP = txP;
						} else if (strstr(line, "eth0") != NULL) {
							ethRxB = rxB; ethRxP = rxP; ethTxB = txB; ethTxP = txP;
						}
					}
				}
			}
			fclose(fp);
		}

		snprintf(body, sizeof(body),
			"{\"essid\":\"%s\",\"channel\":\"%s\",\"network_mode\":\"%s\","
			"\"channel_bonding\":\"%s\",\"authentication\":\"%s\",\"security_mode\":\"%s\","
			"\"rf_band\":\"%s\",\"wps_mode\":\"%s\","
			"\"wlan0\":{\"rx_bytes\":%lu,\"rx_packets\":%lu,\"tx_bytes\":%lu,\"tx_packets\":%lu},"
			"\"eth0\":{\"rx_bytes\":%lu,\"rx_packets\":%lu,\"tx_bytes\":%lu,\"tx_packets\":%lu}}\n",
			essidEsc, channelEsc, netModeEsc, bondingEsc, authEsc, secModeEsc, rfBandEsc, wpsModeEsc,
			wlanRxB, wlanRxP, wlanTxB, wlanTxP,
			ethRxB, ethRxP, ethTxB, ethTxP);
		body[sizeof(body) - 1] = '\0';
		sendJsonResponse(wp, 200, body);
		return 1;
	}

	if (strcmp(path, "/api/state.json") == 0) {
		api_dynbuf_t db;
		char netType[16], brMode[16], loopBr[16], macClone[32];
		char dhEn[16], dhSt[32], dhEnd[32], dhSub[32], dhLt[24], dhDom[64], dhD1[32], dhD2[32];
		char aclMode[16];
		char rfMaxC[16], rfBcn[16], rfDtim[16], rfRts[16], rfSp[16], rfSs[16], rfStbc[16], rfSgi[16], rfSka[24];
		char qosEn[16], qosDn[24], qosUp[24];
		char ddEn[16], ddPv[32], ddHn[128], ddUn[64], ddPw[128], ddTk[128], ddCu[256], ddIv[24], ddSt[256];
		char wdEn[16], wdTg[96], wdIv[24], wdMf[16], wdAc[24];
		char tzVal[64], nowStr[64], tsLog[256], ledNight[16];
		char rtMode[24], rtProto[24], rtWif[16], rtWip[32], rtWmk[32], rtWgw[32], rtWd1[32], rtWd2[32], rtWmtu[16];
		char fwSpi[16], fwPing[16], fwMgmt[16], fwSsh[16], fwMss[16], fwDmz[32];
		char lwIp[32], lwMk[32], lwGw[32], lwDn[96], liveDn1[32], liveDn2[32];
		char sshConfEn[16], telConfEn[16], upnpConfEn[16], sshjConfEn[16], sshjUser[64], sshjHost[64];
		char flashErr[160];
		char *aclRaw, *wdLog, *sshKeys, *rcLocal;
		unsigned long jffsTotalKb = 0, jffsUsedKb = 0;
		int jffsPct = 0;
		struct statfs sfs;
		int hasUdhcpd = 0, hasDropbear = 0, hasTelnetd = 0, hasUpnpd = 0;
		int hasDdns = 0, hasWatchdog = 0, hasSshjWd = 0, hasSshjCli = 0;
		int listenSsh = 0, listenTel = 0, estabRemSsh = 0;
		int ddRunning = 0, wdRunning = 0, hasVlan = 0, hasNat = 0, ctCnt = 0;
		int curSsh = 0, curTel = 0, curUpnp = 0, sshjWd = 0, sshjProc = 0, sshjRun = 0, sshjAct = 0;
		time_t nowT;
		struct tm tmBuf;
		size_t alen;

		aclRaw = (char *)calloc(1, 1024);
		wdLog = (char *)calloc(1, 2048);
		sshKeys = (char *)calloc(1, 4200);
		rcLocal = (char *)calloc(1, 4200);
		dynInit(&db, 16384);
		if (db.oom || !aclRaw || !wdLog || !sshKeys || !rcLocal) {
			free(aclRaw);
			free(wdLog);
			free(sshKeys);
			free(rcLocal);
			free(db.data);
			sendJsonResponse(wp, 500, "{\"status\":\"error\",\"message\":\"out_of_memory\"}\n");
			return 1;
		}

		scanLiveProcAndTcp(&hasUdhcpd, &hasDropbear, &hasTelnetd, &hasUpnpd,
			&hasDdns, &hasWatchdog, &hasSshjWd, &hasSshjCli,
			&listenSsh, &listenTel, &estabRemSsh);

		getParamOrDef("network_type", "2", netType, sizeof(netType));
		getParamOrDef("BridgeMode", "4", brMode, sizeof(brMode));
		getParamOrDef("LoopBreakingEnabled", "0", loopBr, sizeof(loopBr));
		getParamOrDef("MacCloningAddr", "0", macClone, sizeof(macClone));

		getParamOrDef("DHCPDEnabled", "1", dhEn, sizeof(dhEn));
		getParamOrDef("DHCPDStartAddress", "192.168.1.10", dhSt, sizeof(dhSt));
		getParamOrDef("DHCPDEndAddress", "192.168.1.20", dhEnd, sizeof(dhEnd));
		getParamOrDef("DHCPDSubnet", "255.255.255.0", dhSub, sizeof(dhSub));
		getParamOrDef("DHCPDLeaseTime", "864000", dhLt, sizeof(dhLt));
		getParamOrDef("DHCPDDomain", "Local", dhDom, sizeof(dhDom));
		getParamOrDef("DHCPDDNS1", "1.1.1.1", dhD1, sizeof(dhD1));
		getParamOrDef("DHCPDDNS2", "8.8.8.8", dhD2, sizeof(dhD2));

		if (!readProcFirstLine("/proc/sys/dev/mtlk/wlan0/AclMode", aclMode, sizeof(aclMode))) {
			getParamOrDef("AclMode", "0", aclMode, sizeof(aclMode));
		}
		if (!readFileTrimmed("/proc/sys/dev/mtlk/wlan0/ACL", aclRaw, 1024, 1)) {
			getParamOrDef("ACL", "", aclRaw, 1024);
		} else {
			for (alen = 0; aclRaw[alen] != '\0'; alen++) {
				if (aclRaw[alen] == '\r' || aclRaw[alen] == '\n') {
					aclRaw[alen] = ' ';
				}
			}
		}

		if (!readProcFirstLine("/proc/sys/dev/mtlk/wlan0/MaxConnections", rfMaxC, sizeof(rfMaxC))) {
			getParamOrDef("MaxConnections", "16", rfMaxC, sizeof(rfMaxC));
		}
		if (!readProcFirstLine("/proc/sys/dev/mtlk/wlan0/BeaconPeriod", rfBcn, sizeof(rfBcn))) {
			getParamOrDef("BeaconPeriod", "100", rfBcn, sizeof(rfBcn));
		}
		if (!readProcFirstLine("/proc/sys/dev/mtlk/wlan0/DTIMPeriod", rfDtim, sizeof(rfDtim))) {
			getParamOrDef("DTIMPeriod", "5", rfDtim, sizeof(rfDtim));
		}
		readLiveWlanRts(rfRts, sizeof(rfRts));
		if (!readProcFirstLine("/proc/sys/dev/mtlk/wlan0/ShortPreamble", rfSp, sizeof(rfSp))) {
			getParamOrDef("ShortPreamble", "1", rfSp, sizeof(rfSp));
		}
		if (!readProcFirstLine("/proc/sys/dev/mtlk/wlan0/ShortSlotTimeOptionEnabled", rfSs, sizeof(rfSs))) {
			getParamOrDef("ShortSlotTimeOptionEnabled", "1", rfSs, sizeof(rfSs));
		}
		if (!readProcFirstLine("/proc/sys/dev/mtlk/wlan0/STBC", rfStbc, sizeof(rfStbc))) {
			getParamOrDef("STBC", "0", rfStbc, sizeof(rfStbc));
		}
		if (!readProcFirstLine("/proc/sys/dev/mtlk/wlan0/ShortCyclicPrefix", rfSgi, sizeof(rfSgi))) {
			getParamOrDef("ShortCyclicPrefix", "0", rfSgi, sizeof(rfSgi));
		}
		if (!readProcFirstLine("/proc/sys/dev/mtlk/wlan0/StaKeepaliveTimeout", rfSka, sizeof(rfSka))) {
			getParamOrDef("StaKeepaliveTimeout", "60000", rfSka, sizeof(rfSka));
		}

		readKvFromFile("/mnt/jffs2/qos.conf", "ENABLED", "0", qosEn, sizeof(qosEn));
		readKvFromFile("/mnt/jffs2/qos.conf", "DOWN_KBPS", "0", qosDn, sizeof(qosDn));
		readKvFromFile("/mnt/jffs2/qos.conf", "UP_KBPS", "0", qosUp, sizeof(qosUp));

		readKvFromFile("/mnt/jffs2/ddns.conf", "ENABLED", "0", ddEn, sizeof(ddEn));
		readKvFromFile("/mnt/jffs2/ddns.conf", "PROVIDER", "duckdns", ddPv, sizeof(ddPv));
		readKvFromFile("/mnt/jffs2/ddns.conf", "HOSTNAME", "", ddHn, sizeof(ddHn));
		readKvFromFile("/mnt/jffs2/ddns.conf", "USERNAME", "", ddUn, sizeof(ddUn));
		readKvFromFile("/mnt/jffs2/ddns.conf", "PASSWORD", "", ddPw, sizeof(ddPw));
		readKvFromFile("/mnt/jffs2/ddns.conf", "TOKEN", "", ddTk, sizeof(ddTk));
		readKvFromFile("/mnt/jffs2/ddns.conf", "CUSTOM_URL", "", ddCu, sizeof(ddCu));
		readKvFromFile("/mnt/jffs2/ddns.conf", "INTERVAL", "600", ddIv, sizeof(ddIv));
		readFileTrimmed("/tmp/ddns.status", ddSt, sizeof(ddSt), 0);
		ddRunning = (isPidFileAlive("/tmp/ddns_updater.pid") || hasDdns) ? 1 : 0;

		readKvFromFile("/mnt/jffs2/watchdog.conf", "ENABLED", "0", wdEn, sizeof(wdEn));
		readKvFromFile("/mnt/jffs2/watchdog.conf", "TARGET", "1.1.1.1", wdTg, sizeof(wdTg));
		readKvFromFile("/mnt/jffs2/watchdog.conf", "INTERVAL", "60", wdIv, sizeof(wdIv));
		readKvFromFile("/mnt/jffs2/watchdog.conf", "MAX_FAIL", "5", wdMf, sizeof(wdMf));
		readKvFromFile("/mnt/jffs2/watchdog.conf", "ACTION", "recover", wdAc, sizeof(wdAc));
		readFileTrimmed("/tmp/watchdog.log", wdLog, 2048, 0);
		wdRunning = (isPidFileAlive("/tmp/net_watchdog.pid") || hasWatchdog) ? 1 : 0;

		if (!readFileTrimmed("/etc/TZ", tzVal, sizeof(tzVal), 1)) {
			readFileTrimmed("/mnt/jffs2/tz.conf", tzVal, sizeof(tzVal), 1);
		}
		if (tzVal[0] != '\0') {
			setenv("TZ", tzVal, 1);
			tzset();
		}
		nowStr[0] = '\0';
		nowT = time(NULL);
		if (localtime_r(&nowT, &tmBuf) != NULL) {
			strftime(nowStr, sizeof(nowStr), "%Y-%m-%d %H:%M:%S %Z", &tmBuf);
		}
		readFileTrimmed("/tmp/timesync.log", tsLog, sizeof(tsLog), 0);
		readKvFromFile("/mnt/jffs2/led.conf", "NIGHT_MODE", "0", ledNight, sizeof(ledNight));
		hasVlan = (access("/lib/modules/2.6.16-star/net/8021q.ko", F_OK) == 0) ? 1 : 0;
		if (!readFileTrimmed("/mnt/jffs2/dropbear/authorized_keys", sshKeys, 4200, 0)) {
			readFileTrimmed("/root/.ssh/authorized_keys", sshKeys, 4200, 0);
		}
		readFileTrimmed("/mnt/jffs2/rc.local", rcLocal, 4200, 0);

		readKvFromFile("/mnt/jffs2/router.conf", "ROUTER_MODE", "bridge", rtMode, sizeof(rtMode));
		readKvFromFile("/mnt/jffs2/router.conf", "WAN_PROTO", "dhcp", rtProto, sizeof(rtProto));
		readKvFromFile("/mnt/jffs2/router.conf", "WAN_IP", "", rtWip, sizeof(rtWip));
		readKvFromFile("/mnt/jffs2/router.conf", "WAN_MASK", "255.255.255.0", rtWmk, sizeof(rtWmk));
		readKvFromFile("/mnt/jffs2/router.conf", "WAN_GW", "", rtWgw, sizeof(rtWgw));
		readKvFromFile("/mnt/jffs2/router.conf", "WAN_DNS1", "1.1.1.1", rtWd1, sizeof(rtWd1));
		readKvFromFile("/mnt/jffs2/router.conf", "WAN_DNS2", "8.8.8.8", rtWd2, sizeof(rtWd2));
		readKvFromFile("/mnt/jffs2/router.conf", "WAN_MTU", "1500", rtWmtu, sizeof(rtWmtu));
		readKvFromFile("/mnt/jffs2/router.conf", "SPI", "1", fwSpi, sizeof(fwSpi));
		readKvFromFile("/mnt/jffs2/router.conf", "WAN_PING", "0", fwPing, sizeof(fwPing));
		readKvFromFile("/mnt/jffs2/router.conf", "WAN_MGMT", "0", fwMgmt, sizeof(fwMgmt));
		readKvFromFile("/mnt/jffs2/router.conf", "WAN_SSH", "0", fwSsh, sizeof(fwSsh));
		readKvFromFile("/mnt/jffs2/router.conf", "TCPMSS", "1", fwMss, sizeof(fwMss));
		readKvFromFile("/mnt/jffs2/router.conf", "DMZ_IP", "", fwDmz, sizeof(fwDmz));

		if (strcmp(rtMode, "wisp") == 0) {
			snprintf(rtWif, sizeof(rtWif), "wlan0");
		} else if (strcmp(rtMode, "router") == 0) {
			snprintf(rtWif, sizeof(rtWif), "eth0");
		} else {
			rtWif[0] = '\0';
		}

		readKvFromFile("/tmp/wan_status.conf", "WAN_IP", "", lwIp, sizeof(lwIp));
		readKvFromFile("/tmp/wan_status.conf", "WAN_MASK", "", lwMk, sizeof(lwMk));
		readKvFromFile("/tmp/wan_status.conf", "WAN_GW", "", lwGw, sizeof(lwGw));
		readKvFromFile("/tmp/wan_status.conf", "WAN_DNS", "", lwDn, sizeof(lwDn));
		if (rtWif[0] != '\0') {
			char ifIp[32], ifMk[32];
			getIfaceIpv4AndMask(rtWif, ifIp, sizeof(ifIp), ifMk, sizeof(ifMk));
			if (ifIp[0] != '\0') {
				snprintf(lwIp, sizeof(lwIp), "%s", ifIp);
			}
			if (ifMk[0] != '\0') {
				snprintf(lwMk, sizeof(lwMk), "%s", ifMk);
			}
		}
		{
			char ifGw[32];
			readLiveDefaultGw(ifGw, sizeof(ifGw));
			if (ifGw[0] != '\0') {
				snprintf(lwGw, sizeof(lwGw), "%s", ifGw);
			}
		}
		readLiveResolvDns(liveDn1, sizeof(liveDn1), liveDn2, sizeof(liveDn2));
		if (liveDn1[0] != '\0') {
			if (liveDn2[0] != '\0') {
				snprintf(lwDn, sizeof(lwDn), "%s, %s", liveDn1, liveDn2);
			} else {
				snprintf(lwDn, sizeof(lwDn), "%s", liveDn1);
			}
		}

		hasNat = (access("/sbin/mini-nat", X_OK) == 0 && access("/proc/net/ip_conntrack", R_OK) == 0) ? 1 : 0;
		ctCnt = (access("/proc/net/ip_conntrack", R_OK) == 0) ? countFileLines("/proc/net/ip_conntrack") : 0;

		if (statfs("/mnt/jffs2", &sfs) == 0 && sfs.f_blocks > 0) {
			unsigned long bsize = (unsigned long)sfs.f_bsize;
			jffsTotalKb = (unsigned long)(((unsigned long long)sfs.f_blocks * bsize) / 1024ULL);
			if (sfs.f_blocks >= sfs.f_bfree) {
				jffsUsedKb = (unsigned long)(((unsigned long long)(sfs.f_blocks - sfs.f_bfree) * bsize) / 1024ULL);
			}
			jffsPct = (int)((jffsTotalKb > 0) ? ((jffsUsedKb * 100UL) / jffsTotalKb) : 0);
		}
		readFileTrimmed("/tmp/config_save.err", flashErr, sizeof(flashErr), 1);

		readKvFromSysConf("SSH_Enabled", "1", sshConfEn, sizeof(sshConfEn));
		readKvFromSysConf("Telnet_Enabled", "1", telConfEn, sizeof(telConfEn));
		readKvFromSysConf("UPnP_Enabled", "0", upnpConfEn, sizeof(upnpConfEn));
		readKvFromSysConf("SSHJ_Enabled", "0", sshjConfEn, sizeof(sshjConfEn));
		readKvFromSysConf("SSHJ_User", "", sshjUser, sizeof(sshjUser));
		if (!isValidIdentStr(sshjUser, 1, 48)) {
			sshjUser[0] = '\0';
		}
		readKvFromSysConf("SSHJ_Host", "wap610n", sshjHost, sizeof(sshjHost));
		if (!isValidIdentStr(sshjHost, 1, 32)) {
			snprintf(sshjHost, sizeof(sshjHost), "wap610n");
		}
		curSsh = (listenSsh || hasDropbear) ? 1 : 0;
		curTel = (listenTel || hasTelnetd) ? 1 : 0;
		curUpnp = hasUpnpd ? 1 : 0;
		sshjWd = (isPidFileAlive("/tmp/sshj_tunnel.pid") || hasSshjWd) ? 1 : 0;
		sshjProc = (isPidFileAlive("/tmp/sshj_ssh.pid") || hasSshjCli) ? 1 : 0;
		sshjRun = (sshjProc && estabRemSsh) ? 1 : 0;
		sshjAct = (strcmp(sshjConfEn, "1") == 0 || sshjRun || sshjWd || sshjProc) ? 1 : 0;

		DYN_LIT(&db, "{\"status\":\"success\",\"opmode\":{\"network_type\":");
		dynAppendJsonStr(&db, netType);
		DYN_LIT(&db, ",\"bridge_mode\":");
		dynAppendJsonStr(&db, brMode);
		DYN_LIT(&db, ",\"loop_breaking\":");
		dynAppendJsonStr(&db, loopBr);
		DYN_LIT(&db, ",\"mac_cloning_addr\":");
		dynAppendJsonStr(&db, macClone);

		DYN_LIT(&db, "},\"dhcp\":{\"enabled\":");
		dynAppendJsonStr(&db, dhEn);
		DYN_LIT(&db, ",\"start\":");
		dynAppendJsonStr(&db, dhSt);
		DYN_LIT(&db, ",\"end\":");
		dynAppendJsonStr(&db, dhEnd);
		DYN_LIT(&db, ",\"subnet\":");
		dynAppendJsonStr(&db, dhSub);
		DYN_LIT(&db, ",\"lease_time\":");
		dynAppendJsonStr(&db, dhLt);
		DYN_LIT(&db, ",\"domain\":");
		dynAppendJsonStr(&db, dhDom);
		DYN_LIT(&db, ",\"dns1\":");
		dynAppendJsonStr(&db, dhD1);
		DYN_LIT(&db, ",\"dns2\":");
		dynAppendJsonStr(&db, dhD2);
		dynAppendFmt(&db, ",\"running\":%d,\"static_leases\":", hasUdhcpd);
		appendJsonLinesFile(&db, "/mnt/jffs2/dhcp_static.conf");
		DYN_LIT(&db, ",\"active_leases\":");
		appendActiveLeasesJson(&db);

		DYN_LIT(&db, "},\"acl\":{\"mode\":");
		dynAppendJsonStr(&db, aclMode);
		DYN_LIT(&db, ",\"raw\":");
		dynAppendJsonStr(&db, aclRaw);

		DYN_LIT(&db, "},\"rf\":{\"max_conn\":");
		dynAppendJsonStr(&db, rfMaxC);
		DYN_LIT(&db, ",\"beacon\":");
		dynAppendJsonStr(&db, rfBcn);
		DYN_LIT(&db, ",\"dtim\":");
		dynAppendJsonStr(&db, rfDtim);
		DYN_LIT(&db, ",\"rts\":");
		dynAppendJsonStr(&db, rfRts);
		DYN_LIT(&db, ",\"short_preamble\":");
		dynAppendJsonStr(&db, rfSp);
		DYN_LIT(&db, ",\"short_slot\":");
		dynAppendJsonStr(&db, rfSs);
		DYN_LIT(&db, ",\"stbc\":");
		dynAppendJsonStr(&db, rfStbc);
		DYN_LIT(&db, ",\"short_gi\":");
		dynAppendJsonStr(&db, rfSgi);
		DYN_LIT(&db, ",\"sta_ka_to\":");
		dynAppendJsonStr(&db, rfSka);

		DYN_LIT(&db, "},\"routes\":");
		appendJsonLinesFile(&db, "/mnt/jffs2/routes.conf");
		DYN_LIT(&db, ",\"vlans\":");
		appendJsonLinesFile(&db, "/mnt/jffs2/vlan.conf");

		DYN_LIT(&db, ",\"router\":{\"mode\":");
		dynAppendJsonStr(&db, rtMode);
		DYN_LIT(&db, ",\"wan_proto\":");
		dynAppendJsonStr(&db, rtProto);
		DYN_LIT(&db, ",\"wan_if\":");
		dynAppendJsonStr(&db, rtWif);
		DYN_LIT(&db, ",\"wan_ip\":");
		dynAppendJsonStr(&db, rtWip);
		DYN_LIT(&db, ",\"wan_mask\":");
		dynAppendJsonStr(&db, rtWmk);
		DYN_LIT(&db, ",\"wan_gw\":");
		dynAppendJsonStr(&db, rtWgw);
		DYN_LIT(&db, ",\"wan_dns1\":");
		dynAppendJsonStr(&db, rtWd1);
		DYN_LIT(&db, ",\"wan_dns2\":");
		dynAppendJsonStr(&db, rtWd2);
		DYN_LIT(&db, ",\"wan_mtu\":");
		dynAppendJsonStr(&db, rtWmtu);
		DYN_LIT(&db, ",\"spi\":");
		dynAppendJsonStr(&db, fwSpi);
		DYN_LIT(&db, ",\"wan_ping\":");
		dynAppendJsonStr(&db, fwPing);
		DYN_LIT(&db, ",\"wan_mgmt\":");
		dynAppendJsonStr(&db, fwMgmt);
		DYN_LIT(&db, ",\"wan_ssh\":");
		dynAppendJsonStr(&db, fwSsh);
		DYN_LIT(&db, ",\"tcpmss\":");
		dynAppendJsonStr(&db, fwMss);
		DYN_LIT(&db, ",\"dmz_ip\":");
		dynAppendJsonStr(&db, fwDmz);
		DYN_LIT(&db, ",\"live_wan_ip\":");
		dynAppendJsonStr(&db, lwIp);
		DYN_LIT(&db, ",\"live_wan_mask\":");
		dynAppendJsonStr(&db, lwMk);
		DYN_LIT(&db, ",\"live_wan_gw\":");
		dynAppendJsonStr(&db, lwGw);
		DYN_LIT(&db, ",\"live_wan_dns\":");
		dynAppendJsonStr(&db, lwDn);
		dynAppendFmt(&db, ",\"has_nat\":%d,\"conntrack_count\":%d,\"portfwd\":", hasNat, ctCnt);
		appendJsonLinesFile(&db, "/mnt/jffs2/portfwd.conf");

		DYN_LIT(&db, "},\"qos\":{\"enabled\":");
		dynAppendJsonStr(&db, qosEn);
		DYN_LIT(&db, ",\"down_kbps\":");
		dynAppendJsonStr(&db, qosDn);
		DYN_LIT(&db, ",\"up_kbps\":");
		dynAppendJsonStr(&db, qosUp);

		DYN_LIT(&db, "},\"ddns\":{\"enabled\":");
		dynAppendJsonStr(&db, ddEn);
		DYN_LIT(&db, ",\"provider\":");
		dynAppendJsonStr(&db, ddPv);
		DYN_LIT(&db, ",\"hostname\":");
		dynAppendJsonStr(&db, ddHn);
		DYN_LIT(&db, ",\"username\":");
		dynAppendJsonStr(&db, ddUn);
		dynAppendFmt(&db, ",\"has_pass\":%d,\"has_token\":%d,\"custom_url\":",
			(ddPw[0] != '\0') ? 1 : 0, (ddTk[0] != '\0') ? 1 : 0);
		dynAppendJsonStr(&db, ddCu);
		DYN_LIT(&db, ",\"interval\":");
		dynAppendJsonStr(&db, ddIv);
		dynAppendFmt(&db, ",\"running\":%d,\"status\":", ddRunning);
		dynAppendJsonStr(&db, ddSt);

		DYN_LIT(&db, "},\"watchdog\":{\"enabled\":");
		dynAppendJsonStr(&db, wdEn);
		DYN_LIT(&db, ",\"target\":");
		dynAppendJsonStr(&db, wdTg);
		DYN_LIT(&db, ",\"interval\":");
		dynAppendJsonStr(&db, wdIv);
		DYN_LIT(&db, ",\"max_fail\":");
		dynAppendJsonStr(&db, wdMf);
		DYN_LIT(&db, ",\"action\":");
		dynAppendJsonStr(&db, wdAc);
		dynAppendFmt(&db, ",\"running\":%d,\"log\":", wdRunning);
		dynAppendJsonStr(&db, wdLog);

		DYN_LIT(&db, "},\"system\":{\"tz\":");
		dynAppendJsonStr(&db, tzVal);
		DYN_LIT(&db, ",\"now\":");
		dynAppendJsonStr(&db, nowStr);
		DYN_LIT(&db, ",\"timesync_log\":");
		dynAppendJsonStr(&db, tsLog);
		DYN_LIT(&db, ",\"led_night\":");
		dynAppendJsonStr(&db, ledNight);
		dynAppendFmt(&db, ",\"has_vlan_mod\":%d,\"jffs_used_kb\":%lu,\"jffs_total_kb\":%lu,\"jffs_pct\":%d,\"flash_save_err\":",
			hasVlan, jffsUsedKb, jffsTotalKb, jffsPct);
		dynAppendJsonStr(&db, flashErr);
		DYN_LIT(&db, ",\"ssh_keys\":");
		dynAppendJsonStr(&db, sshKeys);
		DYN_LIT(&db, ",\"rc_local\":");
		dynAppendJsonStr(&db, rcLocal);

		dynAppendFmt(&db, "},\"remote\":{\"ssh\":%d,\"telnet\":%d,\"upnp\":%d,"
			"\"sshj\":%d,\"sshj_running\":%d,\"sshj_watchdog\":%d,\"sshj_user\":",
			curSsh, curTel, curUpnp, sshjAct, sshjRun, sshjWd);
		dynAppendJsonStr(&db, sshjUser);
		DYN_LIT(&db, ",\"sshj_host\":");
		dynAppendJsonStr(&db, sshjHost);
		DYN_LIT(&db, "}}\n");

		free(aclRaw);
		free(wdLog);
		free(sshKeys);
		free(rcLocal);
		if (db.oom || db.data == NULL) {
			free(db.data);
			sendJsonResponse(wp, 500, "{\"status\":\"error\",\"message\":\"out_of_memory\"}\n");
			return 1;
		}
		sendJsonResponse(wp, 200, db.data);
		free(db.data);
		return 1;
	}

	sendJsonResponse(wp, 404, "{\"error\":\"not_found\"}\n");
	return 1;
}

/******************************************************************************/
/*
 *	Default error handler.  The developer should insert code to handle
 *	error messages in the desired manner.
 */

void defaultErrorHandler(int etype, char_t *msg)
{
#if 0
	write(1, msg, gstrlen(msg));
#endif
}

/******************************************************************************/
/*
 *	Trace log. Customize this function to log trace output
 */

void defaultTraceHandler(int level, char_t *buf)
{
/*
 *	The following code would write all trace regardless of level
 *	to stdout.
 */
#if 0
	if (buf) {
		write(1, buf, gstrlen(buf));
	}
#endif
}

/******************************************************************************/
/*
 *	Returns a pointer to an allocated qualified unique temporary file name.
 *	This filename must eventually be deleted with bfree();
 */

char_t *websGetCgiCommName()
{
	char_t	*pname1, *pname2;
	char_t	fallbackBuf[128];
	static int cgiCounter = 0;

	pname1 = tempnam(NULL, T("cgi"));
	if (pname1 == NULL) {
		snprintf(fallbackBuf, sizeof(fallbackBuf), "/tmp/cgi_%d_%d", (int)getpid(), ++cgiCounter);
		fallbackBuf[sizeof(fallbackBuf) - 1] = '\0';
		pname2 = bstrdup(B_L, fallbackBuf);
	} else {
		pname2 = bstrdup(B_L, pname1);
		free(pname1);
	}
	return pname2;
}

/******************************************************************************/
/*
 *	Launch the CGI process and return a handle to it.
 */

int websLaunchCgiProc(char_t *cgiPath, char_t **argp, char_t **envp,
					  char_t *stdIn, char_t *stdOut)
{
	int	pid, fdin, fdout, rc, fd;
	const char errMsg[] = "content-type: text/html\n\nExecution of cgi process failed\n";

	debug_printf("websLaunchCgiProc: Into websLaunchCgiProc pid=%d\n", getpid());

	fdin = fdout = rc = -1;
	if ((fdin = open(stdIn, O_RDONLY | O_CREAT, 0600)) < 0 ||
		(fdout = open(stdOut, O_WRONLY | O_CREAT | O_TRUNC, 0600)) < 0) {
		if (fdin >= 0) {
			close(fdin);
		}
		if (fdout >= 0) {
			close(fdout);
		}
		return -1;
	}

	rc = pid = fork();
	if (pid == 0) {
		/* Child process: restore default signals, redirect stdio, close inherited FDs */
		signal(SIGPIPE, SIG_DFL);
		signal(SIGCHLD, SIG_DFL);
		if (dup2(fdin, 0) == -1 || dup2(fdout, 1) == -1) {
			_exit(127);
		}
		for (fd = 3; fd < 256; fd++) {
			close(fd);
		}
		gchdir(MT_WebRootDir);

		execve(cgiPath, argp, envp);
		write(1, errMsg, sizeof(errMsg) - 1);
		_exit(127);
	}

	close(fdout);
	close(fdin);
	return rc;
}

/******************************************************************************/
/*
 *	Check the CGI process.  Return 0 if it does not exist; non 0 if it does.
 */

int websCheckCgiProc(int handle)
{
	int rc;
/*
 *	Check to see if the CGI child process has terminated or not yet.
 *	When SIGCHLD is SIG_IGN, waitpid returns -1 with errno == ECHILD when the child exits.
 */
	rc = waitpid(handle, NULL, WNOHANG);
	if (rc == -1 && errno == EINTR) {
		return 1;
	}
	if (rc == handle || rc == -1) {
		return 0;
	} else {
		return 1;
	}
}

/******************************************************************************/

#ifdef B_STATS
static void memLeaks() 
{
	int		fd;

	if ((fd = gopen(T("leak.txt"), O_CREAT | O_TRUNC | O_WRONLY, 0666)) >= 0) {
		bstats(fd, printMemStats);
		close(fd);
	}
}

/******************************************************************************/
/*
 *	Print memory usage / leaks
 */

static void printMemStats(int handle, char_t *fmt, ...)
{
	va_list		args;
	char_t		buf[256];

	va_start(args, fmt);
	vsprintf(buf, fmt, args);
	va_end(args);
	write(handle, buf, strlen(buf));
}
#endif

/******************************************************************************/
