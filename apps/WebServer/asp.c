/*
 * asp.c -- Active Server Page Support
 *
 * Copyright (c) GoAhead Software Inc., 1995-2000. All Rights Reserved.
 *
 * See the file "license.txt" for usage and redistribution license requirements
 *
 * $Id: asp.c,v 1.3 2002/10/24 14:44:50 bporter Exp $
 */

/******************************** Description *********************************/

/*
 *	The ASP module processes ASP pages and executes embedded scripts. It 
 *	support an open scripting architecture with in-built support for 
 *	Ejscript(TM).
 */

/********************************* Includes ***********************************/

#include	"wsIntrn.h"
/********************************** Locals ************************************/

sym_fd_t	websAspFunctions = -1;	/* Symbol table of functions */
static int		aspOpenCount = 0;		/* count of apps using this module */

/***************************** Forward Declarations ***************************/

static char_t	*strtokcmp(char_t *s1, char_t *s2);
static char_t	*skipWhite(char_t *s);

/************************************* Code ***********************************/

/*
 *	Create script spaces and commands
 */

int websAspOpen()
{
	if (++aspOpenCount == 1) {
/*
 *	Create the table for ASP functions
 */
		websAspFunctions = symOpen(WEBS_SYM_INIT_1031);

/*
 *	Create standard ASP commands
 */
		websAspDefine(T("write"), websAspWrite);
	}
	return 0;
}

/************************************* Code ***********************************/
/*
 *	Close Asp symbol table.
 */

void websAspClose()
{
	if (--aspOpenCount <= 0) {
		if (websAspFunctions != -1) {
			symClose(websAspFunctions);
			websAspFunctions = -1;
		}
	}
}

/******************************************************************************/
/*
 *	Process ASP requests and expand all scripting commands. We read the
 *	entire ASP page into memory and then process. If you have really big 
 *	documents, it is better to make them plain HTML files rather than ASPs.
 */

