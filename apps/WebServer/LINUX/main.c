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
static int			port = 80;
static int			retries = 5;					/* Server port retries */
static int			finished;						/* Finished flag */

/****************************** Forward Declarations **************************/

static int 	initWebs();
static int  websHomePageHandler(webs_t wp, char_t *urlPrefix, char_t *webDir,
				int arg, char_t *url, char_t *path, char_t *query);
extern void defaultErrorHandler(int etype, char_t *msg);
extern void defaultTraceHandler(int level, char_t *buf);
extern void upldForm(webs_t wp, char_t *path, char_t *query);
extern void formDumpFirmware(webs_t wp, char_t *path, char_t *query);

static void websDiag(const char *message)
{
	int fd;

	fd = open("/dev/kmsg", O_WRONLY);
	if (fd >= 0) {
		write(fd, "<4>wap610n-webs: ",
			sizeof("<4>wap610n-webs: ") - 1);
		write(fd, message, strlen(message));
		write(fd, "\n", 1);
		close(fd);
	}
	fd = open("/dev/console", O_WRONLY);
	if (fd >= 0) {
		write(fd, "[wap610n-webs] ", sizeof("[wap610n-webs] ") - 1);
		write(fd, message, strlen(message));
		write(fd, "\n", 1);
		close(fd);
	}
	printf("wap610n-webs: %s\n", message);
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

/*********************************** Code *************************************/

/*
 *	Main -- entry point from LINUX
 */

int main(int argc, char** argv)
{
	struct sigaction setup_action;
	sigset_t block_mask;
	char cmd[256];
	int firewallReloadID;
	struct msgbuffer {
		long mtype;
		char mtext[1024];
	} msgInfo;

	//Jacky.Yang 15-May-209, set open debug message value;
	//openDebug=1;
	
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
		if (socketReady(-1) || socketSelect(-1, 1000)) {
			socketProcess(-1);
		}
		websCgiCleanup();
		emfSchedProcess();
		
		/* Check IPC message queue for debug toggle */
		if (firewallReloadID != -1) {
			int mlen = msgrcv(firewallReloadID, &msgInfo, (size_t)(sizeof(msgInfo.mtext) - 1), 0, IPC_NOWAIT);
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
	struct hostent	*hp;
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
