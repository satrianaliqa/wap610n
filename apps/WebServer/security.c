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



int getWLSBridgePort()
{
	FILE* tmpFile = NULL;
	char buff[256];

	int portNum = 1;
	int found = 0;

	system ("brctl show br0 > /tmp/brctl_show");
	tmpFile = fopen ("/tmp/brctl_show","rt");

	if (tmpFile)
	{
		// get all lines from file
		while (!feof(tmpFile) && !ferror(tmpFile))
		{
			fgets(buff,255,tmpFile);

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
	if (found == 1)
		return portNum;
	else
		return -1;
}

int GetBridgPortForMAC(const char* mac)
{
	FILE* tmpFile = NULL;
	char buff[256];

	char port [20];
	int portNum = -1;
	char macaddr[30];

	system ("brctl showmacs br0 >/tmp/brctl_showmacs");
	tmpFile = fopen ("/tmp/brctl_showmacs","rt");

	if (tmpFile)
	{
		// get all lines from file
		while (!feof(tmpFile) && !ferror(tmpFile))
		{
			fgets(buff,255,tmpFile);

			if (strstr(buff,"port no mac addr")!= NULL) // skip headers
				continue;

			sscanf(buff,"%s %s",port,macaddr);
			if (macaddr != "" && strcmp(macaddr,mac) == 0)
			{
				portNum = atoi(port);
				break;
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

	char ipaddr [30];
	char tmp1[30];
	char tmp2[30];
	char macaddr[30];

	tmpFile = fopen ("/proc/net/arp","rt");

	if (tmpFile)
	{
		// get all lines from file
		while (!feof(tmpFile) && !ferror(tmpFile))
		{
			fgets(buff,255,tmpFile);

			if (strstr(buff,"IP address")!= NULL) // skip headers
				continue;

			sscanf(buff,"%s %s %s %s",ipaddr,tmp1,tmp2,macaddr);

			if (ipaddr != "" && strcmp(ipaddr,ip) == 0)
			{
				strncpy(mac,macaddr,mac_len-1); // In case we have multiple entries, we take the last which is the most updated.
			}
		}
		fclose(tmpFile);
	}

	return 0;
}


int isIPFromWLS(const char* ip)
{
	char mac[256] = "";
	int i=0;
	int port = 0;

	IPtoMAC(ip,mac,255);

	// Need to convert mac to lower case
	for (i=0;i<strlen(mac);i++)
	{
		mac[i] = tolower(mac[i]);
	}

	port = GetBridgPortForMAC(mac);

	if (port == getWLSBridgePort())
	{
		printf ("ip %s (%s) is from WLS\n",ip,mac);
		return 1;
	}
	else
	{
		printf ("ip %s (%s) is from eth0\n",ip,mac);
		return 0;
	}
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
	char_t			MTuserName[MT_MAX_PARAM_VALUE_LENGTH] = "";
	char_t			MTuserPass[MT_MAX_PARAM_VALUE_LENGTH] = "";
	char_t			MTAuthTimeout[MT_MAX_PARAM_VALUE_LENGTH] = "";
	char_t			MTPasswordOpen[MT_MAX_PARAM_VALUE_LENGTH] = "";
	char_t			wirelessMgmtDisabled[10];
	static time_t   lastAuth = 0;
	time_t			now = 0;
	int				AuthTimeout = 0;
	int				passwordOpen = 0;

	a_assert(websValid(wp));
	a_assert(url && *url);
	a_assert(path && *path);

	/*

		Password is saved encrypted in the sys.conf file.
		In order to change it manually, follow these steps :
		1. Set the AdminPassword to an unencrypted value
		2. Set AdminPasswordOpen = 1 to indicate that the password is not encrypted
		3. load the main page, to allow the webserver to encrypt and save the password.

		The password is now saved as encrypted.
	*/

	MT_Get_Param(MTSECURITY_USER_NAME_VAR_NAME,MTuserName,MT_MAX_PARAM_VALUE_LENGTH-1);
	MT_Get_Param(MTSECURITY_USER_PASSWD_VAR_NAME,MTuserPass,MT_MAX_PARAM_VALUE_LENGTH-1);
	MT_Get_Param(MTSECURITY_INACTIVE_TIMEOUT, MTAuthTimeout,MT_MAX_PARAM_VALUE_LENGTH-1);
	MT_Get_Param(MTSECURITY_ADMINPASS_OPEN, MTPasswordOpen,MT_MAX_PARAM_VALUE_LENGTH-1);

/*
 *	If no user and password are defined, let the user see the page.
 */
	if (gstrlen(MTuserName) == 0 || gstrlen(MTuserPass) == 0)
		return 0;

/*
 *	Get more authentication parameters from the configuration file.
 */	

	AuthTimeout = atoi(MTAuthTimeout);
/*
	passwordOpen = atoi(MTPasswordOpen);


 //	If the password is not encrypted, encrypt it and rewrite it to the configuration file.
 
	if (passwordOpen == 1)
	{
		char_t *argp[] ={{MTSECURITY_USER_PASSWD_VAR_NAME},{MTSECURITY_ADMINPASS_OPEN}};
		int n = 0;

		// Encrypt the password
		umEncryptString(MTuserPass);

		// Set the new parameters
		MT_Set_Param(MTSECURITY_USER_PASSWD_VAR_NAME,MTuserPass,MT_SET_FIRST_VALUE);
		MT_Set_Param(MTSECURITY_ADMINPASS_OPEN,"",MT_SET_FIRST_VALUE);
		
		// Save the changed parameters
		MT_WriteConfFiles(2, argp);

	}

	// decrypt the password
	umEncryptString(MTuserPass);
*/
	
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
		if (now>lastAuth+AuthTimeout)
		{
			//Jacky.Yang3-Dec-2008, fix timeout didn't pop login window again, if password is blank.
			userid = "";
			password = "";
			//password = NULL;
		}
		lastAuth = now;
	}


	if (password == NULL)
	{
		MT_Get_Param("WirelessMgmtEnabled",wirelessMgmtDisabled,10);

		if (strstr("0",wirelessMgmtDisabled) != NULL && isIPFromWLS(wp->ipaddr))
		{
			websError(wp, 403, T("Access Denied\nWireless management is fordidden"));
			return 1;
		}
	}
	//june.chen, 2011-02-17, add for solving issue that after disabling wireless management, without closimg window already, user could still set DUT through opened window.
//This is extremely ugly, but I don't want to change the original code structure.
	else{
		MT_Get_Param("WirelessMgmtEnabled",wirelessMgmtDisabled,10);

		if (strstr("0",wirelessMgmtDisabled) != NULL && isIPFromWLS(wp->ipaddr))
		{
			websError(wp, 403, T("Access Denied\nWireless management is fordidden"));
			return 1;
		}

	}
//june.chen end


/*
 *	Get the access limit for the URL.  Exit if none found. By default, all pages require user name and password.
 */
	accessLimit = url; //umGetAccessLimit(path);

#ifdef MT_USE_STAT_TASK
	if (strstr(accessLimit, "/mt_stat")) return 0; // Check that mt_stat_page is in the URL
#endif

	//Add for HNAP support - U-Media Ricky Cao
	//Because HNAP will check usernam and password, so I skip to check them at here
	if (strstr(accessLimit,"/HNAP1")){ // <--- check that HNAP1 is in the URL
		return 0; // <--- return 0, which means it's ok to move to the next handler in list.
	}
	//U-Media Ricky Cao
	
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
      /* bugfix 5/24/02 -- we were leaking the memory pointed to by
       * 'accessLimit'. Thanks to Simon Byholm.
       */
      bfree(B_L, accessLimit);
		return 1;
	}
#endif

/*
 *	Get the access limit for the URL
 */
	am = AM_BASIC;//umGetAccessMethodForURL(accessLimit);

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
	//Jacky.Yang 24-Jul-2008, don't chekc user name
	//} else 	if (userid && *userid) {
	} else 	if (1) {
		
		/*if (gstrcmp(userid,MTuserName) != 0) {
			websStats.access++;
			websError(wp, 401, T("Access Denied\nUnknown User"));
			nRet = 1;
		} else*/ if (password && * password) {
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
				}

				bfree (B_L, userpass);
			}
#ifdef DIGEST_ACCESS_SUPPORT
		} else if (flags & WEBS_AUTH_DIGEST) {

			char_t *digestCalc;

/*
 *			Check digest for equivalence
 */
			
			wp->password = bstrdup(B_L, MTuserPass);

			a_assert(wp->digest);
			a_assert(wp->nonce);
			a_assert(wp->password);
							 
			digestCalc = websCalcDigest(wp);
			a_assert(digestCalc);

			if (gstrcmp(wp->digest, digestCalc) != 0) {
				websStats.access++;
            /* 16 Jun 03 -- error code changed from 405 to 401 -- thanks to
             * Jay Chalfant.
             */
				websError(wp, 401, T("Access Denied\nWrong Password"));
				nRet = 1;
			}

			bfree (B_L, digestCalc);
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
	} else if (am != AM_FULL) {
/*
 *		This will cause the browser to display a password / username
 *		dialog
 */
#ifdef DIGEST_ACCESS_SUPPORT
		if (am == AM_DIGEST) {
			wp->flags |= WEBS_AUTH_DIGEST;
		}
#endif
		websStats.errors++;
		websError(wp, 401, T("Access to this document requires a User ID"));
		nRet = 1;
	}

	//bfree(B_L, accessLimit);

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


