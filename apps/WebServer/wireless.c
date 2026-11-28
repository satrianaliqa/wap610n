/*
 * wireless.c - Open-source C89 wireless form and ASP handlers for WAP610N GoAhead WebServer
 * Replaces legacy proprietary wireless.o blob (USE_BLOB_WIRELESS=0).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>

#include "uemf.h"
#include "webs.h"
#include "wsIntrn.h"
#include "mt_api.h"
#include "utils.h"
#include "management.h"
#include "hnap_config_metalink.h"
#include "wireless.h"

extern int isWaitOtherProcessBusy(void);
extern void setWaitOtherProcess(int busy);
extern int MT_get_station_layer2_link_status(void);
extern int MT_get_station_link_status(void);

#define WL_MAX_SCAN_ENTRIES 64

typedef struct {
	char bssid[32];
	char essid[128];
	char mode[16];
	int channel;
	int signalDbm;
	int hasSignal;
	char band[16];
	char encryType[32];
	int isWps;
} wlScanEntry_t;

static int wlIsDigitsInRange(const char *s, long minVal, long maxVal)
{
	char *endp;
	long val;

	if (s == NULL || s[0] == '\0') {
		return 0;
	}
	val = strtol(s, &endp, 10);
	if (*endp != '\0') {
		return 0;
	}
	return (val >= minVal && val <= maxVal) ? 1 : 0;
}

static int wlIsValidMac(const char *s)
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
			if (!isxdigit((unsigned char)s[i])) {
				return 0;
			}
		}
	}
	return 1;
}

static void wlShellSingleQuote(const char *in, char *out, size_t outSz)
{
	size_t i = 0;
	size_t j = 0;

	if (out == NULL || outSz < 3) {
		return;
	}
	out[j++] = '\'';
	if (in != NULL) {
		while (in[i] != '\0' && j + 5 < outSz) {
			if (in[i] == '\'') {
				out[j++] = '\'';
				out[j++] = '\\';
				out[j++] = '\'';
				out[j++] = '\'';
			} else if (in[i] != '\r' && in[i] != '\n') {
				out[j++] = in[i];
			}
			i++;
		}
	}
	out[j++] = '\'';
	out[j] = '\0';
}

static void wlFormatWepKey(const char *raw, char *out, size_t outSz)
{
	size_t len;
	size_t i;
	int allHex = 1;

	if (out == NULL || outSz == 0) {
		return;
	}
	out[0] = '\0';
	if (raw == NULL || raw[0] == '\0') {
		return;
	}
	if ((raw[0] == '0') && (raw[1] == 'x' || raw[1] == 'X')) {
		snprintf(out, outSz, "%s", raw);
		out[outSz - 1] = '\0';
		return;
	}
	len = strlen(raw);
	for (i = 0; i < len; i++) {
		if (!isxdigit((unsigned char)raw[i])) {
			allHex = 0;
			break;
		}
	}
	if (allHex && (len == 10 || len == 26)) {
		snprintf(out, outSz, "0x%s", raw);
	} else {
		snprintf(out, outSz, "%s", raw);
	}
	out[outSz - 1] = '\0';
}

static void wlCloseInheritedFds(void)
{
	int fd;

	for (fd = 3; fd < 256; fd++) {
		close(fd);
	}
}

static char *wlReadFileAlloc(const char *path, size_t *outLen)
{
	struct stat st;
	FILE *fp;
	char *buf;
	size_t sz;
	size_t nread;

	if (outLen != NULL) {
		*outLen = 0;
	}
	if (stat(path, &st) != 0 || st.st_size <= 0 || st.st_size > (off_t)(256 * 1024)) {
		return NULL;
	}
	sz = (size_t)st.st_size;
	buf = (char *)malloc(sz + 1);
	if (buf == NULL) {
		return NULL;
	}
	memset(buf, 0, sz + 1);
	fp = fopen(path, "r");
	if (fp == NULL) {
		free(buf);
		return NULL;
	}
	nread = fread(buf, 1, sz, fp);
	fclose(fp);
	buf[nread] = '\0';
	if (outLen != NULL) {
		*outLen = nread;
	}
	return buf;
}

static void wlCopyRange(const char *src, size_t maxSrc, char *dst, size_t dstSz, int trimTrailing)
{
	size_t n = 0;

	if (dst == NULL || dstSz == 0) {
		return;
	}
	dst[0] = '\0';
	if (src == NULL) {
		return;
	}
	while (n < maxSrc && n + 1 < dstSz && src[n] != '\0' &&
	       src[n] != '\r' && src[n] != '\n') {
		dst[n] = src[n];
		n++;
	}
	dst[n] = '\0';
	if (trimTrailing) {
		while (n > 0 && (dst[n - 1] == ' ' || dst[n - 1] == '\t')) {
			dst[--n] = '\0';
		}
	}
}

static void wlCopyTrim(const char *src, size_t maxSrc, char *dst, size_t dstSz)
{
	wlCopyRange(src, maxSrc, dst, dstSz, 1);
}

static void wlHtmlEscape(const char *in, char *out, size_t outSz)
{
	size_t i = 0;
	size_t j = 0;

	if (out == NULL || outSz == 0) {
		return;
	}
	out[0] = '\0';
	if (in == NULL) {
		return;
	}
	while (in[i] != '\0' && j + 7 < outSz) {
		if (in[i] == '&') {
			memcpy(out + j, "&amp;", 5);
			j += 5;
		} else if (in[i] == '<') {
			memcpy(out + j, "&lt;", 4);
			j += 4;
		} else if (in[i] == '>') {
			memcpy(out + j, "&gt;", 4);
			j += 4;
		} else if (in[i] == '"') {
			memcpy(out + j, "&quot;", 6);
			j += 6;
		} else if (in[i] == '\'') {
			memcpy(out + j, "&#39;", 5);
			j += 5;
		} else {
			out[j++] = in[i];
		}
		i++;
	}
	out[j] = '\0';
}

static void wlWriteTrString(webs_t wp, const char *input)
{
	const char *p = input;
	const char *trStart;
	const char *idPos;
	const char *idEnd;
	const char *tagEnd;
	const char *endTr;
	char key[128];
	char *translated;
	size_t keyLen;

	while (p != NULL && *p != '\0') {
		trStart = strstr(p, "<!--#tr id=\"");
		if (trStart == NULL) {
			websWrite(wp, T("%s"), (char_t *)p);
			break;
		}
		if (trStart > p) {
			char *prefix = (char *)malloc((size_t)(trStart - p) + 1);
			if (prefix != NULL) {
				memcpy(prefix, p, (size_t)(trStart - p));
				prefix[trStart - p] = '\0';
				websWrite(wp, T("%s"), prefix);
				free(prefix);
			}
		}
		idPos = trStart + 12;
		idEnd = strchr(idPos, '"');
		tagEnd = (idEnd != NULL) ? strstr(idEnd, "-->") : NULL;
		endTr = (tagEnd != NULL) ? strstr(tagEnd + 3, "<!--#endtr-->") : NULL;
		if (idEnd == NULL || tagEnd == NULL || endTr == NULL) {
			websWrite(wp, T("%s"), (char_t *)trStart);
			break;
		}
		keyLen = (size_t)(idEnd - idPos);
		if (keyLen >= sizeof(key)) {
			keyLen = sizeof(key) - 1;
		}
		memcpy(key, idPos, keyLen);
		key[keyLen] = '\0';
		translated = NULL;
		if (lang_get(key, &translated) == 1 && translated != NULL && translated[0] != '\0') {
			websWrite(wp, T("%s"), translated);
		} else {
			size_t fbLen = (size_t)(endTr - (tagEnd + 3));
			char *fb = (char *)malloc(fbLen + 1);
			if (fb != NULL) {
				memcpy(fb, tagEnd + 3, fbLen);
				fb[fbLen] = '\0';
				websWrite(wp, T("%s"), fb);
				free(fb);
			}
		}
		p = endTr + 13;
	}
}

static void wlSendPlainHeaders(webs_t wp)
{
	wp->flags &= ~WEBS_KEEP_ALIVE;
	websWrite(wp,
		T("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n"
		  "Connection: close\r\nPragma: no-cache\r\n"
		  "Cache-Control: no-cache\r\n\r\n"));
}

static void wlCommitParams(int paramCount, char_t **paramNames)
{
	doSystem("if [ -x /bin/config_lock.sh ]; then /bin/config_lock.sh %d >/dev/null 2>&1; fi",
		(int)getpid());
	if (paramCount > 0 && paramNames != NULL) {
		MT_WriteConfFiles(paramCount, paramNames);
	}
	websPreserveCustomSysConf(NULL);
	doSystem("[ -s /mnt/jffs2/sys.conf ] && cp -f /mnt/jffs2/sys.conf /tmp/sys.conf >/dev/null 2>&1; "
	         "[ -s /mnt/jffs2/wlan0.conf ] && cp -f /mnt/jffs2/wlan0.conf /tmp/wlan0.conf >/dev/null 2>&1; "
	         "if [ -x /bin/config_unlock.sh ]; then /bin/config_unlock.sh %d >/dev/null 2>&1; fi",
	         (int)getpid());
	submitToFlash();
}

static void wlReloadDriverAndDaemons(void)
{
	char netType[16];
	char secMode[16];

	doSystem("cd /root/mtlk/etc && /root/mtlk/etc/reload_mtlk_driver.sh >/dev/null 2>&1");
	memset(netType, 0, sizeof(netType));
	memset(secMode, 0, sizeof(secMode));
	MT_Get_Param(T("network_type"), netType, sizeof(netType) - 1);
	MT_Get_Param(T("NonProcSecurityMode"), secMode, sizeof(secMode) - 1);
	if (atoi(netType) == 2) {
		doSystem("killall hostapd >/dev/null 2>&1");
		if (atoi(secMode) > 1) {
			doSystem("/root/mtlk/etc/hostapd -d /mnt/jffs2/hostapd0.conf >/dev/null 2>&1 &");
		}
	}
}

static void wlExecScan(void)
{
	char *buf;
	int associated = 0;

	doSystem("iwconfig wlan0 > /tmp/link_status.txt 2>/dev/null");
	buf = wlReadFileAlloc("/tmp/link_status.txt", NULL);
	if (buf != NULL) {
		char *ap = strstr(buf, "Access Point");
		if (ap != NULL && strstr(ap, "Not-Associated") == NULL) {
			associated = 1;
		}
		free(buf);
	}
	unlink("/tmp/link_status.txt");

	if (associated) {
		doSystem("ifconfig wlan0 up >/dev/null 2>&1");
		doSystem("killall -HUP wpa_supplicant >/dev/null 2>&1");
		sleep(1);
	}
	doSystem("/root/mtlk/etc/mtpriv wlan0 Wildcard_ESSID '' >/dev/null 2>&1");
	doSystem("iwlist wlan0 scan > /tmp/scanList.txt 2>/dev/null");
}

static int wlParseScanList(wlScanEntry_t *entries, int maxEntries)
{
	char *buf;
	char *cellPtr;
	int count = 0;

	buf = wlReadFileAlloc("/tmp/scanList.txt", NULL);
	if (buf == NULL) {
		return 0;
	}
	cellPtr = strstr(buf, "Cell ");
	while (cellPtr != NULL && count < maxEntries) {
		char *nextCell = strstr(cellPtr + 5, "Cell ");
		size_t blkLen = (nextCell != NULL) ? (size_t)(nextCell - cellPtr) : strlen(cellPtr);
		char *blk = (char *)malloc(blkLen + 1);
		wlScanEntry_t *e = &entries[count];
		char *p;
		char *q;
		char *nl;
		int hasWpa1 = 0;
		int hasWpa2 = 0;
		int isPsk = 0;
		int isEap = 0;
		int encOn = 0;

		if (blk == NULL) {
			break;
		}
		memcpy(blk, cellPtr, blkLen);
		blk[blkLen] = '\0';
		memset(e, 0, sizeof(*e));
		strcpy(e->mode, "Auto");
		strcpy(e->band, "2.4");
		strcpy(e->encryType, "Disabled");

		p = strstr(blk, "Address:");
		if (p != NULL) {
			p += 8;
			while (*p == ' ' || *p == '\t') {
				p++;
			}
			wlCopyTrim(p, 17, e->bssid, sizeof(e->bssid));
		}

		p = strstr(blk, "ESSID:");
		if (p != NULL) {
			p += 6;
			while (*p == ' ' || *p == '\t') {
				p++;
			}
			if (*p == '"') {
				p++;
				q = strchr(p, '"');
				if (q != NULL) {
					wlCopyRange(p, (size_t)(q - p), e->essid, sizeof(e->essid), 0);
				}
			} else {
				nl = strchr(p, '\n');
				wlCopyTrim(p, nl != NULL ? (size_t)(nl - p) : strlen(p),
					e->essid, sizeof(e->essid));
			}
		}

		p = strstr(blk, "Mode:");
		if (p != NULL) {
			p += 5;
			while (*p == ' ' || *p == '\t') {
				p++;
			}
			nl = strchr(p, '\n');
			wlCopyTrim(p, nl != NULL ? (size_t)(nl - p) : 15, e->mode, sizeof(e->mode));
			if (strcmp(e->mode, "Auto") != 0 && strcmp(e->mode, "Managed") != 0) {
				strcpy(e->mode, "Managed");
			}
		}

		p = strstr(blk, "Channel:");
		if (p != NULL) {
			e->channel = atoi(p + 8);
			if (e->channel > 14) {
				strcpy(e->band, "5");
			}
		}

		p = strstr(blk, "Signal level=");
		if (p != NULL) {
			p += 13;
			e->signalDbm = atoi(p);
			e->hasSignal = 1;
		} else {
			p = strstr(blk, "Quality=");
			if (p != NULL) {
				e->signalDbm = atoi(p + 8);
				e->hasSignal = 1;
			}
		}

		if (strstr(blk, "5.2") != NULL || e->channel > 14) {
			strcpy(e->band, "5");
		}
		if (strstr(blk, "not WPS") == NULL && strstr(blk, "WPS") != NULL) {
			e->isWps = 1;
		}

		hasWpa2 = (strstr(blk, "WPA2") != NULL || strstr(blk, "RSN") != NULL) ? 1 : 0;
		hasWpa1 = (strstr(blk, "WPA ") != NULL || strstr(blk, "WPA\n") != NULL ||
		           strstr(blk, "WPA Version 1") != NULL) ? 1 : 0;
		isPsk = (strstr(blk, "PSK") != NULL) ? 1 : 0;
		isEap = (strstr(blk, "802.1X") != NULL || strstr(blk, "EAP") != NULL) ? 1 : 0;

		p = strstr(blk, "Encryption key:");
		if (p != NULL && strstr(p, "on") != NULL &&
		    (strchr(p, '\n') == NULL || strstr(p, "on") < strchr(p, '\n'))) {
			encOn = 1;
		}

		if (hasWpa2 && hasWpa1) {
			strcpy(e->encryType, isEap ? "WPA2-Enterprise Mixed" : "WPA2-Personal Mixed");
		} else if (hasWpa2) {
			strcpy(e->encryType, isEap ? "WPA2-Enterprise" : "WPA2-Personal");
		} else if (hasWpa1 || isPsk || isEap) {
			strcpy(e->encryType, isEap ? "WPA-Enterprise" : "WPA-Personal");
		} else if (encOn) {
			strcpy(e->encryType, "WEP");
		} else {
			strcpy(e->encryType, "Disabled");
		}

		free(blk);
		count++;
		cellPtr = nextCell;
	}
	free(buf);
	return count;
}

static void formUpdateIPAddress(webs_t wp, char_t *path, char_t *query)
{
	FILE *fp;
	char line[128];
	char *tok;
	char ip[64];

	(void)path;
	(void)query;

	strcpy(ip, "NULL");
	fp = fopen("/var/Get_IPInfo_From_DHCP", "r");
	if (fp != NULL) {
		memset(line, 0, sizeof(line));
		if (fgets(line, sizeof(line), fp) != NULL) {
			tok = strtok(line, ",\r\n");
			if (tok != NULL && tok[0] != '\0') {
				snprintf(ip, sizeof(ip), "%s", tok);
				ip[sizeof(ip) - 1] = '\0';
				printf("updateIPAddress: DHCP will apply ip %s to WET610N\n", ip);
				{
					FILE *sfp = fopen("/var/success_get_ip", "w");
					if (sfp != NULL) {
						fputs("1\n", sfp);
						fclose(sfp);
					}
				}
			}
		}
		fclose(fp);
	}
	wlSendPlainHeaders(wp);
	websWrite(wp, T("%s"), ip);
	websDone(wp, 200);
}

static void wlApplySecurityCommon(webs_t wp, int redirectAfter)
{
	char_t *nowMode = websGetVar(wp, T("nowMode"), T(""));
	char_t *freqBand = websGetVar(wp, T("frequencyBand"), T(""));
	char_t *secMode = websGetVar(wp, T("securityMode"), T(""));
	char_t *wepRaw[4];
	char wepFmt[4][64];
	static const char *wepKeys[4] = {
		"WepKeys_DefaultKey0", "WepKeys_DefaultKey1",
		"WepKeys_DefaultKey2", "WepKeys_DefaultKey3"
	};
	char_t *defWepKey = websGetVar(wp, T("defaultWEPKey"), T(""));
	char_t *wepLen = websGetVar(wp, T("WEPKeyLength"), T(""));
	char_t *wepAuth = websGetVar(wp, T("WEPAuth"), T(""));
	char_t *wpaPMode = websGetVar(wp, T("WPAPersonalMode"), T(""));
	char_t *wpaPEnc = websGetVar(wp, T("WPAPersonalEncryption"), T(""));
	char_t *wpaPsk = websGetVar(wp, T("WPAPSK"), T(""));
	char_t *wpaEMode = websGetVar(wp, T("WPAEnterpriseMode"), T(""));
	char_t *wpaEEnc = websGetVar(wp, T("WPAEnterpriseEncryption"), T(""));
	char_t *wpaRadIp = websGetVar(wp, T("WPARadiusIP"), T(""));
	char_t *wpaRadPort = websGetVar(wp, T("WPARadiusPort"), T(""));
	char_t *wpaRadKey = websGetVar(wp, T("WPARadiusKey"), T(""));
	char_t *radRekey = websGetVar(wp, T("RadiusReKeyInterval"), T(""));
	char_t *paramNames[32];
	char curBand[16];
	int paramCount = 0;
	int i;

	memset(curBand, 0, sizeof(curBand));
	MT_Get_Param(T("FrequencyBand"), curBand, sizeof(curBand) - 1);

	wepRaw[0] = websGetVar(wp, T("WEPKey0"), T(""));
	wepRaw[1] = websGetVar(wp, T("WEPKey1"), T(""));
	wepRaw[2] = websGetVar(wp, T("WEPKey2"), T(""));
	wepRaw[3] = websGetVar(wp, T("WEPKey3"), T(""));

	if (secMode != NULL && secMode[0] != '\0') {
		MT_Set_Param(T("NonProcSecurityMode"), secMode, 1);
		paramNames[paramCount++] = T("NonProcSecurityMode");
	}
	for (i = 0; i < 4; i++) {
		wlFormatWepKey(wepRaw[i], wepFmt[i], sizeof(wepFmt[i]));
		if (wepFmt[i][0] != '\0') {
			MT_Set_Param(T((char *)wepKeys[i]), wepFmt[i], 1);
			paramNames[paramCount++] = T((char *)wepKeys[i]);
		}
	}
	if (defWepKey != NULL && defWepKey[0] != '\0') {
		MT_Set_Param(T("WepTxKeyIdx"), defWepKey, 1);
		paramNames[paramCount++] = T("WepTxKeyIdx");
	}
	if (wepLen != NULL && wepLen[0] != '\0') {
		MT_Set_Param(T("NonProc_WepKeyLength"), wepLen, 1);
		paramNames[paramCount++] = T("NonProc_WepKeyLength");
	}
	if (wepAuth != NULL && wepAuth[0] != '\0') {
		MT_Set_Param(T("NonProc_Authentication"), wepAuth, 1);
		paramNames[paramCount++] = T("NonProc_Authentication");
	}
	if (secMode != NULL && strcmp(secMode, "2") == 0) {
		MT_Set_Param(T("WepAuto"), T("0"), 1);
		paramNames[paramCount++] = T("WepAuto");
	}
	if (wpaPEnc != NULL && wpaPEnc[0] != '\0') {
		MT_Set_Param(T("NonProc_WPA_Personal_Encapsulation"), wpaPEnc, 1);
		paramNames[paramCount++] = T("NonProc_WPA_Personal_Encapsulation");
	}
	if (wpaPsk != NULL && wpaPsk[0] != '\0') {
		MT_Set_Param(T("NonProc_WPA_Personal_PSK"), wpaPsk, 1);
		paramNames[paramCount++] = T("NonProc_WPA_Personal_PSK");
	}
	if (wpaEMode != NULL && wpaEMode[0] != '\0') {
		MT_Set_Param(T("NonProc_WPA_Enterprise_Mode"), wpaEMode, 1);
		paramNames[paramCount++] = T("NonProc_WPA_Enterprise_Mode");
	}
	if (wpaEEnc != NULL && wpaEEnc[0] != '\0') {
		MT_Set_Param(T("NonProc_WPA_Enterprise_Encapsulation"), wpaEEnc, 1);
		paramNames[paramCount++] = T("NonProc_WPA_Enterprise_Encapsulation");
	}
	if (wpaRadIp != NULL && wpaRadIp[0] != '\0') {
		MT_Set_Param(T("NonProc_WPA_Enterprise_Radius_IP"), wpaRadIp, 1);
		paramNames[paramCount++] = T("NonProc_WPA_Enterprise_Radius_IP");
	}
	if (wpaRadPort != NULL && wpaRadPort[0] != '\0') {
		MT_Set_Param(T("NonProc_WPA_Enterprise_Radius_Port"), wpaRadPort, 1);
		paramNames[paramCount++] = T("NonProc_WPA_Enterprise_Radius_Port");
	}
	if (wpaRadKey != NULL && wpaRadKey[0] != '\0') {
		MT_Set_Param(T("NonProc_WPA_Enterprise_Radius_Key"), wpaRadKey, 1);
		paramNames[paramCount++] = T("NonProc_WPA_Enterprise_Radius_Key");
	}
	if (radRekey != NULL && radRekey[0] != '\0') {
		MT_Set_Param(T("NonProc_WPA_Enterprise_Radius_ReKey_Interval"), radRekey, 1);
		paramNames[paramCount++] = T("NonProc_WPA_Enterprise_Radius_ReKey_Interval");
	}

	if (nowMode != NULL && strcmp(nowMode, "apMode") == 0) {
		if (wpaPMode != NULL && wpaPMode[0] != '\0') {
			MT_Set_Param(T("NonProc_WPA_Personal_Mode"), wpaPMode, 1);
			paramNames[paramCount++] = T("NonProc_WPA_Personal_Mode");
		}
		wlCommitParams(paramCount, paramNames);
		doSystem("killall hostapd >/dev/null 2>&1");
		doSystem("cd /root/mtlk/web && /root/mtlk/web/init_security.sh >/dev/null 2>&1");
		if (secMode != NULL && atoi(secMode) > 1) {
			doSystem("/root/mtlk/etc/hostapd -d /mnt/jffs2/hostapd0.conf >/dev/null 2>&1 &");
		}
		if (redirectAfter) {
			websRedirect(wp, T("/wireless/security.asp"));
		}
	} else if (nowMode != NULL && strcmp(nowMode, "stationMode") == 0) {
		char_t *essid = websGetVar(wp, T("ESSID"), T(""));
		char_t *bssid = websGetVar(wp, T("BSSID"), T(""));
		char quotedSsid[300];
		char pModeBuf[8];

		if (essid == NULL || essid[0] == '\0') {
			return;
		}
		if (freqBand != NULL && freqBand[0] != '\0') {
			MT_Set_Param(T("FrequencyBand"), freqBand, 1);
			paramNames[paramCount++] = T("FrequencyBand");
		}
		MT_Set_Param(T("NonProc_ESSID"), essid, 1);
		paramNames[paramCount++] = T("NonProc_ESSID");
		MT_Set_Param(T("Wildcard_ESSID"), essid, 1);
		paramNames[paramCount++] = T("Wildcard_ESSID");
		if (bssid != NULL && bssid[0] != '\0') {
			MT_Set_Param(T("BSSID"), bssid, 1);
			paramNames[paramCount++] = T("BSSID");
		}
		if (wpaPMode != NULL && wpaPMode[0] != '\0') {
			snprintf(pModeBuf, sizeof(pModeBuf), "%s", wpaPMode);
		} else {
			strcpy(pModeBuf, (wpaPEnc != NULL && strcmp(wpaPEnc, "2") == 0) ? "3" : "2");
		}
		MT_Set_Param(T("NonProc_WPA_Personal_Mode"), pModeBuf, 1);
		paramNames[paramCount++] = T("NonProc_WPA_Personal_Mode");
		MT_Set_Param(T("unconfigured"), T("0"), 1);
		paramNames[paramCount++] = T("unconfigured");
		MT_Set_Param(T("forceWlanDisconnect"), T("0"), 1);
		paramNames[paramCount++] = T("forceWlanDisconnect");

		wlCommitParams(paramCount, paramNames);
		wlShellSingleQuote(essid, quotedSsid, sizeof(quotedSsid));
		doSystem("/root/mtlk/etc/mtpriv wlan0 Wildcard_ESSID %s >/dev/null 2>&1", quotedSsid);
		doSystem("cd /root/mtlk/web && /root/mtlk/web/init_security.sh reactivate >/dev/null 2>&1");
	}
}

static void formWirelessSecurity(webs_t wp, char_t *path, char_t *query)
{
	(void)path;
	(void)query;
	wlApplySecurityCommon(wp, 1);
}

static void formWirelessWPS(webs_t wp, char_t *path, char_t *query)
{
	char_t *wpsEn = websGetVar(wp, T("WPSEnable"), T(""));
	char_t *paramNames[1];

	(void)path;
	(void)query;

	if (wpsEn != NULL && wlIsDigitsInRange(wpsEn, 0, 1)) {
		MT_Set_Param(T("NonProc_WPS_ActivateWPS"), wpsEn, 1);
		paramNames[0] = T("NonProc_WPS_ActivateWPS");
		wlCommitParams(1, paramNames);
	}
	websRedirect(wp, T("/wireless/wps.asp"));
}

static void formAccessControl(webs_t wp, char_t *path, char_t *query)
{
	static const char *acPrefix[4] = { "AC_BK", "AC_BE", "AC_VI", "AC_VO" };
	char_t *nowMode;
	char_t *accessMode;
	char_t *curWmm;
	char_t *useAggr;
	char_t *accAggr;
	char_t *maxPkt;
	char_t *maxByte;
	char_t *tmoInt;
	char_t *minSize;
	char_t *addbaTmo;
	char_t *aggrWin;
	char_t *cwMinSta;
	char_t *cwMaxSta;
	char_t *aifsnSta;
	char_t *txopSta;
	char_t *cwMinAp;
	char_t *cwMaxAp;
	char_t *aifsnAp;
	char_t *txopAp;
	int modeIdx;
	int isAp;
	int pid;
	char kBuf[13][64];
	char_t *paramNames[16];
	int n = 0;

	(void)path;
	(void)query;

	if (checkLoadDefault() == 1) {
		websRedirect(wp, T("/admin/upgrade.asp"));
		return;
	}
	if (isWaitOtherProcessBusy() != 0) {
		websError(wp, 503, T("Device busy applying configuration"));
		return;
	}

	nowMode = websGetVar(wp, T("nowMode"), T(""));
	accessMode = websGetVar(wp, T("accessMode"), T("0"));
	curWmm = websGetVar(wp, T("CurrentWMM"), T(""));
	useAggr = websGetVar(wp, T("AC_BK_UseAggregation"), T(""));
	accAggr = websGetVar(wp, T("AC_BK_AcceptAggregation"), T(""));
	maxPkt = websGetVar(wp, T("AC_BK_MaxNumOfPackets"), T(""));
	maxByte = websGetVar(wp, T("AC_BK_MaxNumOfBytes"), T(""));
	tmoInt = websGetVar(wp, T("AC_BK_TimeoutInterval"), T(""));
	minSize = websGetVar(wp, T("AC_BK_MinSizeOfPacketInAggr"), T(""));
	addbaTmo = websGetVar(wp, T("AC_BK_ADDBATimeout"), T(""));
	aggrWin = websGetVar(wp, T("AC_BK_AggregationWindowSize"), T(""));
	cwMinSta = websGetVar(wp, T("selectCWMinSTA"), T(""));
	cwMaxSta = websGetVar(wp, T("selectCWMaxSTA"), T(""));
	aifsnSta = websGetVar(wp, T("AC_BK_AIFSN"), T(""));
	txopSta = websGetVar(wp, T("AC_BK_TXOP"), T(""));
	cwMinAp = websGetVar(wp, T("selectCWMinAP"), T(""));
	cwMaxAp = websGetVar(wp, T("selectCWMaxAP"), T(""));
	aifsnAp = websGetVar(wp, T("AC_BK_AIFSN_AP"), T(""));
	txopAp = websGetVar(wp, T("AC_BK_TXOP_AP"), T(""));

	modeIdx = atoi(accessMode);
	if (modeIdx < 0 || modeIdx > 3) {
		websRedirect(wp, T("/station/wmm.asp"));
		return;
	}
	isAp = (nowMode != NULL && strcmp(nowMode, "apMode") == 0) ? 1 : 0;

	saveOldWebPid(getpid(), "wmm.asp");
	setWaitOtherProcess(1);
	pid = fork();
	if (pid < 0) {
		setWaitOtherProcess(0);
		unlink("/var/old_web_pid");
		websError(wp, 500, T("Unable to apply WMM settings"));
		return;
	}
	if (pid != 0) {
		websRedirect(wp, T("/wait_page.asp"));
		return;
	}

	wlCloseInheritedFds();
	waitRedirectPage();
	stopWPS_PBC();

	snprintf(kBuf[0], sizeof(kBuf[0]), "%s_UseAggregation", acPrefix[modeIdx]);
	snprintf(kBuf[1], sizeof(kBuf[1]), "%s_AcceptAggregation", acPrefix[modeIdx]);
	snprintf(kBuf[2], sizeof(kBuf[2]), "%s_MaxNumOfPackets", acPrefix[modeIdx]);
	snprintf(kBuf[3], sizeof(kBuf[3]), "%s_MaxNumOfBytes", acPrefix[modeIdx]);
	snprintf(kBuf[4], sizeof(kBuf[4]), "%s_TimeoutInterval", acPrefix[modeIdx]);
	snprintf(kBuf[5], sizeof(kBuf[5]), "%s_MinSizeOfPacketInAggr", acPrefix[modeIdx]);
	snprintf(kBuf[6], sizeof(kBuf[6]), "%s_ADDBATimeout", acPrefix[modeIdx]);
	snprintf(kBuf[7], sizeof(kBuf[7]), "%s_AggregationWindowSize", acPrefix[modeIdx]);

	if (useAggr[0] != '\0') {
		MT_Set_Param(kBuf[0], useAggr, 1);
		paramNames[n++] = kBuf[0];
	}
	if (accAggr[0] != '\0') {
		MT_Set_Param(kBuf[1], accAggr, 1);
		paramNames[n++] = kBuf[1];
	}
	if (maxPkt[0] != '\0') {
		MT_Set_Param(kBuf[2], maxPkt, 1);
		paramNames[n++] = kBuf[2];
	}
	if (maxByte[0] != '\0') {
		MT_Set_Param(kBuf[3], maxByte, 1);
		paramNames[n++] = kBuf[3];
	}
	if (tmoInt[0] != '\0') {
		MT_Set_Param(kBuf[4], tmoInt, 1);
		paramNames[n++] = kBuf[4];
	}
	if (minSize[0] != '\0') {
		MT_Set_Param(kBuf[5], minSize, 1);
		paramNames[n++] = kBuf[5];
	}
	if (addbaTmo[0] != '\0') {
		MT_Set_Param(kBuf[6], addbaTmo, 1);
		paramNames[n++] = kBuf[6];
	}
	if (aggrWin[0] != '\0') {
		MT_Set_Param(kBuf[7], aggrWin, 1);
		paramNames[n++] = kBuf[7];
	}
	if (curWmm[0] != '\0') {
		MT_Set_Param(T("CurrentWMM"), curWmm, 1);
		paramNames[n++] = T("CurrentWMM");
	}

	if (isAp) {
		snprintf(kBuf[8], sizeof(kBuf[8]), "%s_CWmin_AP", acPrefix[modeIdx]);
		snprintf(kBuf[9], sizeof(kBuf[9]), "%s_CWmax_AP", acPrefix[modeIdx]);
		snprintf(kBuf[10], sizeof(kBuf[10]), "%s_AIFSN_AP", acPrefix[modeIdx]);
		snprintf(kBuf[11], sizeof(kBuf[11]), "%s_TXOP_AP", acPrefix[modeIdx]);
		if (cwMinAp[0] != '\0') { MT_Set_Param(kBuf[8], cwMinAp, 1); paramNames[n++] = kBuf[8]; }
		if (cwMaxAp[0] != '\0') { MT_Set_Param(kBuf[9], cwMaxAp, 1); paramNames[n++] = kBuf[9]; }
		if (aifsnAp[0] != '\0') { MT_Set_Param(kBuf[10], aifsnAp, 1); paramNames[n++] = kBuf[10]; }
		if (txopAp[0] != '\0') { MT_Set_Param(kBuf[11], txopAp, 1); paramNames[n++] = kBuf[11]; }
	} else {
		snprintf(kBuf[8], sizeof(kBuf[8]), "%s_CWmin", acPrefix[modeIdx]);
		snprintf(kBuf[9], sizeof(kBuf[9]), "%s_CWmax", acPrefix[modeIdx]);
		snprintf(kBuf[10], sizeof(kBuf[10]), "%s_AIFSN", acPrefix[modeIdx]);
		snprintf(kBuf[11], sizeof(kBuf[11]), "%s_TXOP", acPrefix[modeIdx]);
		if (cwMinSta[0] != '\0') { MT_Set_Param(kBuf[8], cwMinSta, 1); paramNames[n++] = kBuf[8]; }
		if (cwMaxSta[0] != '\0') { MT_Set_Param(kBuf[9], cwMaxSta, 1); paramNames[n++] = kBuf[9]; }
		if (aifsnSta[0] != '\0') { MT_Set_Param(kBuf[10], aifsnSta, 1); paramNames[n++] = kBuf[10]; }
		if (txopSta[0] != '\0') { MT_Set_Param(kBuf[11], txopSta, 1); paramNames[n++] = kBuf[11]; }
	}

	wlCommitParams(n, paramNames);
	wlReloadDriverAndDaemons();
	startWPS_PBC();
	childFinishAndExit();
}

static void formMacFilter(webs_t wp, char_t *path, char_t *query)
{
	char_t *aclMode = websGetVar(wp, T("AclMode"), T("0"));
	char aclBuf[320];
	char varName[16];
	char_t *macVal;
	char_t *paramNames[2];
	int i;
	int first = 1;

	(void)path;
	(void)query;

	memset(aclBuf, 0, sizeof(aclBuf));
	for (i = 1; i <= 16; i++) {
		snprintf(varName, sizeof(varName), "mac_%d", i);
		macVal = websGetVar(wp, varName, T(""));
		if (macVal != NULL && macVal[0] != '\0' &&
		    strcmp(macVal, "00:00:00:00:00:00") != 0 && wlIsValidMac(macVal)) {
			if (!first && strlen(aclBuf) + 1 < sizeof(aclBuf)) {
				strcat(aclBuf, "/");
			}
			if (strlen(aclBuf) + strlen(macVal) + 1 < sizeof(aclBuf)) {
				strcat(aclBuf, macVal);
				first = 0;
			}
		}
	}

	if (!wlIsDigitsInRange(aclMode, 0, 2)) {
		websError(wp, 400, T("Invalid ACL mode"));
		return;
	}
	MT_Set_Param(T("AclMode"), aclMode, 1);
	MT_Set_Param(T("ACL"), aclBuf, 1);
	paramNames[0] = T("AclMode");
	paramNames[1] = T("ACL");
	wlCommitParams(2, paramNames);
	wlReloadDriverAndDaemons();
	websRedirect(wp, T("/wireless/mac_filter.asp"));
}

static void formAdvancedWireless(webs_t wp, char_t *path, char_t *query)
{
	char_t *nowMode;
	char_t *advCoding;
	char_t *erpProt;
	char_t *protType;
	char_t *pwrSel;
	char_t *qMap;
	char_t *ethDownUp;
	char_t *radarDet;
	char_t *aocsRestrict;
	char_t *hiddenSsid;
	char_t *obssProt;
	char_t *apFwd;
	char_t *relMcast;
	char_t *paramNames[16];
	int paramCount = 0;
	int pid;

	(void)path;
	(void)query;

	if (checkLoadDefault() == 1) {
		websRedirect(wp, T("/admin/upgrade.asp"));
		return;
	}
	if (isWaitOtherProcessBusy() != 0) {
		websError(wp, 503, T("Device busy applying configuration"));
		return;
	}

	nowMode = websGetVar(wp, T("nowMode"), T(""));
	advCoding = websGetVar(wp, T("AdvancedCoding"), T(""));
	erpProt = websGetVar(wp, T("ERPProtectionType"), T(""));
	protType = websGetVar(wp, T("ProtectionType"), T(""));
	pwrSel = websGetVar(wp, T("PowerSelection"), T(""));
	qMap = websGetVar(wp, T("Use11QMap"), T(""));
	ethDownUp = websGetVar(wp, T("EthDownUp"), T(""));
	radarDet = websGetVar(wp, T("RadarDetect"), T(""));
	aocsRestrict = websGetVar(wp, T("AocsRestrictCh"), T(""));
	hiddenSsid = websGetVar(wp, T("HiddenSSID"), T(""));
	obssProt = websGetVar(wp, T("OverlappingBSSProtection"), T(""));
	apFwd = websGetVar(wp, T("APforwarding"), T(""));
	relMcast = websGetVar(wp, T("ReliableMulticast"), T(""));

	saveOldWebPid(getpid(), "wireless_advanced.asp");
	setWaitOtherProcess(1);
	pid = fork();
	if (pid < 0) {
		setWaitOtherProcess(0);
		unlink("/var/old_web_pid");
		websError(wp, 500, T("Unable to apply advanced wireless settings"));
		return;
	}
	if (pid != 0) {
		websRedirect(wp, T("/wait_page.asp"));
		return;
	}

	wlCloseInheritedFds();
	waitRedirectPage();
	stopWPS_PBC();

	if (strcmp(nowMode, "apMode") == 0) {
		if (radarDet[0] != '\0') { MT_Set_Param(T("11hRadarDetect"), radarDet, 1); paramNames[paramCount++] = T("11hRadarDetect"); }
		if (aocsRestrict[0] != '\0') { MT_Set_Param(T("AocsRestrictCh"), aocsRestrict, 1); paramNames[paramCount++] = T("AocsRestrictCh"); }
		if (advCoding[0] != '\0') { MT_Set_Param(T("AdvancedCoding"), advCoding, 1); paramNames[paramCount++] = T("AdvancedCoding"); }
		if (hiddenSsid[0] != '\0') { MT_Set_Param(T("HiddenSSID"), hiddenSsid, 1); paramNames[paramCount++] = T("HiddenSSID"); }
		if (erpProt[0] != '\0') { MT_Set_Param(T("ERPProtectionType"), erpProt, 1); paramNames[paramCount++] = T("ERPProtectionType"); }
		if (obssProt[0] != '\0') { MT_Set_Param(T("OverlappingBSSProtection"), obssProt, 1); paramNames[paramCount++] = T("OverlappingBSSProtection"); }
		if (protType[0] != '\0') { MT_Set_Param(T("11nProtectionType"), protType, 1); paramNames[paramCount++] = T("11nProtectionType"); }
		if (apFwd[0] != '\0') { MT_Set_Param(T("APforwarding"), apFwd, 1); paramNames[paramCount++] = T("APforwarding"); }
		if (relMcast[0] != '\0') { MT_Set_Param(T("ReliableMulticast"), relMcast, 1); paramNames[paramCount++] = T("ReliableMulticast"); }
		if (pwrSel[0] != '\0') { MT_Set_Param(T("PowerSelection"), pwrSel, 1); paramNames[paramCount++] = T("PowerSelection"); }
		if (qMap[0] != '\0') { MT_Set_Param(T("Use11QMap"), qMap, 1); paramNames[paramCount++] = T("Use11QMap"); }
	} else {
		if (advCoding[0] != '\0') { MT_Set_Param(T("AdvancedCoding"), advCoding, 1); paramNames[paramCount++] = T("AdvancedCoding"); }
		if (erpProt[0] != '\0') { MT_Set_Param(T("ERPProtectionType"), erpProt, 1); paramNames[paramCount++] = T("ERPProtectionType"); }
		if (protType[0] != '\0') { MT_Set_Param(T("11nProtectionType"), protType, 1); paramNames[paramCount++] = T("11nProtectionType"); }
		if (pwrSel[0] != '\0') { MT_Set_Param(T("PowerSelection"), pwrSel, 1); paramNames[paramCount++] = T("PowerSelection"); }
		if (qMap[0] != '\0') { MT_Set_Param(T("Use11QMap"), qMap, 1); paramNames[paramCount++] = T("Use11QMap"); }
		if (ethDownUp[0] != '\0') { MT_Set_Param(T("EthDownUp"), ethDownUp, 1); paramNames[paramCount++] = T("EthDownUp"); }
	}

	wlCommitParams(paramCount, paramNames);
	wlReloadDriverAndDaemons();
	startWPS_PBC();
	childFinishAndExit();
}

static void formStationScan(webs_t wp, char_t *path, char_t *query)
{
	char_t *activeMode;
	int pid;

	(void)path;
	(void)query;

	if (checkLoadDefault() == 1) {
		websRedirect(wp, T("/admin/upgrade.asp"));
		return;
	}
	if (isWaitOtherProcessBusy() != 0) {
		websError(wp, 503, T("Device busy applying configuration"));
		return;
	}

	activeMode = websGetVar(wp, T("activeMode"), T(""));
	saveOldWebPid(getpid(), "sta_link_status.asp");
	setWaitOtherProcess(1);
	pid = fork();
	if (pid < 0) {
		setWaitOtherProcess(0);
		unlink("/var/old_web_pid");
		websError(wp, 500, T("Unable to apply wireless scan selection"));
		return;
	}
	if (pid != 0) {
		websRedirect(wp, T("/wait_page.asp"));
		return;
	}

	wlCloseInheritedFds();
	waitRedirectPage();
	if (strcmp(activeMode, "reScan") == 0) {
		wlExecScan();
	} else if (strcmp(activeMode, "connectToAP") == 0) {
		wlApplySecurityCommon(wp, 0);
	}
	childFinishAndExit();
}

static void formUpdateSiteSurvey(webs_t wp, char_t *path, char_t *query)
{
	wlScanEntry_t *entries;
	int count;
	int i;
	char curEssid[128];
	char curBssid[32];
	char hdrBuf[256];
	char escSsid[256];
	char encDisplay[48];
	FILE *fp;

	(void)path;
	(void)query;

	if (checkLoadDefault() == 1) {
		wlSendPlainHeaders(wp);
		websWrite(wp, T(""));
		websDone(wp, 200);
		return;
	}
	stopWPS_PBC();
	if (MT_get_station_layer2_link_status() == 1) {
		fp = fopen("/var/scanning", "w");
		if (fp != NULL) {
			fputs("1\n", fp);
			fclose(fp);
		}
	}

	wlExecScan();
	memset(curEssid, 0, sizeof(curEssid));
	memset(curBssid, 0, sizeof(curBssid));
	MT_Get_Param(T("NonProc_ESSID"), curEssid, sizeof(curEssid) - 1);
	MT_Get_Param(T("BSSID"), curBssid, sizeof(curBssid) - 1);

	entries = (wlScanEntry_t *)calloc(WL_MAX_SCAN_ENTRIES, sizeof(wlScanEntry_t));
	count = (entries != NULL) ? wlParseScanList(entries, WL_MAX_SCAN_ENTRIES) : 0;

	wlSendPlainHeaders(wp);
	snprintf(hdrBuf, sizeof(hdrBuf),
		"<div style=\"font-size:11pt; font-weight: bold; text-align:center;\">"
		"<!--#tr id=\"w.wnss.2-24\" -->Number of Wireless Networks:<!--#endtr--> %d</div><br>",
		count);
	wlWriteTrString(wp, hdrBuf);
	websWrite(wp, T("<table border=\"1\" cellspacing=\"1\" cellpadding=\"1\" "
		"style=\"width:420px;border:solid; border-width:1px;vertical-align:top;\"><tr>"));
	wlWriteTrString(wp, "<td style=\"width:38px;font-size:9pt;font-weight:bolder;text-align:center;word-wrap:break-word;\"><!--#tr id=\"w.wnss.2-25\" -->Select<!--#endtr--></td>");
	wlWriteTrString(wp, "<td style=\"width:183px;font-size:9pt;font-weight:bolder;text-align:center;word-wrap:break-word;\"><!--#tr id=\"w.wnss.2-26\" -->Wireless Network Name<!--#endtr--></td>");
	wlWriteTrString(wp, "<td style=\"width:30px;font-size:9pt;font-weight:bolder;text-align:center;word-wrap:break-word;\"><!--#tr id=\"w.wnss.2-27\" -->GHz<!--#endtr--></td>");
	wlWriteTrString(wp, "<td style=\"width:40px;font-size:9pt;font-weight:bolder;text-align:center;word-wrap:break-word;\"><!--#tr id=\"w.wnss.2-28\" -->Signal<!--#endtr--></td>");
	wlWriteTrString(wp, "<td style=\"width:130px;font-size:9pt;font-weight:bolder;text-align:center;\"><!--#tr id=\"w.wnss.2-29\" -->Security<!--#endtr--></td>");
	websWrite(wp, T("</tr><input type=hidden id=arrayCount name=arrayCount value=\"%d\">"), count);

	for (i = 0; i < count; i++) {
		wlScanEntry_t *e = &entries[i];
		int selected = 0;

		if (curEssid[0] != '\0' && strcmp(e->essid, curEssid) == 0 &&
		    (strcmp(curBssid, "Auto") == 0 || curBssid[0] == '\0' || strcmp(e->bssid, curBssid) == 0)) {
			selected = 1;
		}
		websWrite(wp,
			T("<tr><td style=\"width:38px;word-wrap:break-word;\" align=\"center\">"
			  "<input type=radio id=setRadio_%d name=\"setRadio\"%s></td>"),
			i, selected ? " checked" : "");
		websWrite(wp, T("<input type=\"hidden\" id=\"BSSID_%d\" name=\"BSSID_%d\" value=\"%s\">"),
			i, i, e->bssid);
		wlHtmlEscape(e->essid, escSsid, sizeof(escSsid));
		websWrite(wp,
			T("<td style=\"width:183px;word-wrap:break-word;\" align=\"left\" id=ESSID_%d>"
			  "<div id=\"ssidField\">%s</div></td>"),
			i, escSsid);
		websWrite(wp,
			T("<td style=\"width:30px;word-wrap:break-word;\" id=freqBand_%d align=\"center\">%s</td>"),
			i, e->band);
		websWrite(wp, T("<input type=\"hidden\" id=\"channel_%d\" name=\"channel_%d\" value=\"%d\">"),
			i, i, e->channel);
		websWrite(wp, T("<input type=\"hidden\" id=\"encryType_%d\" name=\"encryType_%d\" value=\"%s\">"),
			i, i, e->encryType);
		if (e->hasSignal) {
			websWrite(wp,
				T("<td style=\"width:40px;word-wrap:break-word;\" id=singalLevel_%d align=\"center\">%d%c</td>"),
				i, e->signalDbm, '%');
		} else {
			websWrite(wp,
				T("<td style=\"width:40px;word-wrap:break-word;\" id=singalLevel_%d align=\"center\">0%c</td>"),
				i, '%');
		}
		if (strcmp(e->encryType, "WPA-Personal") == 0) {
			strcpy(encDisplay, "WPA Personal");
		} else if (strcmp(e->encryType, "WPA2-Personal") == 0 || strcmp(e->encryType, "WPA2-Personal Mixed") == 0) {
			strcpy(encDisplay, "WPA2 Personal");
		} else if (strcmp(e->encryType, "WPA-Enterprise") == 0) {
			strcpy(encDisplay, "WPA Enterprise");
		} else if (strcmp(e->encryType, "WPA2-Enterprise") == 0 || strcmp(e->encryType, "WPA2-Enterprise Mixed") == 0) {
			strcpy(encDisplay, "WPA2 Enterprise");
		} else {
			snprintf(encDisplay, sizeof(encDisplay), "%s", e->encryType);
		}
		websWrite(wp,
			T("<td style=\"width:130px;word-wrap:break-word;\" align=\"left\"><table><tr>"
			  "<td style=\"width:25px;word-wrap:break-word;\"%s></td>"
			  "<td style=\"width:100px\">%s</td></tr></table></td></tr>"),
			e->isWps ? " style=\"background-image:url(../image/WFA_WPS_Mark_small.gif); background-repeat:no-repeat;\"" : "",
			encDisplay);
	}
	websWrite(wp, T("</table><br>"));
	if (entries != NULL) {
		free(entries);
	}
	startWPS_PBC();
	websDone(wp, 200);
}

static void wlReadIwconfigLink(char *essid, size_t essidSz, char *bssid, size_t bssidSz,
                               int *channelOut, char *bitRate, size_t bitRateSz,
                               int *sigDbm, int *connectedOut)
{
	char *buf;
	char *p;
	char *q;

	essid[0] = '\0';
	bssid[0] = '\0';
	bitRate[0] = '\0';
	*channelOut = 0;
	*sigDbm = 0;
	*connectedOut = 0;

	doSystem("iwconfig wlan0 > /tmp/updateWPSStatus.txt 2>/dev/null");
	buf = wlReadFileAlloc("/tmp/updateWPSStatus.txt", NULL);
	unlink("/tmp/updateWPSStatus.txt");
	if (buf == NULL) {
		return;
	}

	p = strstr(buf, "ESSID:");
	if (p != NULL) {
		p += 6;
		if (*p == '"') {
			p++;
			q = strchr(p, '"');
			if (q != NULL) {
				wlCopyRange(p, (size_t)(q - p), essid, essidSz, 0);
			}
		}
	}
	p = strstr(buf, "Channel=");
	if (p != NULL) {
		*channelOut = atoi(p + 8);
	}
	p = strstr(buf, "Access Point:");
	if (p != NULL) {
		p += 13;
		while (*p == ' ' || *p == '\t') {
			p++;
		}
		if (strncmp(p, "Not-Associated", 14) != 0) {
			wlCopyTrim(p, 17, bssid, bssidSz);
			*connectedOut = 1;
		}
	}
	p = strstr(buf, "Bit Rate");
	if (p != NULL) {
		char *eq = strchr(p, ':');
		if (eq == NULL) {
			eq = strchr(p, '=');
		}
		if (eq != NULL) {
			q = eq + 1;
			while (*q == ' ') {
				q++;
			}
			wlCopyTrim(q, 24, bitRate, bitRateSz);
		}
	}
	p = strstr(buf, "Signal level");
	if (p != NULL) {
		char *eq = strchr(p, '=');
		if (eq != NULL) {
			*sigDbm = atoi(eq + 1);
		}
	}
	free(buf);
}

static void formUpdateWPSStatus(webs_t wp, char_t *path, char_t *query)
{
	char secModeStr[16];
	char wpaModeStr[16];
	char essid[128];
	char escSsid[256];
	char bssid[32];
	char bitRate[32];
	const char *secLabel;
	char htmlBuf[1024];
	int channel = 0;
	int sig = 0;
	int linkUp = 0;

	(void)path;
	(void)query;

	memset(secModeStr, 0, sizeof(secModeStr));
	memset(wpaModeStr, 0, sizeof(wpaModeStr));
	MT_Get_Param(T("NonProcSecurityMode"), secModeStr, sizeof(secModeStr) - 1);
	MT_Get_Param(T("NonProc_WPA_Personal_Mode"), wpaModeStr, sizeof(wpaModeStr) - 1);

	switch (atoi(secModeStr)) {
	case 2:
		secLabel = "<!--#tr id=\"w.bws.2-3\" -->WEP<!--#endtr-->";
		break;
	case 3:
		secLabel = (atoi(wpaModeStr) == 1)
			? "<!--#tr id=\"w.bws.2-4\" -->WPA Personal<!--#endtr-->"
			: "<!--#tr id=\"w.bws.2-5\" -->WPA2 Personal<!--#endtr-->";
		break;
	default:
		secLabel = "<!--#tr id=\"w.bws.2-2\" -->Disabled<!--#endtr-->";
		break;
	}

	wlReadIwconfigLink(essid, sizeof(essid), bssid, sizeof(bssid),
		&channel, bitRate, sizeof(bitRate), &sig, &linkUp);
	wlHtmlEscape(essid, escSsid, sizeof(escSsid));

	wlSendPlainHeaders(wp);
	if (MT_get_station_link_status() == 1 && linkUp) {
		snprintf(htmlBuf, sizeof(htmlBuf),
			"<table><tr><td class=\"WPSStatus\"><!--#tr id=\"w.bws.2-31\" -->Link Status:<!--#endtr--></td>"
			"<td><!--#tr id=\"w.bws.2-35\" -->Connected<!--#endtr--></td></tr>"
			"<tr><td class=\"WPSStatus\"><!--#tr id=\"w.bws.2-33\" -->Network Name (SSID):<!--#endtr--></td>"
			"<td>%s</td></tr>"
			"<tr><td class=\"WPSStatus\"><!--#tr id=\"w.bws.2-34\" -->Security:<!--#endtr--></td>"
			"<td>%s</td></tr></table>",
			escSsid, secLabel);
	} else {
		snprintf(htmlBuf, sizeof(htmlBuf),
			"<table><tr><td class=\"WPSStatus\"><!--#tr id=\"w.bws.2-31\" -->Link Status:<!--#endtr--></td>"
			"<td><!--#tr id=\"w.bws.2-32\" -->Disconnected<!--#endtr--></td></tr>"
			"<tr><td class=\"WPSStatus\"><!--#tr id=\"w.bws.2-33\" -->Network Name (SSID):<!--#endtr--></td>"
			"<td></td></tr>"
			"<tr><td class=\"WPSStatus\"><!--#tr id=\"w.bws.2-34\" -->Security:<!--#endtr--></td>"
			"<td></td></tr></table>");
	}
	wlWriteTrString(wp, htmlBuf);
	websDone(wp, 200);
}

static void wlWriteLinkStatusRow(webs_t wp, const char *labelTag, const char *valHtml)
{
	websWrite(wp, T("<tr><td class=\"subMenuSubContent\"></td><td class=\"subMenuLeftSide\"></td><td class=\"tdLabel\">"));
	wlWriteTrString(wp, labelTag);
	websWrite(wp, T("</td><td class=\"tdContent\">"));
	if (valHtml != NULL && valHtml[0] != '\0') {
		wlWriteTrString(wp, valHtml);
	}
	websWrite(wp, T("</td></tr>"));
}

static void formUpdateLinkStatus(webs_t wp, char_t *path, char_t *query)
{
	char secModeStr[16];
	char wpaModeStr[16];
	char freqBandCfg[16];
	char bondingCfg[16];
	char ulBondingCfg[16];
	char macStr[32];
	char essid[128];
	char escSsid[256];
	char bssid[32];
	char bitRate[32];
	char radioBand[16];
	char chanWidth[16];
	char wideChan[16];
	char stdChanStr[16];
	char sigStr[24];
	char *conBuf;
	char *statBuf;
	const char *secLabel;
	int channel = 0;
	int sig = 0;
	int linkUp = 0;
	int haveGConInfo = 0;

	(void)path;
	(void)query;

	memset(secModeStr, 0, sizeof(secModeStr));
	memset(wpaModeStr, 0, sizeof(wpaModeStr));
	memset(freqBandCfg, 0, sizeof(freqBandCfg));
	memset(bondingCfg, 0, sizeof(bondingCfg));
	memset(ulBondingCfg, 0, sizeof(ulBondingCfg));
	memset(macStr, 0, sizeof(macStr));
	MT_Get_Param(T("NonProcSecurityMode"), secModeStr, sizeof(secModeStr) - 1);
	MT_Get_Param(T("NonProc_WPA_Personal_Mode"), wpaModeStr, sizeof(wpaModeStr) - 1);
	MT_Get_Param(T("FrequencyBand"), freqBandCfg, sizeof(freqBandCfg) - 1);
	MT_Get_Param(T("ChannelBonding"), bondingCfg, sizeof(bondingCfg) - 1);
	MT_Get_Param(T("UpperLowerChannelBonding"), ulBondingCfg, sizeof(ulBondingCfg) - 1);
	MT_Get_Param(T("MAC"), macStr, sizeof(macStr) - 1);

	switch (atoi(secModeStr)) {
	case 2:
		secLabel = "WEP";
		break;
	case 3:
		secLabel = (atoi(wpaModeStr) == 1) ? "WPA Personal" : "WPA2 Personal";
		break;
	default:
		secLabel = "<!--#tr id=\"w.bws.2-2\" -->Disabled<!--#endtr-->";
		break;
	}

	wlReadIwconfigLink(essid, sizeof(essid), bssid, sizeof(bssid),
		&channel, bitRate, sizeof(bitRate), &sig, &linkUp);
	wlHtmlEscape(essid, escSsid, sizeof(escSsid));

	strcpy(radioBand, (channel > 14 || (channel == 0 && atoi(freqBandCfg) == 0)) ? "5 GHz" : "2.4 GHz");
	strcpy(chanWidth, "20 MHz");
	strcpy(wideChan, "-");

	doSystem("iwpriv wlan0 g_ConInfo > /tmp/updateLinkConInfo.txt 2>/dev/null");
	conBuf = wlReadFileAlloc("/tmp/updateLinkConInfo.txt", NULL);
	unlink("/tmp/updateLinkConInfo.txt");
	if (conBuf != NULL) {
		if (strstr(conBuf, "Not supported") == NULL) {
			haveGConInfo = 1;
			if (strstr(conBuf, "5.2GHz") != NULL || strstr(conBuf, "5 GHz") != NULL || channel > 14) {
				strcpy(radioBand, "5 GHz");
			}
			if (strstr(conBuf, "40MHz") != NULL || strstr(conBuf, "40 MHz") != NULL) {
				strcpy(chanWidth, "40 MHz");
				snprintf(wideChan, sizeof(wideChan), "%d",
					strstr(conBuf, "Lower") != NULL ? (channel - 2) : (channel + 2));
			}
		}
		free(conBuf);
	}
	if (!haveGConInfo && atoi(bondingCfg) == 1) {
		strcpy(chanWidth, "40 MHz");
		snprintf(wideChan, sizeof(wideChan), "%d",
			atoi(ulBondingCfg) == 1 ? (channel - 2) : (channel + 2));
	}

	if (bitRate[0] == '\0' || sig == 0) {
		doSystem("/root/mtlk/etc/mtdump wlan0 constatus > /tmp/updateLinkConstat.txt 2>/dev/null");
		statBuf = wlReadFileAlloc("/tmp/updateLinkConstat.txt", NULL);
		unlink("/tmp/updateLinkConstat.txt");
		if (statBuf != NULL) {
			char *line = statBuf;
			while (line != NULL && *line != '\0') {
				char *nextNl = strchr(line, '\n');
				char *p1 = strchr(line, '|');
				char *p2 = (p1 != NULL) ? strchr(p1 + 1, '|') : NULL;
				if (p1 != NULL && p2 != NULL && strstr(line, "RSSI") == NULL && strchr(line, ':') != NULL) {
					double rateVal;
					if (sig == 0) {
						int parsedSig = atoi(p1 + 1);
						if (parsedSig < 0) {
							sig = parsedSig;
						}
					}
					if (bitRate[0] == '\0') {
						rateVal = atof(p2 + 1);
						if (rateVal > 0.0) {
							snprintf(bitRate, sizeof(bitRate), "%.1f Mbps", rateVal);
						}
					}
					break;
				}
				line = (nextNl != NULL) ? (nextNl + 1) : NULL;
			}
			free(statBuf);
		}
	}

	snprintf(stdChanStr, sizeof(stdChanStr), "%d", channel);
	if (sig != 0) {
		snprintf(sigStr, sizeof(sigStr), "%d dBm", sig);
	} else {
		strcpy(sigStr, "N/A dBm");
	}
	if (bitRate[0] == '\0') {
		strcpy(bitRate, "N/A");
	}

	wlSendPlainHeaders(wp);
	websWrite(wp, T("<table class=\"mainTableContent\" cellspacing=\"0\">"
		"<tr><td class=\"subMenuMainContent\" colspan=\"2\">"));
	wlWriteTrString(wp, "<!--#tr id=\"status.wn.1\" -->Wireless Network<!--#endtr-->");
	websWrite(wp, T("</td><td colspan=\"2\" class=\"blankContent\"></td></tr>"));

	if (MT_get_station_link_status() == 1 && linkUp) {
		wlWriteLinkStatusRow(wp, "<!--#tr id=\"status.wn.1-1\" -->Link Status:<!--#endtr-->", "<!--#tr id=\"w.bws.2-35\" -->Connected<!--#endtr-->");
		wlWriteLinkStatusRow(wp, "<!--#tr id=\"status.wn.1-21\" -->MAC Address:<!--#endtr-->", macStr);
		wlWriteLinkStatusRow(wp, "<!--#tr id=\"status.wn.1-3\" -->Network Name (SSID):<!--#endtr-->", escSsid);
		wlWriteLinkStatusRow(wp, "<!--#tr id=\"status.wn.1-4\" -->BSSID:<!--#endtr-->", bssid);
		wlWriteLinkStatusRow(wp, "<!--#tr id=\"status.wn.1-5\" -->Radio Band:<!--#endtr-->", radioBand);
		wlWriteLinkStatusRow(wp, "<!--#tr id=\"status.wn.1-6\" -->Channel Width:<!--#endtr-->", chanWidth);
		wlWriteLinkStatusRow(wp, "<!--#tr id=\"status.wn.1-7\" -->Wide Channel:<!--#endtr-->", wideChan);
		wlWriteLinkStatusRow(wp, "<!--#tr id=\"status.wn.1-8\" -->Standard Channel:<!--#endtr-->", stdChanStr);
		wlWriteLinkStatusRow(wp, "<!--#tr id=\"status.wn.1-9\" -->Bit Rate:<!--#endtr-->", bitRate);
		wlWriteLinkStatusRow(wp, "<!--#tr id=\"status.wn.1-10\" -->Signal:<!--#endtr-->", sigStr);
		wlWriteLinkStatusRow(wp, "<!--#tr id=\"status.wn.1-11\" -->Security:<!--#endtr-->", secLabel);
	} else {
		wlWriteLinkStatusRow(wp, "<!--#tr id=\"status.wn.1-1\" -->Link Status:<!--#endtr-->", "<!--#tr id=\"w.bws.2-32\" -->Disconnected<!--#endtr-->");
		wlWriteLinkStatusRow(wp, "<!--#tr id=\"status.wn.1-21\" -->MAC Address:<!--#endtr-->", macStr);
		wlWriteLinkStatusRow(wp, "<!--#tr id=\"status.wn.1-3\" -->Network Name (SSID):<!--#endtr-->", "");
		wlWriteLinkStatusRow(wp, "<!--#tr id=\"status.wn.1-4\" -->BSSID:<!--#endtr-->", "");
		wlWriteLinkStatusRow(wp, "<!--#tr id=\"status.wn.1-5\" -->Radio Band:<!--#endtr-->", "");
		wlWriteLinkStatusRow(wp, "<!--#tr id=\"status.wn.1-6\" -->Channel Width:<!--#endtr-->", "");
		wlWriteLinkStatusRow(wp, "<!--#tr id=\"status.wn.1-7\" -->Wide Channel:<!--#endtr-->", "");
		wlWriteLinkStatusRow(wp, "<!--#tr id=\"status.wn.1-8\" -->Standard Channel:<!--#endtr-->", "");
		wlWriteLinkStatusRow(wp, "<!--#tr id=\"status.wn.1-9\" -->Bit Rate:<!--#endtr-->", "");
		wlWriteLinkStatusRow(wp, "<!--#tr id=\"status.wn.1-10\" -->Signal:<!--#endtr-->", "");
		wlWriteLinkStatusRow(wp, "<!--#tr id=\"status.wn.1-11\" -->Security:<!--#endtr-->", "");
	}
	websWrite(wp, T("<tr><td class=\"subMenuSubContent\"></td><td class=\"subMenuLeftSide\"></td><td class=\"blankContent\" colspan=\"2\">&nbsp;</td></tr></table>"));
	websDone(wp, 200);
}

static int aspGetLinkStatus(int eid, webs_t wp, int argc, char_t **argv)
{
	char essid[128];
	char bssid[32];
	char bitRate[32];
	int channel = 0;
	int sig = 0;
	int linkUp = 0;

	(void)eid;
	(void)argc;
	(void)argv;

	wlReadIwconfigLink(essid, sizeof(essid), bssid, sizeof(bssid),
		&channel, bitRate, sizeof(bitRate), &sig, &linkUp);
	return websWrite(wp, T("%s,%d,%s,%s,%d"),
		linkUp ? "Connected" : "Disconnected", channel, bssid, bitRate, sig);
}

static int aspGetCurrectLanIP(int eid, webs_t wp, int argc, char_t **argv)
{
	char ipBuf[64];
	char maskBuf[64];

	(void)eid;
	(void)argc;
	(void)argv;

	memset(ipBuf, 0, sizeof(ipBuf));
	memset(maskBuf, 0, sizeof(maskBuf));
	if (hnap_getIfIp("br0", ipBuf) != 0 || ipBuf[0] == '\0') {
		puts("ERROR: Get LAN IP fail .. ");
		MT_Get_Param(T("ip_lan"), ipBuf, sizeof(ipBuf) - 1);
	}
	if (hnap_getIfNetmask("br0", maskBuf) != 0 || maskBuf[0] == '\0') {
		puts("ERROR: Get LAN Netmask fail .. ");
		MT_Get_Param(T("subnet_lan"), maskBuf, sizeof(maskBuf) - 1);
	}
	return websWrite(wp, T("%s,%s"), ipBuf, maskBuf);
}

static int aspGetWPSLinkStatus(int eid, webs_t wp, int argc, char_t **argv)
{
	(void)eid;
	(void)argc;
	(void)argv;
	return websWrite(wp, T("%s"), MT_get_station_link_status() == 1 ? "Connected" : "Disconnected");
}

static int aspGetRadioInfo(int eid, webs_t wp, int argc, char_t **argv)
{
	wlScanEntry_t *entries;
	int count;
	int i;
	char curEssid[128];
	char curBssid[32];
	char bandOut[32];

	(void)eid;
	(void)argc;
	(void)argv;

	memset(curEssid, 0, sizeof(curEssid));
	memset(curBssid, 0, sizeof(curBssid));
	MT_Get_Param(T("NonProc_ESSID"), curEssid, sizeof(curEssid) - 1);
	MT_Get_Param(T("BSSID"), curBssid, sizeof(curBssid) - 1);
	bandOut[0] = '\0';

	entries = (wlScanEntry_t *)calloc(WL_MAX_SCAN_ENTRIES, sizeof(wlScanEntry_t));
	if (entries != NULL) {
		count = wlParseScanList(entries, WL_MAX_SCAN_ENTRIES);
		for (i = 0; i < count; i++) {
			if (strcmp(entries[i].essid, curEssid) == 0 &&
			    (curBssid[0] == '\0' || strcmp(entries[i].bssid, curBssid) == 0)) {
				snprintf(bandOut, sizeof(bandOut), "%s GHz,20 MHz", entries[i].band);
				break;
			}
		}
		free(entries);
	}
	return websWrite(wp, T("%s"), bandOut);
}

/* Fallback stubs for handlers overridden in main.c (safeForm* / safeAsp*) */
static void wlStubRedirectRoot(webs_t wp, char_t *path, char_t *query)
{
	(void)path;
	(void)query;
	websRedirect(wp, T("/"));
}

