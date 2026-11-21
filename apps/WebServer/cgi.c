/*
 * cgi.c -- CGI processing (for the GoAhead Web server
 *
 * Copyright (c) GoAhead Software Inc., 1995-2000. All Rights Reserved.
 *
 * See the file "license.txt" for usage and redistribution license requirements
 *
 * $Id: cgi.c,v 1.3 2002/10/24 14:44:50 bporter Exp $
 */

/********************************** Description *******************************/
/*
 *	This module implements the /cgi-bin handler. CGI processing differs from
 *	goforms processing in that each CGI request is executed as a separate 
 *	process, rather than within the webserver process. For each CGI request the
 *	environment of the new process must be set to include all the CGI variables
 *	and its standard input and output must be directed to the socket.  This
 *	is done using temporary files.
 */

/*********************************** Includes *********************************/
#include	"wsIntrn.h"
#include	"mt_api.h"
#ifdef UEMF
	#include	"uemf.h"
#else
	#include	"basic/basicInternal.h"
#endif

/************************************ Locals **********************************/
typedef struct {				/* Struct for CGI tasks which have completed */
	webs_t	wp;					/* pointer to session websRec */
	char_t	*stdIn;				/* file desc. for task's temp input fd */
	char_t	*stdOut;			/* file desc. for task's temp output fd */
	char_t	*cgiPath;			/* path to executable process file */
	char_t	**argp;				/* pointer to buf containing argv tokens */
	char_t	**envp;				/* pointer to array of environment strings */
	int		handle;				/* process handle of the task */
	long	fplacemark;			/* seek location for CGI output file */
	char_t  *cgiWebVarFile;
} cgiRec;
static cgiRec	**cgiList;		/* hAlloc chain list of wp's to be closed */
static int		cgiMax;			/* Size of hAlloc list */

/************************************* Code ***********************************/

static int cgiIsWhitelistedEnv(const char_t *name)
{
	if (name == NULL || *name == '\0') {
		return 0;
	}
	if (gstrncmp(name, T("HTTP_"), 5) == 0) {
		return (gstrcmp(name, T("HTTP_AUTHORIZATION")) != 0);
	}
	if (gstrcmp(name, T("REQUEST_METHOD")) == 0 ||
		gstrcmp(name, T("QUERY_STRING")) == 0 ||
		gstrcmp(name, T("CONTENT_TYPE")) == 0 ||
		gstrcmp(name, T("CONTENT_LENGTH")) == 0 ||
		gstrcmp(name, T("GATEWAY_INTERFACE")) == 0 ||
		gstrcmp(name, T("SERVER_HOST")) == 0 ||
		gstrcmp(name, T("SERVER_NAME")) == 0 ||
		gstrcmp(name, T("SERVER_URL")) == 0 ||
		gstrcmp(name, T("SERVER_PORT")) == 0 ||
		gstrcmp(name, T("SERVER_ADDR")) == 0 ||
		gstrcmp(name, T("SERVER_SOFTWARE")) == 0 ||
		gstrcmp(name, T("SERVER_PROTOCOL")) == 0 ||
		gstrcmp(name, T("SERVER_TYPE")) == 0 ||
		gstrcmp(name, T("REMOTE_ADDR")) == 0 ||
		gstrcmp(name, T("PATH_INFO")) == 0) {
		return 1;
	}
	return 0;
}

/*
 *	Process a form request. Returns 1 always to indicate it handled the URL
 */
