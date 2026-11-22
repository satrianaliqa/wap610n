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
#include	<signal.h>
#include	<unistd.h> 
#include	<sys/types.h>
#include	<sys/wait.h>
#include	<fcntl.h>
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
extern void defaultErrorHandler(int etype, char_t *msg);
extern void defaultTraceHandler(int level, char_t *buf);
extern void upldForm(webs_t wp, char_t *path, char_t *query);
extern void formDumpFirmware(webs_t wp, char_t *path, char_t *query);

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
				fclose(fp);
				return 1;
			}
		}
	}
	fclose(fp);
	return 0;
}

static void preserveCustomSysConfKeys(void)
{
	static const char * const keys[3] = {
		"SSH_Enabled",
		"Telnet_Enabled",
		"WirelessMgmtEnabled"
	};
	char vals[3][16];
	int i;
	FILE *fp;

	for (i = 0; i < 3; i++) {
		if (!readConfValueExact("/tmp/sys.conf", keys[i], vals[i], sizeof(vals[i]))) {
			snprintf(vals[i], sizeof(vals[i]), "1");
		}
	}
	fp = fopen("/mnt/jffs2/sys.conf", "a");
	if (fp != NULL) {
		for (i = 0; i < 3; i++) {
			char existing[16];
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
	FILE *fp;
	char buf[512];
	char chStr[16];
	int nread;
	char *p;
	size_t i;

	(void)eid;
	(void)argc;
	(void)argv;
	chStr[0] = '\0';
	fp = popen("/sbin/iwconfig wlan0 2>/dev/null", "r");
	if (fp != NULL) {
		nread = (int)fread(buf, 1, sizeof(buf) - 1, fp);
		pclose(fp);
		if (nread > 0) {
			buf[nread] = '\0';
			p = strstr(buf, "Channel=");
			if (p != NULL) {
				p += 8;
				i = 0;
				while (*p >= '0' && *p <= '9' && i < sizeof(chStr) - 1) {
					chStr[i++] = *p++;
				}
				chStr[i] = '\0';
			}
		}
	}
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
	char cloneMAC[32];
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
	if (waitOtherProcess != 0) {
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
	safeCopyFormVar(wp, T("cloneMAC"), cloneMAC, sizeof(cloneMAC));

	if (ipConfigMethod[0] == '\0' || hostName[0] == '\0' || lanIP[0] == '\0') {
		websRedirect(wp, T("/network/sta_network.asp"));
		return;
	}

	saveOldWebPid(getpid(), "sta_network.asp");
	waitOtherProcess = 1;
	pid = fork();
	if (pid < 0) {
		waitOtherProcess = 0;
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
	MT_Set_Param("ip_lan", lanIP, 1);
	paramNames[paramCount++] = "ip_lan";
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

	MT_WriteConfFiles(paramCount, paramNames);
	preserveCustomSysConfKeys();
	submitToFlash();

	{
		FILE *hf = fopen("/etc/hostname", "w");
		if (hf != NULL) {
			fprintf(hf, "%s\n", hostName);
			fclose(hf);
		}
	}

	if (strcmp(ipConfigMethod, "0") == 0) {
		doSystem("killall udhcpc >/dev/null 2>&1; /etc/udhcpc/dhcp.tcl startup &");
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
	if (waitOtherProcess != 0) {
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

	isApMode = (atoi(netType) == 2 || strcmp(nowMode, "apMode") == 0) ? 1 : 0;

	saveOldWebPid(getpid(), "wireless_basic.asp");
	waitOtherProcess = 1;
	pid = fork();
	if (pid < 0) {
		waitOtherProcess = 0;
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

	MT_WriteConfFiles(paramCount, paramNames);
	submitToFlash();

	if (isApMode) {
		if (country[0] != '\0') {
			doSystem("/root/mtlk/etc/mtpriv wlan0 Country %s >/dev/null 2>&1", country);
		}
		if (freqBand[0] != '\0') {
			doSystem("/root/mtlk/etc/mtpriv wlan0 FrequencyBand %s >/dev/null 2>&1", freqBand);
		}
		if (channelBonding[0] != '\0') {
			doSystem("/root/mtlk/etc/mtpriv wlan0 ChannelBonding %s >/dev/null 2>&1", channelBonding);
		}
		if (upperLower[0] != '\0') {
			doSystem("/root/mtlk/etc/mtpriv wlan0 UpperLowerChannelBonding %s >/dev/null 2>&1", upperLower);
		}
		if (channel[0] != '\0') {
			if (strcmp(channel, "0") == 0) {
				doSystem("/sbin/iwconfig wlan0 channel auto >/dev/null 2>&1");
			} else {
				doSystem("/sbin/iwconfig wlan0 channel %s >/dev/null 2>&1", channel);
			}
		}
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
int websMainPid = 0;

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
			websInitRealm();
			lang_free();
			lang_init();
			waitOtherProcess = 0;
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
	websSetDefaultPage(T("/network/ap_network.asp"));
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

static int websHomePageHandler(webs_t wp, char_t *urlPrefix, char_t *webDir,
	int arg, char_t *url, char_t *path, char_t *query)
{
/*
 *	If the empty or "/" URL is invoked, redirect default URLs to the home page
 */
	if (*url == '\0' || gstrcmp(url, T("/")) == 0) {
		char network_type[8];
		memset(network_type, '\0', sizeof(network_type));
		MT_Get_Param("network_type", network_type, sizeof(network_type) - 1);
		network_type[sizeof(network_type) - 1] = '\0';
		
		if (strcmp(network_type, "0") == 0)
			websRedirect(wp, T("/network/sta_network.asp"));
		else
			websRedirect(wp, T("/network/ap_network.asp"));
		
		return 1;
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
		} else if (c >= 0x20 && c < 0x7f) {
			dst[i++] = (char)c;
		}
	}
	dst[i] = '\0';
}

static void sendJsonResponse(webs_t wp, int status, const char *body)
{
	size_t bodyLen = strlen(body);
	wp->flags &= ~WEBS_KEEP_ALIVE;
	websWrite(wp, T("HTTP/1.0 %d %s\r\n"), status, (status == 200) ? "OK" : "Not Found");
	websWrite(wp, T("Server: %s\r\n"), WEBS_NAME);
	websWrite(wp, T("Connection: close\r\n"));
	websWrite(wp, T("Cache-Control: no-store\r\n"));
	websWrite(wp, T("Content-Type: application/json\r\n"));
	websWrite(wp, T("Content-Length: %lu\r\n\r\n"), (unsigned long)bodyLen);
	websWriteBlock(wp, (char_t *)body, (int)bodyLen);
	websDone(wp, status);
}

static int websApiHandler(webs_t wp, char_t *urlPrefix, char_t *webDir,
	int arg, char_t *url, char_t *path, char_t *query)
{
	char body[1024];
	FILE *fp;
	char line[256];

	if (path == NULL) {
		return 0;
	}

	if (strcmp(path, "/api/status.json") == 0) {
		unsigned long uptimeSec = 0;
		unsigned long memTotal = 0, memFree = 0, buffers = 0, cached = 0;
		unsigned long commitLimit = 0, committedAs = 0;
		int tcpEstab = 0, tcpListen = 0, tcpCloseWait = 0;
		char hostRaw[64], hostEsc[128], netType[16], netTypeEsc[32], verRaw[96], verEsc[192];

		memset(hostRaw, 0, sizeof(hostRaw));
		memset(netType, 0, sizeof(netType));
		memset(verRaw, 0, sizeof(verRaw));
		MT_Get_Param("HostName", hostRaw, sizeof(hostRaw) - 1);
		MT_Get_Param("network_type", netType, sizeof(netType) - 1);
		jsonEscapeCopy(hostEsc, sizeof(hostEsc), hostRaw);
		jsonEscapeCopy(netTypeEsc, sizeof(netTypeEsc), netType);

		fp = fopen("/proc/uptime", "r");
		if (fp != NULL) {
			if (fgets(line, sizeof(line), fp) != NULL) {
				sscanf(line, "%lu", &uptimeSec);
			}
			fclose(fp);
		}

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
			"\"version\":\"%s\",\"uptime_sec\":%lu,"
			"\"mem_kb\":{\"total\":%lu,\"free\":%lu,\"buffers\":%lu,\"cached\":%lu,"
			"\"commit_limit\":%lu,\"committed_as\":%lu},"
			"\"tcp\":{\"established\":%d,\"listen\":%d,\"close_wait\":%d}}\n",
			hostEsc, netTypeEsc, verEsc, uptimeSec,
			memTotal, memFree, buffers, cached, commitLimit, committedAs,
			tcpEstab, tcpListen, tcpCloseWait);
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