static int wlStubAspEmpty(int eid, webs_t wp, int argc, char_t **argv)
{
	(void)eid;
	(void)wp;
	(void)argc;
	(void)argv;
	return 0;
}

void formDefineWireless(void)
{
	websFormDefine(T("wirelessBasic"), wlStubRedirectRoot);
	websFormDefine(T("networkBasic"), wlStubRedirectRoot);
	websFormDefine(T("wirelessSecurity"), formWirelessSecurity);
	websFormDefine(T("accessControl"), formAccessControl);
	websFormDefine(T("macFilter"), formMacFilter);
	websFormDefine(T("wirelessWPS"), formWirelessWPS);
	websFormDefine(T("updateSiteSurvey"), formUpdateSiteSurvey);
	websFormDefine(T("updateWPSStatus"), formUpdateWPSStatus);
	websFormDefine(T("updateLinkStatus"), formUpdateLinkStatus);
	websFormDefine(T("advancedWireless"), formAdvancedWireless);
	websFormDefine(T("stationScan"), formStationScan);
	websFormDefine(T("wirelessTypeSet"), wlStubRedirectRoot);
	websFormDefine(T("updateIPAddress"), formUpdateIPAddress);

	websAspDefine(T("getLinkStatus"), aspGetLinkStatus);
	websAspDefine(T("getWPSLinkStatus"), aspGetWPSLinkStatus);
	websAspDefine(T("getRadioInfo"), aspGetRadioInfo);
	websAspDefine(T("getWirelessChannel"), wlStubAspEmpty);
	websAspDefine(T("getCurrectLanIP"), aspGetCurrectLanIP);
	websAspDefine(T("waitToRedirect"), wlStubAspEmpty);
	websAspDefine(T("getBSSID"), wlStubAspEmpty);
}