int websCgiHandler(webs_t wp, char_t *urlPrefix, char_t *webDir, int arg, 
		char_t *url, char_t *path, char_t* query)
{
	cgiRec		*cgip;
	sym_t		*s;
	char_t		cgiBuf[FNAMESIZE], *stdIn, *stdOut, cwd[FNAMESIZE], cgiWebVars[FNAMESIZE];
	char_t		*cp, *cgiName, *cgiPath, **argp, **envp, **ep, **newPtr;
	int			n, envpsize, argpsize, pHandle, cid;
	int			tempFd = -1;
	a_assert(websValid(wp));
	a_assert(url && *url);
	a_assert(path && *path == '/');
	websStats.cgiHits++;
	cgiWebVars[0] = '\0';
/*
 *	Extract the form name and then build the full path name.  The form
 *	name will follow the first '/' in path.
 */
	gstrncpy(cgiBuf, path, TSZ(cgiBuf) - 1);
	cgiBuf[TSZ(cgiBuf) - 1] = '\0';
	if ((cgiName = gstrchr(&cgiBuf[1], '/')) == NULL) {
		websError(wp, 200, T("Missing CGI name"));
		return 1;
	}
	cgiName++;
	if ((cp = gstrchr(cgiName, '/')) != NULL) {
		*cp = '\0';
	}
	if (gstrstr(cgiName, "..") != NULL) {
		websError(wp, 200, T("Access Denied"));
		return 1;
	}
	/* [BUG-12] Restrict CGI path: only execute scripts from cgi-bin directory */
	fmtAlloc(&cgiPath, FNAMESIZE, T("%s/cgi-bin/%s"), websGetDefaultDir(), cgiName);
	if (cgiPath == NULL) {
		websError(wp, 503, T("Insufficient memory"));
		return 1;
	}
#ifndef VXWORKS
/*
 *	See if the file exists and is executable.  If not error out.
 *	Don't do this step for VxWorks, since the module may already
 *	be part of the OS image, rather than in the file system.
 */
	{
		gstat_t		sbuf;
		if (gstat(cgiPath, &sbuf) != 0 || (sbuf.st_mode & S_IFREG) == 0) {
			websError(wp, 404, T("CGI process file does not exist"));
			bfree(B_L, cgiPath);
			return 1;
		}
#if (defined (WIN) || defined (CE))
		if (gstrstr(cgiPath, T(".exe")) == NULL &&
			gstrstr(cgiPath, T(".bat")) == NULL &&
			gstrstr(cgiPath, T(".tcl")) == NULL &&
			gstrstr(cgiPath, T(".pl")) == NULL) {
#elif (defined (NW))
			if (gstrstr(cgiPath, T(".nlm")) == NULL) {
#else
		if (gaccess(cgiPath, X_OK) != 0) {
#endif /* WIN || CE */
			websError(wp, 403, T("CGI process file is not executable"));
			bfree(B_L, cgiPath);
			return 1;
		}
	}
#endif /* ! VXWORKS */


	/*
	 * Generate a temporary output file name for the CGI process.
	 */
	snprintf(cgiWebVars, sizeof(cgiWebVars), "%s/cgivarsXXXXXX", MT_WebTmpDir);
	cgiWebVars[sizeof(cgiWebVars) - 1] = '\0';
	tempFd = mkstemp(cgiWebVars);
	if (tempFd >= 0) {
		close(tempFd);
	} else {
		cgiWebVars[0] = '\0';
	}

/*
 *	Get the CWD for resetting after launching the child process CGI
 */
	if (ggetcwd(cwd, FNAMESIZE) == NULL) {
		gstrncpy(cwd, websGetDefaultDir(), FNAMESIZE - 1);
		cwd[FNAMESIZE - 1] = '\0';
	}
/*
 *	Retrieve the directory of the child process CGI
 */
	if ((cp = gstrrchr(cgiPath, '/')) != NULL) {
		*cp = '\0';
		gchdir(cgiPath);
		*cp = '/';
	}
/*
 *	Build command line arguments.  Only used if there is no non-encoded
 *	= character.
 */
	argpsize = 10;
	argp = balloc(B_L, argpsize * sizeof(char_t *));
	if (argp == NULL) {
		if (cgiWebVars[0] != '\0') {
			gunlink(cgiWebVars);
		}
		bfreeSafe(B_L, cgiPath);
		gchdir(cwd);
		websError(wp, 503, T("Insufficient memory"));
		return 1;
	}
	*argp = cgiPath;
	n = 1;
	if (query && gstrchr(query, '=') == NULL) {
		websDecodeUrl(query, query, gstrlen(query));
		for (cp = gstrtok(query, T(" ")); cp != NULL; ) {
			if (n + 1 >= argpsize) {
				argpsize *= 2;
				newPtr = brealloc(B_L, argp, argpsize * sizeof(char_t *));
				if (newPtr == NULL) {
					break;
				}
				argp = newPtr;
			}
			*(argp + n) = cp;
			n++;
			cp = gstrtok(NULL, T(" "));
		}
	}
	*(argp + n) = NULL;
/*
 *	Add whitelisted standard CGI environment strings (do not export raw query params).
 */
	envpsize = 64;
	envp = balloc(B_L, envpsize * sizeof(char_t *));
	if (envp == NULL) {
		bfreeSafe(B_L, argp);
		if (cgiWebVars[0] != '\0') {
			gunlink(cgiWebVars);
		}
		bfreeSafe(B_L, cgiPath);
		gchdir(cwd);
		websError(wp, 503, T("Insufficient memory"));
		return 1;
	}
	n = 0;
	fmtAlloc(envp + n, FNAMESIZE, T("%s=%s"), T("PATH"), T("/sbin:/bin:/usr/sbin:/usr/bin"));
	n++;
	fmtAlloc(envp + n, FNAMESIZE, T("%s=%s"), T("PATH_TRANSLATED"), cgiPath);
	n++;
	fmtAlloc(envp + n, FNAMESIZE, T("%s=%s"), T("SCRIPT_NAME"), cgiName);
	n++;
	fmtAlloc(envp + n, FNAMESIZE, T("%s=%s"), T("CGI_WEB_VAR_FILE"), cgiWebVars);
	n++;
	fmtAlloc(envp + n, FNAMESIZE, T("%s=%s"), T("REMOTE_USER"),
		wp->userName ? wp->userName : T(""));
	n++;
	fmtAlloc(envp + n, FNAMESIZE, T("%s=%s"), T("AUTH_TYPE"),
		wp->authType ? wp->authType : T(""));
	n++;

	for (s = symFirst(wp->cgiVars); s != NULL; s = symNext(wp->cgiVars)) {
		if (s->content.valid && s->content.type == string &&
			cgiIsWhitelistedEnv(s->name.value.string)) {
			if (n + 1 >= envpsize) {
				envpsize *= 2;
				newPtr = brealloc(B_L, envp, envpsize * sizeof(char_t *));
				if (newPtr == NULL) {
					break;
				}
				envp = newPtr;
			}
			fmtAlloc(envp + n, FNAMESIZE, T("%s=%s"), s->name.value.string,
				s->content.value.string);
			if (envp[n] != NULL) {
				n++;
			}
		}
	}
	*(envp + n) = NULL;
/*
 *	Create temporary file name(s) for the child's stdin and stdout.
 *	For POST data the stdin temp file (and name) should already exist.
 */
	if (wp->cgiStdin == NULL) {
		wp->cgiStdin = websGetCgiCommName();
	}
	stdIn = bstrdupA(B_L, wp->cgiStdin);
	stdOut = websGetCgiCommName();
	if (stdIn == NULL || stdOut == NULL) {
		for (ep = envp; *ep != NULL; ep++) {
			bfreeSafe(B_L, *ep);
		}
		bfreeSafe(B_L, cgiPath);
		bfreeSafe(B_L, argp);
		bfreeSafe(B_L, envp);
		bfreeSafe(B_L, stdIn);
		bfreeSafe(B_L, stdOut);
		if (cgiWebVars[0] != '\0') {
			gunlink(cgiWebVars);
		}
		gchdir(cwd);
		websError(wp, 503, T("Insufficient memory"));
		return 1;
	}
/*
 *	Transfer ownership of wp->cgiStdin to cgip->stdIn so websFree() will not
 *	prematurely unlink stdin if wp terminates early.
 */
	bfreeSafe(B_L, wp->cgiStdin);
	wp->cgiStdin = NULL;
/*
 *	Now launch the process.
 */
	if ((pHandle = websLaunchCgiProc(cgiPath, argp, envp, stdIn, stdOut)) == -1) {
		websError(wp, 500, T("failed to spawn CGI task"));
		for (ep = envp; *ep != NULL; ep++) {
			bfreeSafe(B_L, *ep);
		}
		bfreeSafe(B_L, cgiPath);
		bfreeSafe(B_L, argp);
		bfreeSafe(B_L, envp);
		gunlink(stdIn);
		gunlink(stdOut);
		bfreeSafe(B_L, stdIn);
		bfreeSafe(B_L, stdOut);
		if (cgiWebVars[0] != '\0') {
			gunlink(cgiWebVars);
		}
	} else {
		cid = hAllocEntry((void***) &cgiList, &cgiMax, sizeof(cgiRec));
		if (cid < 0) {
			websError(wp, 503, T("Insufficient memory"));
			for (ep = envp; *ep != NULL; ep++) {
				bfreeSafe(B_L, *ep);
			}
			bfreeSafe(B_L, cgiPath);
			bfreeSafe(B_L, argp);
			bfreeSafe(B_L, envp);
			gunlink(stdIn);
			gunlink(stdOut);
			bfreeSafe(B_L, stdIn);
			bfreeSafe(B_L, stdOut);
			if (cgiWebVars[0] != '\0') {
				gunlink(cgiWebVars);
			}
		} else {
			cgip = cgiList[cid];
			cgip->handle = pHandle;
			cgip->stdIn = stdIn;
			cgip->stdOut = stdOut;
			cgip->cgiPath = cgiPath;
			cgip->argp = argp;
			cgip->envp = envp;
			cgip->wp = wp;
			cgip->fplacemark = 0;
			fmtAlloc(&cgip->cgiWebVarFile, gstrlen(cgiWebVars) + 1, T("%s"), cgiWebVars);
			websTimeoutCancel(wp);
		}
	}
/*
 *	Restore the current working directory after spawning child CGI
 */
 	gchdir(cwd);
	return 1;
}



/******************************************************************************/
/*
 *	Any entry in the cgiList need to be checked to see if it has
 */
void websCgiGatherOutput (cgiRec *cgip)
{
	gstat_t	sbuf;
	char_t	cgiBuf[FNAMESIZE];
	if (cgip == NULL || !websValid(cgip->wp)) {
		return;
	}
	if ((gstat(cgip->stdOut, &sbuf) == 0) && 
		(sbuf.st_size > cgip->fplacemark)) {
		int fdout;
		fdout = gopen(cgip->stdOut, O_RDONLY | O_BINARY, 0444 );
/*
 *		Check to see if any data is available in the
 *		output file and send its contents to the socket.
 */
		if (fdout >= 0) {
			webs_t	wp = cgip->wp;
			int		nRead;
/*
 *			Write the HTTP header on our first pass
 */
			if (cgip->fplacemark == 0) {
				websWrite(wp, T("HTTP/1.0 200 OK\r\n"));
			}
			glseek(fdout, cgip->fplacemark, SEEK_SET);
			while ((nRead = gread(fdout, cgiBuf, FNAMESIZE)) > 0) {
				if (websWriteBlock(wp, cgiBuf, nRead) < 0) {
					break;
				}
				cgip->fplacemark += nRead;
			}
			gclose(fdout);
		}
	}
}



/******************************************************************************/
/*
 *	Any entry in the cgiList need to be checked to see if it has
 *	completed, and if so, process its output and clean up.
 */
void websCgiCleanup()
{
	cgiRec	*cgip;
	webs_t	wp;
	char_t	**ep;
	int		cid;
#ifdef WIN
	int		nTries;
#endif
	for (cid = 0; cid < cgiMax; cid++) {
		if ((cgip = cgiList[cid]) != NULL) {
			wp = cgip->wp;
			if (!websValid(wp)) {
				/* Client connection disappeared before CGI exited; wait for exit to unlink files */
				if (websCheckCgiProc(cgip->handle) != 0) {
					continue;
				}
			} else {
				websCgiGatherOutput(cgip);
			}
			if (websCheckCgiProc(cgip->handle) == 0) {
/*
 *				We get here if the CGI process has terminated.  Clean up.
 */
#ifdef WIN
				nTries = 0;
				while ((cgip->fplacemark == 0) && (nTries < 100)) {
					websCgiGatherOutput(cgip);
					if (cgip->fplacemark == 0) {
						Sleep(10);
					}
					nTries++;
				}
#else
				if (websValid(wp)) {
					websCgiGatherOutput(cgip);
				}
#endif
				if (websValid(wp)) {
					if (cgip->fplacemark == 0) {
						websError(wp, 200, T("CGI generated no output"));
					} else {
						websDone(wp, 200);
					}
				}

/*
 *				Collect WebVar output from CGI and parse it
 */
				if (cgip->cgiWebVarFile && cgip->cgiWebVarFile[0] != '\0') {
					MT_CollectExecWebParams(cgip->cgiWebVarFile);
					gunlink(cgip->cgiWebVarFile);
				}

/*
 *				Remove the temporary re-direction files
 */
				if (cgip->stdIn) {
					gunlink(cgip->stdIn);
				}
				bfreeSafe(B_L, cgip->stdIn);

				if (cgip->stdOut) {
					gunlink(cgip->stdOut);
				}
/*
 *				Free all the memory buffers pointed to by cgip.
 */
				cgiMax = hFree((void***) &cgiList, cid);
				for (ep = cgip->envp; ep != NULL && *ep != NULL; ep++) {
					bfreeSafe(B_L, *ep);
				}
				bfreeSafe(B_L, cgip->cgiPath);
				bfreeSafe(B_L, cgip->argp);
				bfreeSafe(B_L, cgip->envp);
				bfreeSafe(B_L, cgip->stdOut);
				bfreeSafe(B_L, cgip->cgiWebVarFile);
				bfreeSafe(B_L, cgip);
			}
		}
	}
}
/******************************************************************************/