int startOrigParser(webs_t wp, char_t **lpath, int *ejid, char_t **nextp, char_t **last, int *rc)
{
	char_t *token, *ep, *cp, *lang, *result;
	int engine;
	engine = EMF_SCRIPT_EJSCRIPT;
	*nextp = skipWhite(*nextp + 2);
	
/*
 *		Decode the language
 */
		token = T("language");
		if ((lang = strtokcmp(*nextp, token)) != NULL) {
			if ((cp = strtokcmp(lang, T("=javascript"))) != NULL) {
				engine = EMF_SCRIPT_EJSCRIPT;
			} else {
				cp = *nextp;
			}
			*nextp = cp;
		}
		
/*
 *		Find tailing bracket and then evaluate the script
 */
		if ((ep = gstrstr(*nextp, T("%>"))) != NULL) {
			*ep = '\0';
			*last = ep + 2;
			*nextp = skipWhite(*nextp);
/*
 *			Handle backquoted newlines
 */
			for (cp = *nextp; *cp; ) {
				if (*cp == '\\' && (cp[1] == '\r' || cp[1] == '\n')) {
					*cp++ = ' ';
					while (*cp == '\r' || *cp == '\n') {
						*cp++ = ' ';
					}
				} else {
					cp++;
				}
			}

/*
 *			Now call the relevant script engine. Output is done directly
 *			by the ASP script procedure by calling websWrite()
 */
			if (**nextp) {
				result = NULL;
				if (engine == EMF_SCRIPT_EJSCRIPT) {
					//printf("startOrigParser1: cmd=%s\n", *nextp);
					*rc = scriptEval(engine, *nextp, &result, *ejid);
				} else {
					//printf("startOrigParser2: cmd=%s\n", *nextp);
					*rc = scriptEval(engine, *nextp, &result, (int) wp);
				}
				if (*rc < 0) {
/*
 *					On an error, discard all output accumulated so far
 *					and store the error in the result buffer. Be careful if the
 *					user has called websError() already.
 */
					if (websValid(wp)) {
						if (result) {
							websWrite(wp, T("<h2><b>ASP Error: %s</b></h2>\n"), 
								result);
							websWrite(wp, T("<pre>%s</pre>"), *nextp);
							bfree(B_L, result);
						} else {
							websWrite(wp, T("<h2><b>ASP Error</b></h2>\n%s\n"),
								*nextp);
						}
						websWrite(wp, T("</body></html>\n"));
						*rc = 0;
					}
					//goto done;
					return 0;
				}
			}
			return 1;
		} else {
			websError(wp, 200, T("Unterminated script in %s: \n"), *lpath);
			*rc = -1;
			//goto done;
			return 0;
		}
}
//startMultiLangParser: ruleFlag=>fullHTML or dynamicHTML
int startMultiLangParser(webs_t wp, char_t **nextp, char_t **last, int *rc, char *startTagStart, char *startTagEnd, char *endTag, char *ruleFlag)
{
	//printf("startMultiLangParser: startTagStart=%s(%d), startTagEnd=%s(%d), endTag=%s(%d)\n", startTagStart, strlen(startTagStart), startTagEnd, strlen(startTagEnd), endTag, strlen(endTag));
	
	char_t *token, *ep, *cp, *lang, *result;
	char tagTmp[32], tagBuf[32], language[8];
	char *begin, *end, *transString;
	int loopCount, saveCount;

	*nextp = skipWhite(*nextp + strlen(startTagStart));
	ep = gstrstr(*nextp, T(startTagEnd));
	if (!strcmp(ruleFlag, "fullHTML"))
		*ep = '\0';
	*last = ep + strlen(startTagEnd);
	*nextp = skipWhite(*nextp);
	
/*
 *	Handle backquoted newlines
 */
	for (cp = *nextp; *cp; ) {
		if (*cp == '\\' && (cp[1] == '\r' || cp[1] == '\n')) {
			*cp++ = ' ';
			while (*cp == '\r' || *cp == '\n') {
				*cp++ = ' ';
			}
		}
		else if ((*cp == '\\')) {
			*cp++ = ' ';
		}
		else {
			cp++;
		}
	}
	memset(tagBuf, '\0', sizeof(tagBuf));
	memset(tagTmp, '\0', sizeof(tagTmp));
	
	if (**nextp)
	{
		//printf("startMultiLangParser: tag=%s\n", *nextp);
		begin = strchr(*nextp, '"');
		if (!begin)
		{
			printf("startMultiLangParser: We can't get start \" character, please check translation file!\n");
			return 1;
		}
		begin = begin + 1;
		end = strchr(begin, '"');
		if (!end)
		{
			printf("startMultiLangParser: We can't get end \" character, please check translation file!\n");
			return 1;
		}
		/*memcpy(tagBuf, begin, end-begin);
		*(tagBuf + (end-begin)) = '\0';
		printf("startMultiLangParser: tag=%s(len=%d)\n", tagBuf, strlen(tagBuf));*/
		memcpy(tagTmp, begin, end-begin);
		*(tagTmp + (end-begin)) = '\0';
		
		saveCount=0;
		for (loopCount=0; loopCount<strlen(tagTmp); loopCount++)
		{
			if (tagTmp[loopCount] != ' ')
			{
				tagBuf[saveCount] = tagTmp[loopCount];
				saveCount++;
			}
		}
		//printf("startMultiLangParser: tag=%s(len=%d)\n", tagBuf, strlen(tagBuf));
	}
	
	*nextp = *last;
	ep = gstrstr(*nextp, T(endTag));
	if (!strcmp(ruleFlag, "fullHTML"))
		*ep = '\0';
	*last = ep + strlen(endTag);
	*nextp = skipWhite(*nextp);
	
	for (cp = *nextp; *cp; ) {
		if (*cp == '\\' && (cp[1] == '\r' || cp[1] == '\n')) {
			*cp++ = ' ';
			while (*cp == '\r' || *cp == '\n') {
				*cp++ = ' ';
			}
		}
		else if ((*cp == '\\')) {
			*cp++ = ' ';
		}
		else {
			cp++;
		}
	}
	
	if (**nextp)
	{
		//printf("startMultiLangParser2: Orig String=%s\n", *nextp);

		if (lang_get(tagBuf, &transString) == 1)
		{
			//printf("startMultiLangParser: trans String=%s\n", transString);
			/*if (!strcmp(ruleFlag, "GUIRequest"))
				printf("startMultiLangParser1: trans String=%s\n", transString);*/
			if (!strcmp(ruleFlag, "GUIRequest"))
				return websWrite(wp, T("\"%s\""), transString);
			else
				websWrite(wp, T("%s"), transString);
		}
		else
		{
			/*if (!strcmp(ruleFlag, "GUIRequest"))
				printf("startMultiLangParser2\n");*/
			if (!strcmp(ruleFlag, "GUIRequest"))
				return websWrite(wp, T("\"%s\""), *nextp);
			else
				websWrite(wp, T("%s"), *nextp);
		}
	}
	
	*rc = 0;
	return 1;
}


