/*
 * security.c -- Security handler
 *
 * Copyright (c) GoAhead Software Inc., 1995-2000. All Rights Reserved.
 *
 * See the file "license.txt" for usage and redistribution license requirements
 *
 * $Id: security.c,v 1.9 2003/09/19 17:04:44 bporter Exp $
 */

/******************************** Description *********************************/

/*
 *	This module provides a basic security policy.
 */

/********************************* Includes ***********************************/

#include	"wsIntrn.h"
#include	"um.h"

#include "mt_api.h"

#ifdef DIGEST_ACCESS_SUPPORT
#include	"websda.h"
#endif

/********************************** Defines ***********************************/
/*
 *	The following #defines change the behaviour of security in the absence 
 *	of User Management.
 *	Note that use of User management functions require prior calling of
 *	umInit() to behave correctly
 */

#ifndef USER_MANAGEMENT_SUPPORT
#define umGetAccessMethodForURL(url) AM_FULL
#define umUserExists(userid) 0
#define umUserCanAccessURL(userid, url) 1
#define umGetUserPassword(userid) websGetPassword()
#define umGetAccessLimitSecure(accessLimit) 0
#define umGetAccessLimit(url) NULL
#endif

/******************************** Local Data **********************************/

static char_t	websPassword[WEBS_MAX_PASS];	/* Access password (decoded) */
#ifdef _DEBUG
static int		debugSecurity = 1;
#else
static int		debugSecurity = 0;
#endif



static int cached_wls_port = -1;

int getWLSBridgePort()
{
	FILE* tmpFile = NULL;
	char buff[256];

	int portNum = 1;
	int found = 0;

	if (cached_wls_port != -1)
		return cached_wls_port;

	system ("brctl show br0 > /tmp/brctl_show");
	tmpFile = fopen ("/tmp/brctl_show","rt");

	if (tmpFile)
	{
		// get all lines from file
		while (fgets(buff, sizeof(buff), tmpFile))
		{
			if (strstr(buff,"bridge name")!= NULL) // skip headers
				continue;

			if (strstr(buff,"wlan0")!= NULL)
			{
				found = 1;
				break;
			}

			portNum++;
		}
		fclose(tmpFile);
	}
	if (found == 1) {
		cached_wls_port = portNum;
		return portNum;
	} else
		return -1;
}

int GetBridgPortForMAC(const char* mac)
{
	static time_t last_brctl_time = 0;
	time_t now;
	FILE* tmpFile = NULL;
	char buff[256];

	char port[20];
	int portNum = -1;
	char macaddr[30];

	memset(port, 0, sizeof(port));
	memset(macaddr, 0, sizeof(macaddr));

	time(&now);
	if (now - last_brctl_time > 5 || last_brctl_time == 0) {
		system ("brctl showmacs br0 >/tmp/brctl_showmacs");
		last_brctl_time = now;
	}
	tmpFile = fopen ("/tmp/brctl_showmacs","rt");

	if (tmpFile)
	{
		// get all lines from file
		while (fgets(buff, sizeof(buff), tmpFile))
		{
			if (strstr(buff,"port no mac addr")!= NULL) // skip headers
				continue;

			if (sscanf(buff, "%19s %29s", port, macaddr) == 2) {
				if (macaddr[0] != '\0' && strcmp(macaddr, mac) == 0)
				{
					portNum = atoi(port);
					break;
				}
			}
		}
		fclose(tmpFile);
	}
	return portNum;

}

int IPtoMAC(const char* ip, char* mac, unsigned int mac_len)
{
	FILE* tmpFile = NULL;
	char buff[256];

	char ipaddr[30];
	char tmp1[30];
	char tmp2[30];
	char macaddr[30];

	memset(ipaddr, 0, sizeof(ipaddr));
	memset(tmp1, 0, sizeof(tmp1));
	memset(tmp2, 0, sizeof(tmp2));
	memset(macaddr, 0, sizeof(macaddr));

	tmpFile = fopen ("/proc/net/arp","rt");

	if (tmpFile)
	{
		// get all lines from file
		while (fgets(buff, sizeof(buff), tmpFile))
		{
			if (strstr(buff,"IP address")!= NULL) // skip headers
				continue;

			if (sscanf(buff, "%29s %29s %29s %29s", ipaddr, tmp1, tmp2, macaddr) == 4) {
				if (ipaddr[0] != '\0' && strcmp(ipaddr, ip) == 0)
				{
					strncpy(mac, macaddr, mac_len - 1); // In case we have multiple entries, we take the last which is the most updated.
					mac[mac_len - 1] = '\0';
				}
			}
		}
		fclose(tmpFile);
	}

	return 0;
}

