/*
 * wps.c - Open-source C89 WPS form handlers for WAP610N GoAhead WebServer
 * Replaces legacy proprietary wps.o blob (USE_BLOB_WPS=0).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "uemf.h"
#include "webs.h"
#include "wsIntrn.h"
#include "mt_api.h"
#include "utils.h"
#include "wps.h"

extern int hnap_active_wps_monitor(void);

static int wpsWriteFlagFile(const char *path, const char *content)
{
	FILE *fp;

	fp = fopen(path, "w");
	if (fp == NULL) {
		return -1;
	}
	fputs(content, fp);
	fclose(fp);
	return 0;
}

static void wpsReadFirstLineTrim(const char *path, char *buf, size_t bufLen)
{
	FILE *fp;
	size_t len;

	if (buf == NULL || bufLen == 0) {
		return;
	}
	buf[0] = '\0';
	fp = fopen(path, "r");
	if (fp == NULL) {
		return;
	}
	if (fgets(buf, (int)bufLen, fp) != NULL) {
		len = strlen(buf);
		while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r' ||
		       buf[len - 1] == ' ' || buf[len - 1] == '\t')) {
			buf[--len] = '\0';
		}
	}
	fclose(fp);
}

static long wpsReadUptimeSeconds(void)
{
	FILE *fp;
	long up = 0;

	fp = fopen("/proc/uptime", "r");
	if (fp != NULL) {
		if (fscanf(fp, "%ld", &up) != 1) {
			up = 0;
		}
		fclose(fp);
	}
	return up;
}

static void wpsSendPlainText(webs_t wp, const char *body)
{
	wp->flags &= ~WEBS_KEEP_ALIVE;
	websWrite(wp,
		T("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n"
		  "Connection: close\r\nPragma: no-cache\r\n"
		  "Cache-Control: no-cache\r\n\r\n%s"),
		body != NULL ? body : "");
	websDone(wp, 200);
}

static void formGuiWaitEthernetUP(webs_t wp, char_t *path, char_t *query)
{
	char ipConfigMethod[16];
	FILE *fp;

	(void)path;
	(void)query;

	memset(ipConfigMethod, 0, sizeof(ipConfigMethod));
	MT_Get_Param(T("ip_config_method"), ipConfigMethod, sizeof(ipConfigMethod) - 1);
	debug_printf("guiWaitEthernetUP: Into guiWaitEthernetUP.\n");

	fp = fopen("/var/success_get_ip", "r");
	if (fp == NULL) {
		debug_printf("guiWaitEthernetUP: not found /var/success_get_ip.\n");
		if (atoi(ipConfigMethod) == 1) {
			doSystem("/root/mtlk/web/gui_wps_init.sh stop &");
		}
		wpsSendPlainText(wp, "0");
	} else {
		fclose(fp);
		debug_printf("guiWaitEthernetUP: Find /var/success_get_ip.\n");
		wpsWriteFlagFile("/var/assign_get_ip", "1\n");
		doSystem("/root/mtlk/web/del_wps_temp.sh &");
		wpsSendPlainText(wp, "1");
	}
	debug_printf("guiWaitEthernetUP: Exit guiWaitEthernetUP.\n");
}

static void formCleanWPSFailTemp(webs_t wp, char_t *path, char_t *query)
{
	(void)path;
	(void)query;

	unlink("/var/wpsLoopDetect");
	doSystem("/root/mtlk/web/gui_wps_init.sh stop");
	unlink("/var/gui_wps_waiting");
	wpsSendPlainText(wp, "0");
}

static void formCleanWPSTemp(webs_t wp, char_t *path, char_t *query)
{
	(void)path;
	(void)query;

	unlink("/var/wpsLoopDetect");
	wpsSendPlainText(wp, "0");
}

static void formGetCurrentWPSStatus(webs_t wp, char_t *path, char_t *query)
{
	FILE *fp;
	char line[128];
	char wpsType[32];
	char startStr[32];
	char ipBuf[64];
	char ssidBuf[128];
	char outBuf[300];
	int ts = 0;
	int curStat = -1;
	int statTmp = -1;
	int percent = 0;
	long startTs = 0;
	long nowTs = 0;
	long elapsed = 0;
	struct stat st;

	(void)path;
	(void)query;

	fp = fopen("/var/WPS_History", "r");
	if (fp != NULL) {
		while (fgets(line, sizeof(line), fp) != NULL) {
			if (sscanf(line, "%d,%d", &ts, &statTmp) == 2) {
				curStat = statTmp;
			}
		}
		fclose(fp);
	}

	wpsReadFirstLineTrim("/var/wps_type", wpsType, sizeof(wpsType));
	if (wpsType[0] == '\0') {
		strncpy(wpsType, "-1", sizeof(wpsType) - 1);
		wpsType[sizeof(wpsType) - 1] = '\0';
	}

	wpsReadFirstLineTrim("/var/wps_start_time", startStr, sizeof(startStr));
	if (startStr[0] != '\0') {
		startTs = atol(startStr);
		nowTs = wpsReadUptimeSeconds();
		if (nowTs >= startTs && startTs > 0) {
			elapsed = nowTs - startTs;
			percent = (int)((elapsed * 100L) / 120L);
			if (percent < 0) {
				percent = 0;
			} else if (percent > 100) {
				percent = 100;
			}
		}
	}
	if (curStat == 99) {
		percent = 100;
	}

	ipBuf[0] = '\0';
	if (stat("/var/dhcp_timeout", &st) == 0) {
		strncpy(ipBuf, "dhcp_timeout", sizeof(ipBuf) - 1);
		ipBuf[sizeof(ipBuf) - 1] = '\0';
	} else {
		wpsReadFirstLineTrim("/var/gui_get_ip_temp", ipBuf, sizeof(ipBuf));
		if (ipBuf[0] == '\0') {
			char dhcpLine[128];
			char *comma;

			wpsReadFirstLineTrim("/var/Get_IPInfo_From_DHCP", dhcpLine, sizeof(dhcpLine));
			if (dhcpLine[0] != '\0') {
				comma = strchr(dhcpLine, ',');
				if (comma != NULL) {
					*comma = '\0';
				}
				strncpy(ipBuf, dhcpLine, sizeof(ipBuf) - 1);
				ipBuf[sizeof(ipBuf) - 1] = '\0';
			}
		}
	}
	if (ipBuf[0] == '\0') {
		strncpy(ipBuf, "NULL", sizeof(ipBuf) - 1);
		ipBuf[sizeof(ipBuf) - 1] = '\0';
	}

	wpsReadFirstLineTrim("/var/wps_success_ssid", ssidBuf, sizeof(ssidBuf));

	snprintf(outBuf, sizeof(outBuf), "%d,%s,%d,%s,%s\n",
		curStat, wpsType, percent, ipBuf, ssidBuf);
	outBuf[sizeof(outBuf) - 1] = '\0';
	wpsSendPlainText(wp, outBuf);
}

static void formStartWPS(webs_t wp, char_t *path, char_t *query)
{
	char_t *networkType;
	char_t *nowMode;

	(void)path;
	(void)query;

	networkType = websGetVar(wp, T("networkType"), T(""));
	nowMode = websGetVar(wp, T("nowMode"), T(""));

	wpsArrayIndex = 0;
	wpsErrorCount = 0;
	memset(wpsStatusArray, 0, sizeof(wpsStatusArray));

	wpsWriteFlagFile("/var/wpsLoopDetect", "1\n");
	doSystem("/root/mtlk/web/gui_wps_init.sh start");
	wpsWriteFlagFile("/var/wpsRunning", "1\n");
	doSystem("/root/mtlk/etc/mtpriv wlan0 Wildcard_ESSID ''");

	if (nowMode != NULL && strcmp(nowMode, "getPin") == 0) {
		hnap_active_wps_monitor();
		wpsWriteFlagFile("/var/wps_type", "pin\n");
		if (networkType != NULL && strcmp(networkType, "STA") == 0) {
			doSystem("/root/mtlk/etc/mtlk_wps_cmd.tcl get_conf_via_pin&");
		}
	} else if (nowMode != NULL && strcmp(nowMode, "getPBC") == 0) {
		hnap_active_wps_monitor();
		wpsWriteFlagFile("/var/wps_type", "pbc\n");
		doSystem("/root/mtlk/etc/mtlk_wps_cmd.tcl get_conf_via_pbc&");
	}

	websRedirect(wp, T("/station/wps_status.asp"));
}

void wpsPage(webs_t wp, char_t *path, char_t *query)
{
	char_t *bssidUsage;
	char_t *argv[1];

	(void)path;
	(void)query;

	bssidUsage = websGetVar(wp, T("WPS_BSSID_USAGE"), T(""));
	MT_Set_Param(T("WPS_BSSID_USAGE"), bssidUsage, 1);
	argv[0] = T("WPS_BSSID_USAGE");
	MT_WriteConfFiles(1, argv);
	submitToFlash();
	websRedirect(wp, T("/station/wireless_basic.asp"));
}

void formDefineWPS(void)
{
	websFormDefine(T("getCurrentWPSStatus"), formGetCurrentWPSStatus);
	websFormDefine(T("cleanWPSFailTemp"), formCleanWPSFailTemp);
	websFormDefine(T("cleanWPSTemp"), formCleanWPSTemp);
	websFormDefine(T("guiWaitEthernetUP"), formGuiWaitEthernetUP);
	websFormDefine(T("startWPS"), formStartWPS);
	websFormDefine(T("wpsTest"), wpsPage);
}