int websAspRequest(webs_t wp, char_t *lpath)
{
	debug_printf("websAspRequest: Into websAspRequest for trace web crashed issue. path:%s\n", lpath);
	websStatType	sbuf;
	char			*rbuf;
	//char_t			*token, *lang, *result, *path, *ep, *cp, *buf, *nextp, *origParser, *multiLangParser;
	char_t			*path, *buf, *nextp, *origParser, *multiLangParser;
	char_t			*last, *lastOrigParser, *lastMultiLangParser;
	//int				rc, engine, len, ejid;
	int				rc, len, ejid;

	a_assert(websValid(wp));
	a_assert(lpath && *lpath);

	rc = -1;
	buf = NULL;
	rbuf = NULL;
	
	wp->flags |= WEBS_HEADER_DONE;
	path = websGetRequestPath(wp);

	debug_printf("websAspRequest: Create Ejscript instance in case it is needed. path:%s\n", lpath);
/*
 *	Create Ejscript instance in case it is needed
 */
	ejid = ejOpenEngine(wp->cgiVars, websAspFunctions);
	if (ejid < 0) {
		websError(wp, 200, T("Can't create Ejscript engine"));
		goto done;
	}
	ejSetUserHandle(ejid, (int) wp);

	if (websPageStat(wp, lpath, path, &sbuf) < 0) {
		websError(wp, 200, T("Can't stat %s"), lpath);
		goto done;
	}

	MT_PreParsePage(lpath,ejid);
	
	debug_printf("websAspRequest: Create a buffer to hold the ASP file in-memory. path:%s\n", lpath);
/*
 *	Create a buffer to hold the ASP file in-memory
 */
	len = sbuf.size * sizeof(char);
	if ((rbuf = balloc(B_L, len + 1)) == NULL) {
		websError(wp, 200, T("Can't get memory"));
		goto done;
	}
	rbuf[len] = '\0';

	if (websPageReadData(wp, rbuf, len) != len) {
		websError(wp, 200, T("Cant read %s"), lpath);
		goto done;
	}
	websPageClose(wp);

	debug_printf("websAspRequest: Convert to UNICODE if necessary. path:%s\n", lpath);
/*
 *	Convert to UNICODE if necessary.
 */
	if ((buf = ballocAscToUni(rbuf, len)) == NULL) {
		websError(wp, 200, T("Can't get memory"));
		goto done;
	}

	debug_printf("websAspRequest: Scan for the next \"<%\". path:%s\n", lpath);
/*
 *	Scan for the next "<%"
 */
	last = buf;
	rc = 0;
	//Jacky.Yang 5-Feb-2009, for multi-lang
	int optionMode=0, stopLoop=0;
	char startTagStart[8], startTagEnd[8], endTagStart[8], endTag[18];
	//printf("websAspRequest: last:%s\n", last);
	//while (rc == 0 && *last && ((nextp = gstrstr(last, T("<%"))) != NULL)) {
	while (rc == 0 && *last && !stopLoop) {
		lastOrigParser = last;
		lastMultiLangParser = last;
		origParser = NULL;
		multiLangParser = NULL;
		//printf("websAspRequest: lastOrigParser=%x, lastMultiLangParser=%x\n", lastOrigParser, lastMultiLangParser);
		origParser = gstrstr(lastOrigParser, T("<%"));
		multiLangParser = gstrstr(lastMultiLangParser, T("<!--#tr"));
		//printf("websAspRequest: lastOrigParser=%x, lastMultiLangParser=%x\n", lastOrigParser, lastMultiLangParser);
		if ((origParser != NULL) || (multiLangParser != NULL)) {
			//printf("websAspRequest: origParser=%x, multiLangParser=%x\n", origParser, multiLangParser);
			//if ((origParser < multiLangParser) && origParser != 0)
			if ((origParser != 0) && ((multiLangParser == 0) || (origParser < multiLangParser)))
			{
				//printf("websAspRequest: 1\n");
				optionMode = 1; //optionMode=1, Run original parser.
				nextp = origParser;
				last = lastOrigParser;
			}
			//else if ((multiLangParser < origParser) && multiLangParser != 0)
			else if ((multiLangParser != 0) && ((origParser == 0) || (multiLangParser < origParser)))
			{
				//printf("websAspRequest: 2\n");
				optionMode = 2; //optionMode=2, Run multi-lang parser.
				nextp = multiLangParser;
				last = lastMultiLangParser;
				strcpy(startTagStart, "<!--#tr");
				strcpy(startTagEnd, "-->");
				strcpy(endTag, "<!--#endtr-->");
			}
			else
			{
				//printf("websAspRequest: 3\n");
				optionMode = 0; //Error!!
			}
			websWriteBlock(wp, last, (nextp - last));
			//printf("websAspRequest: lpath=%x, ejid=%d, nextp=%x, last=%x, rc=%d\n", lpath, ejid, nextp, last, rc);
			if (optionMode == 1)//optionMode=1, Run original parser.
			{
				//printf("websAspRequest: 4\n");
				if (startOrigParser(wp, &lpath, &ejid, &nextp, &last, &rc) == 0)
					goto done;
			}
			else if (optionMode == 2)//optionMode=2, Run multi-lang parser.
			{
				//printf("websAspRequest: 5\n");
				if (startMultiLangParser(wp, &nextp, &last, &rc, startTagStart, startTagEnd, endTag, "fullHTML") == 0)
					goto done;
			}
			else
				stopLoop  = 1;
		}
		else
			stopLoop  = 1;
	}
	
	debug_printf("websAspRequest: Output any trailing HTML page text. path:%s\n", lpath);
/*
 *	Output any trailing HTML page text
 */
	if (last && *last && rc == 0) {
		websWriteBlock(wp, last, gstrlen(last));
	}
	rc = 0;
	//debug_printf("websAspRequest: Exit websAspRequest for trace web crashed issue. path:%s\n\n\n", lpath);

/*
 *	Common exit and cleanup
 */
 	debug_printf("websAspRequest: done. path:%s\n", lpath);
done:
	//Jacky.Yang 26-Nov-2008, drop it.
	//MT_PostParsePage(lpath);
	if (websValid(wp)) {
		websPageClose(wp);
		if (ejid >= 0) {
			ejCloseEngine(ejid);
		}
	}
	bfreeSafe(B_L, buf);
	bfreeSafe(B_L, rbuf);
	debug_printf("websAspRequest: Exit websAspRequest from done for trace web crashed issue. path:%s\n\n\n", lpath);
	return rc;
}