int isIPFromWLS(const char* ip)
{
	char mac[256];
	unsigned int i = 0;
	int port = -1;
	int wlsPort = -1;

	if (ip == NULL || ip[0] == '\0') {
		return 0;
	}
	memset(mac, 0, sizeof(mac));
	IPtoMAC(ip, mac, sizeof(mac));

	/* Need to convert mac to lower case */
	for (i = 0; mac[i] != '\0'; i++) {
		mac[i] = (char)tolower((unsigned char)mac[i]);
	}

	if (mac[0] == '\0') {
		return 0;
	}

	port = GetBridgPortForMAC(mac);
	wlsPort = getWLSBridgePort();

	if (port > 0 && wlsPort > 0 && port == wlsPort) {
		printf("ip %s (%s) is from WLS\n", ip, mac);
		return 1;
	}
	printf("ip %s (%s) is from eth0\n", ip, mac);
	return 0;
}

/*********************************** Code *************************************/
/*
 *	Determine if this request should be honored
 */

int websSecurityHandler(webs_t wp, char_t *urlPrefix, char_t *webDir, int arg, 
						char_t *url, char_t *path, char_t *query)
{
	char_t			*type, *userid, *password, *accessLimit;
	int				flags, nRet;
	accessMeth_t	am;
	char_t			MTuserName[MT_MAX_PARAM_VALUE_LENGTH];
	char_t			MTuserPass[MT_MAX_PARAM_VALUE_LENGTH];
	char_t			MTAuthTimeout[MT_MAX_PARAM_VALUE_LENGTH];
	char_t			MTPasswordOpen[MT_MAX_PARAM_VALUE_LENGTH];
	char_t			wirelessMgmtDisabled[16];
	static time_t   lastAuth = 0;
	time_t			now = 0;
	int				AuthTimeout = 0;

	a_assert(websValid(wp));
	a_assert(url && *url);
	a_assert(path && *path);

	memset(MTuserName, 0, sizeof(MTuserName));
	memset(MTuserPass, 0, sizeof(MTuserPass));
	memset(MTAuthTimeout, 0, sizeof(MTAuthTimeout));
	memset(MTPasswordOpen, 0, sizeof(MTPasswordOpen));
	memset(wirelessMgmtDisabled, 0, sizeof(wirelessMgmtDisabled));

	/* Block path traversal attempts BEFORE credential checks */
	if ((path != NULL && gstrstr(path, T("..")) != NULL) ||
	    (url != NULL && gstrstr(url, T("..")) != NULL))
	{
		websError(wp, 403, T("Access Denied"));
		return 1;
	}

	/* Enforce wireless management block regardless of credential state */
	MT_Get_Param("WirelessMgmtEnabled", wirelessMgmtDisabled, sizeof(wirelessMgmtDisabled) - 1);
	wirelessMgmtDisabled[sizeof(wirelessMgmtDisabled) - 1] = '\0';
	if (strcmp(wirelessMgmtDisabled, "0") == 0 && isIPFromWLS(wp->ipaddr))
	{
		websError(wp, 403, T("Access Denied\nWireless management is forbidden"));
		return 1;
	}

	MT_Get_Param(MTSECURITY_USER_NAME_VAR_NAME, MTuserName, sizeof(MTuserName) - 1);
	MTuserName[sizeof(MTuserName) - 1] = '\0';
	MT_Get_Param(MTSECURITY_USER_PASSWD_VAR_NAME, MTuserPass, sizeof(MTuserPass) - 1);
	MTuserPass[sizeof(MTuserPass) - 1] = '\0';
	MT_Get_Param(MTSECURITY_INACTIVE_TIMEOUT, MTAuthTimeout, sizeof(MTAuthTimeout) - 1);
	MTAuthTimeout[sizeof(MTAuthTimeout) - 1] = '\0';
	MT_Get_Param(MTSECURITY_ADMINPASS_OPEN, MTPasswordOpen, sizeof(MTPasswordOpen) - 1);
	MTPasswordOpen[sizeof(MTPasswordOpen) - 1] = '\0';

	/* Only bypass auth when no password is configured */
	if (gstrlen(MTuserPass) == 0)
		return 0;

	AuthTimeout = atoi(MTAuthTimeout);
	
/*
 *	Get the critical request details
 */
	type = websGetRequestType(wp);
	password = websGetRequestPassword(wp);
	userid = websGetRequestUserName(wp);
	flags = websGetRequestFlags(wp);

	/*
		Check Authentication Timeout. If timed out from last activity, re-ask for authentication information.
	*/
	if (AuthTimeout > 0)
	{
		time(&now);
		if (lastAuth != 0 && now > lastAuth + AuthTimeout)
		{
			userid = "";
			password = "";
		}
	}

/*
 *	Get the access limit for the URL.  Exit if none found. By default, all pages require user name and password.
 */
	accessLimit = url; /* umGetAccessLimit(path); */

	/* Add for HNAP support - U-Media Ricky Cao */
	/* Only skip GoAhead Basic Auth when the URL path itself is /HNAP1 (HNAP handler enforces its own Basic Auth) */
	if (path != NULL && (gstrcmp(path, T("/HNAP1")) == 0 || gstrncmp(path, T("/HNAP1/"), 7) == 0)){
		return 0;
	}
	
	if (accessLimit == NULL) {
		return 0;
	}
		 
/*
 *	Check to see if URL must be encrypted
 */
#ifdef WEBS_SSL_SUPPORT
	nRet = umGetAccessLimitSecure(accessLimit);
	if (nRet && ((flags & WEBS_SECURE) == 0)) {
		websStats.access++;
		websError(wp, 405, T("Access Denied\nSecure access is required."));
		trace(3, T("SEC: Non-secure access attempted on <%s>\n"), path);
		bfree(B_L, accessLimit);
		return 1;
	}
#endif

/*
 *	Get the access limit for the URL
 */
	am = AM_BASIC;

	nRet = 0;
	if ((flags & WEBS_LOCAL_REQUEST) && (debugSecurity == 0)) {
/*
 *		Local access is always allowed (defeat when debugging)
 */
	} else if (am == AM_NONE) {
/*
 *		URL is supposed to be hidden!  Make like it wasn't found.
 */
		websStats.access++;
		websError(wp, 404, T("Page Not Found"));
		nRet = 1;
	} else {
		if (password && *password) {
			char_t * userpass = MTuserPass;
			if (userpass) {
				if (gstrcmp(password, userpass) != 0) {
					websStats.access++;
					websError(wp, 401, T("Access Denied\nWrong Password"));
					nRet = 1;
				} else {
/*
 *					User and password check out.
 */
					if (AuthTimeout > 0) {
						lastAuth = now;
					}
				}
			}
#ifdef DIGEST_ACCESS_SUPPORT
		} else if ((flags & WEBS_AUTH_DIGEST) && (am == AM_DIGEST)) {
			char_t *digestCalc = NULL;

			if (wp->digest == NULL || wp->nonce == NULL || wp->uri == NULL ||
				wp->realm == NULL || wp->userName == NULL ||
				gstrcmp(wp->uri, wp->path) != 0) {
				websStats.access++;
				websError(wp, 401, T("Access Denied\nInvalid Digest Auth"));
				nRet = 1;
			} else if (AuthTimeout > 0 && lastAuth != 0 && now > lastAuth + AuthTimeout) {
				websStats.access++;
				websError(wp, 401, T("Access Denied\nSession Timeout"));
				nRet = 1;
			} else {
				bfreeSafe(B_L, wp->password);
				wp->password = bstrdup(B_L, MTuserPass);
				if (wp->password != NULL) {
					digestCalc = websCalcDigest(wp);
				}
				if (digestCalc == NULL || gstrcmp(wp->digest, digestCalc) != 0) {
					websStats.access++;
					websError(wp, 401, T("Access Denied\nWrong Password"));
					nRet = 1;
				} else if (AuthTimeout > 0) {
					lastAuth = now;
				}
				bfreeSafe(B_L, digestCalc);
			}
#endif
		} else {
/*
 *			No password has been specified
 */
#ifdef DIGEST_ACCESS_SUPPORT
			if (am == AM_DIGEST) {
				wp->flags |= WEBS_AUTH_DIGEST;
			}
#endif
			websStats.errors++;
			websError(wp, 401, 
				T("Access to this document requires a password"));
			nRet = 1;
		}
	}

	return nRet;
}

/******************************************************************************/
/*
 *	Delete the default security handler
 */

void websSecurityDelete()
{
	websUrlHandlerDelete(websSecurityHandler);
}

/******************************************************************************/
/*
 *	Store the new password, expect a decoded password. Store in websPassword in 
 *	the decoded form.
 */

void websSetPassword(char_t *password)
{
	a_assert(password);

	gstrncpy(websPassword, password, TSZ(websPassword));
}

/******************************************************************************/
/*
 *	Get password, return the decoded form
 */

char_t *websGetPassword()
{
	return bstrdup(B_L, websPassword);
}

/******************************************************************************/