/******************************************************************************/
/*
 *	Define an ASP Ejscript function. Bind an ASP name to a C procedure.
 */

int websAspDefine(char_t *name, 
	int (*fn)(int ejid, webs_t wp, int argc, char_t **argv))
{
	return ejSetGlobalFunctionDirect(websAspFunctions, name, 
		(int (*)(int, void*, int, char_t**)) fn);
}

/******************************************************************************/
/*
 *	Asp write command. This implemements <% write("text"); %> command
 */

int websAspWrite(int ejid, webs_t wp, int argc, char_t **argv)
{
	int		i;

	a_assert(websValid(wp));
	
	for (i = 0; i < argc; ) {
		a_assert(argv);
		if (websWriteBlock(wp, argv[i], gstrlen(argv[i])) < 0) {
			return -1;
		}
		if (++i < argc) {
			if (websWriteBlock(wp, T(" "), 2) < 0) {
				return -1;
			}
		}
	}
	return 0;
}

/******************************************************************************/
/*
 *	strtokcmp -- Find s2 in s1. We skip leading white space in s1.
 *	Return a pointer to the location in s1 after s2 ends.
 */

static char_t *strtokcmp(char_t *s1, char_t *s2)
{
	int		len;

	s1 = skipWhite(s1);
	len = gstrlen(s2);
	for (len = gstrlen(s2); len > 0 && (tolower(*s1) == tolower(*s2)); len--) {
		if (*s2 == '\0') {
			return s1;
		}
		s1++;
		s2++;
	}
	if (len == 0) {
		return s1;
	}
	return NULL;
}

/******************************************************************************/
/*
 *	Skip white space
 */

static char_t *skipWhite(char_t *s) 
{
	a_assert(s);

	if (s == NULL) {
		return s;
	}
	while (*s && gisspace(*s)) {
		s++;
	}
	return s;
}

/******************************************************************************/

