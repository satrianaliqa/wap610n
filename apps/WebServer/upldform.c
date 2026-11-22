/* upldForm.c - GoForm to handle file upload */

/*
modification history
--------------------
01a,13feb01,lohct  written.
*/

/*
DESCRIPTION
This GoForm procedure handles http file upload.

SEE ALSO:
"GoForms - GoAhead WebServer" 
*/

#include "webs.h"
#include "mt_api.h"
#include "utils.h"

#ifndef WIN
#include "zlib.h"
#include "pthread.h"
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <unistd.h>
#include <errno.h>
pthread_t gBurnThread;
static int gBurnPostIsSysMalloc = 0;
#endif

/* forward declarations */
void        upldForm(webs_t wp, char_t * path, char_t * query);
void        BurnImage(webs_t wp, char_t * path, char_t * query);

typedef struct {
	unsigned int magic;
	unsigned int pcksum2;
	unsigned int headerID;
	unsigned int deviceID;
	unsigned int filler[3];
	unsigned int cksum;
} MT_IMAGE_HEADER;

static int MT_IsLittleEndian()
{
	int testInt = 0x12345678;
	unsigned char* pTestByte = (unsigned char*)&testInt;
	return (int)(*pTestByte == 0x78);
}

// Reverse 32-bit values for big-endian hosts
static void RevULong(unsigned int* pVal)
{
	*pVal = (((*pVal) & 0xFF) << 24) |
		(((*pVal) & 0xFF00) << 8) |
		(((*pVal) & 0xFF0000) >> 8) |
		((*pVal) >> 24);
}

void *MT_BurnAndVerifyImage(void* data)
{
	int locWrite = 0;
	int ferror_val = 0;
	int locVerify = 0;
	int numLeft = 0;
	int numWrite = 0;
	int numRead = 0;
	int retryCount = 0;
	int verifiedOk = 0;
	int i = 0;
	FILE* fp = NULL;
	MT_IMAGE_HEADER imageHeader;
	int isLittleEndian = MT_IsLittleEndian();
	int dummy_crc[] = {0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF};
	int size;

	debug_printf("MT_BurnAndVerifyImage:Into MT_BurnAndVerifyImage function.(pid=%d)\n", getpid());

	// Checksum for the uploaded file
	gMTBurnParams.burnState = checksum_e;
	gMTBurnParams.precentComplete = 0;

	sprintf(gMTBurnParams.errorMessage,"Flash update in progress - do not interrupt this process !");


	// Make sure to override the ending checksum in the current image header
	// 1. Read header
	// 2. Extract the location of the last CRC
	// 3. Write FF instead of the current CRC
	// 4. Close the file.
	if ((fp = fopen(gMTBurnParams.fName, "rb")) != NULL)
	{
		numRead = fread(&imageHeader,1,sizeof(MT_IMAGE_HEADER),fp);
		fclose(fp);
		fp = NULL;

		if (numRead == sizeof(MT_IMAGE_HEADER))
		{
			if (!isLittleEndian)
			{
				RevULong(&imageHeader.magic);
				RevULong(&imageHeader.headerID);
				RevULong(&imageHeader.deviceID);
				RevULong(&imageHeader.cksum);
				RevULong(&imageHeader.pcksum2);
			}
			
			printf ("imageheader.pcksum2 = %d\n",imageHeader.pcksum2);

			if (imageHeader.magic == 0xEA000006 &&
				imageHeader.pcksum2 >= sizeof(MT_IMAGE_HEADER) &&
				imageHeader.pcksum2 <= (unsigned int)(KERNEL_MTD_PARTITION_SIZE - 16))
			{
				if ((fp = fopen(gMTBurnParams.fName, "r+b")) != NULL)
				{
					if (fseek(fp, (long)imageHeader.pcksum2, SEEK_SET) == 0) {
						fwrite(dummy_crc, 1, 4 * sizeof(int), fp);
						fflush(fp);
						fsync(fileno(fp));
						ioctl(fileno(fp), BLKFLSBUF, 0);
					}
					fclose(fp);
					fp = NULL;
				}
			}
		}
	}

	// Burning
	gMTBurnParams.burnState = burning_e;
	gMTBurnParams.precentComplete = 0;

	//doSystem("ps | grep webs > /dev/console");
	for (retryCount = 0; retryCount<gMTBurnParams.retryCount && !verifiedOk;retryCount++)
	{
		locWrite = 0;
		numLeft = gMTBurnParams.imageSize;
		debug_printf("MT_BurnAndVerifyImage: gMTBurnParams.retryCount=%d(pid=%d)\n", gMTBurnParams.retryCount, getpid());
		
		if (retryCount>0)
		{
			sprintf(gMTBurnParams.errorMessage,"Flash update in progress - do not interrupt this process !(retry %d)",retryCount+1);
		}
		if ((fp = fopen(gMTBurnParams.fName, "w+b")) != NULL)
		{
			doSystem("rm -f /var/fw_upgrade_status");
			while (numLeft > 0) {
				doSystem("echo %d,%d >> /var/fw_upgrade_status", gMTBurnParams.burnState, gMTBurnParams.precentComplete);
				debug_printf("\n\n\n\n");
				debug_printf("MT_BurnAndVerifyImage: 1. check numLeft(%d)(pid=%d), gMTBurnParams.precentComplete=%d > FILE_SINGLE_WRITE ? FILE_SINGLE_WRITE : numLeft\n", numLeft, getpid(), gMTBurnParams.precentComplete);
				size = numLeft > FILE_SINGLE_WRITE ? FILE_SINGLE_WRITE : numLeft;
				debug_printf("MT_BurnAndVerifyImage: 2. check numLeft(%d) > FILE_SINGLE_WRITE ? FILE_SINGLE_WRITE : numLeft\n", numLeft);
				
				//doSystem("ps | grep webs > /dev/console");
				debug_printf("MT_BurnAndVerifyImage: numLeft=%d\n", numLeft);
				printf("%d\n",numLeft);
				numWrite = fwrite(&(gMTBurnParams.fileContent[locWrite]), sizeof(*(gMTBurnParams.postData)), size, fp);
				debug_printf("MT_BurnAndVerifyImage: numWrite=%d\n", numWrite);
				if (numWrite <= 0 || ferror(fp))
				{
					ferror_val = ferror(fp);
					sprintf(gMTBurnParams.errorMessage,T("Error : File could not be written<br>ferror=%d locWrite=%d numLeft=%d numWrite=%d Size=%d bytes"), ferror_val, locWrite, numLeft, numWrite, gMTBurnParams.imageSize);
					gMTBurnParams.precentComplete = -1;
					gMTBurnParams.burnState = error_e;
					printf("Error numWrite=0");
					break;
				}
        
				locWrite += numWrite;
				numLeft -= numWrite;
				debug_printf("MT_BurnAndVerifyImage: locWrite=%d, numLeft=%d\n", locWrite, numLeft);
				gMTBurnParams.precentComplete = locWrite*100/gMTBurnParams.imageSize;

				debug_printf("MT_BurnAndVerifyImage: Next while loop. numLeft > 0(numLef=%d)\n", numLeft);
			}
			doSystem("echo %d,%d >> /var/fw_upgrade_status", gMTBurnParams.burnState, gMTBurnParams.precentComplete);
			debug_printf("MT_BurnAndVerifyImage: Exit loop, numLeft > 0(numLef=%d)\n", numLeft);
			
			if (numLeft == 0) {
				if (fflush(fp) != 0) {
					ferror_val = ferror(fp);
					sprintf(gMTBurnParams.errorMessage,T("Error : File flush failed.<br> errno=%d ferror=%d locWrite=%d Size=%d bytes"), errno, ferror_val, locWrite, gMTBurnParams.imageSize);
					gMTBurnParams.precentComplete = -1;
					gMTBurnParams.burnState = error_e;
				} else {
					fsync(fileno(fp));
					ioctl(fileno(fp), BLKFLSBUF, 0);
				}
				if (fclose(fp) != 0) {
					sprintf(gMTBurnParams.errorMessage,T("Error : File close failed.<br> errno=%d locWrite=%d numLeft=%d numWrite=%d Size=%d bytes"), errno, locWrite, numLeft, numWrite, gMTBurnParams.imageSize);
					gMTBurnParams.precentComplete = -1;
					gMTBurnParams.burnState = error_e;
					printf("Error can not close file\n");
				}
				fp = NULL;
			} else {
				sprintf(gMTBurnParams.errorMessage,T("Error : File upload incomplete - numLeft=%d locWrite=%d Size=%d bytes<br>"), numLeft, locWrite,  gMTBurnParams.imageSize);
				gMTBurnParams.precentComplete = -1;
				gMTBurnParams.burnState = error_e;
				printf("Error numLeft=0\n");
				fclose(fp);
				fp = NULL;
			}
		}
		else
		{
			sprintf(gMTBurnParams.errorMessage,T("Error : Flash File could not be Opened"));
			gMTBurnParams.precentComplete = -1;
			gMTBurnParams.burnState = error_e;
			printf("can not open file\n");
		}

		debug_printf("MT_BurnAndVerifyImage: Check gMTBurnParams.precentComplete:%d\n", gMTBurnParams.precentComplete);
		if (gMTBurnParams.precentComplete==100)
		{
			char_t verBuff[1024];
			debug_printf("MT_BurnAndVerifyImage: gMTBurnParams.precentComplete = 100!!\n");

			// Verifying
			gMTBurnParams.burnState = verifying_e;
			gMTBurnParams.precentComplete = 0;
			numLeft = gMTBurnParams.imageSize;
			locVerify = 0;
			debug_printf("MT_BurnAndVerifyImage: Trying to open file %s\n", gMTBurnParams.fName);
			printf("Trying to open file %s\n",gMTBurnParams.fName);
			if ((fp = fopen(gMTBurnParams.fName, "rb")) != NULL)
			{
				debug_printf("MT_BurnAndVerifyImage: We can open file %s and set verifiedOk = 1\n", gMTBurnParams.fName);
				ioctl(fileno(fp), BLKFLSBUF, 0);
				verifiedOk = 1;
				while (numLeft > 0 && verifiedOk == 1) {
					numRead = fread(verBuff,sizeof(char_t),1024,fp);
					if (numRead > numLeft)
						numRead = numLeft;
					if (numRead > 0)
					{
						for (i = 0; i < numRead; i++)
						{
							if (verBuff[i] != gMTBurnParams.fileContent[locVerify]) {
								verifiedOk = 0;
								printf ("ERROR FOUND !\n");
								break;
							}
							locVerify++;
							gMTBurnParams.precentComplete = locVerify*100/gMTBurnParams.imageSize;
						}
						numLeft -= numRead;
					} else {
						ferror_val = ferror(fp);
						verifiedOk = 0;
						printf("Error reading flash during verify (numLeft=%d)\n", numLeft);
						break;
					}
				}
				debug_printf("MT_BurnAndVerifyImage: Closing\n");
				printf ("Closing\n");
				ferror_val = ferror(fp);
				fclose(fp);
				fp = NULL;
			}
			
			debug_printf("MT_BurnAndVerifyImage: Check verifiedOk = %d\n", verifiedOk);
			if (verifiedOk == 1)
			{
				debug_printf("MT_BurnAndVerifyImage: verifiedOk = 1 then break the loop.\n");
				gMTBurnParams.precentComplete = 100;
				break;
			}
		}
	}

	debug_printf("MT_BurnAndVerifyImage: Done.(pid=%d)\n", getpid());
	// Done !
	if (gMTBurnParams.postData) {
		if (gBurnPostIsSysMalloc) {
			free(gMTBurnParams.postData);
		} else {
			bfree(B_L, gMTBurnParams.postData);
		}
		gMTBurnParams.postData = NULL;
	}
	gBurnPostIsSysMalloc = 0;
	gMTBurnParams.fileContent = NULL;

	debug_printf("MT_BurnAndVerifyImage: Check verifiedOk != 1\n");
	if (verifiedOk != 1)
	{
		debug_printf("MT_BurnAndVerifyImage: verifiedOk != 1\n");
		gMTBurnParams.precentComplete = -1;
		gMTBurnParams.burnState = error_e;
		sprintf(gMTBurnParams.errorMessage,T("Error : Error verifying image at location %d<br>ferror=%d"), locVerify, ferror_val);
	}
	else
	{
		debug_printf("MT_BurnAndVerifyImage: verifiedOk = 1\n");
		gMTBurnParams.errorMessage[0]='\0';
		gMTBurnParams.burnState = done_e;
		gMTBurnParams.precentComplete = 100;
		
		
		doSystem("rm -f /var/firmware_upgrading");
		debug_printf("MT_BurnAndVerifyImage: Start to reboot\n");
		system("sleep 3 ; reboot &");
	}

	// Need to reboot the system now !
	//doSystem("ps | grep webs > /dev/console");
	debug_printf("MT_BurnAndVerifyImage: finish!\n");
	
	//Jacky.Yang 22-Jun-2009.
	pthread_exit(0);
}

/*
	A simple function to write a redirection javascript to the client, in order
	to set the borwser to jump to the required page
*/
void MT_WriteRedirectScript(webs_t wp, char_t* newUrl)
{
	char_t *webLocPrefix = "/";
	char_t cleanUrl[512];
	char_t *src;
	char_t *dst;

	if (newUrl == NULL || *newUrl == '\0') {
		return;
	}

	/* Sanitize URL: filter out dangerous characters ', ", <, >, ;, \ to prevent XSS (W-11) */
	src = newUrl;
	dst = cleanUrl;
	while (*src != '\0' && (dst - cleanUrl) < (int)(sizeof(cleanUrl) - 1)) {
		if (*src != '\'' && *src != '\"' && *src != '<' && *src != '>' && *src != ';' && *src != '\\') {
			*dst++ = *src;
		}
		src++;
	}
	*dst = '\0';

//	websWrite(wp, T("content-type: text/html"));
	websWrite(wp, T("<HTML><HEAD></HEAD><BODY>"));
	websWrite(wp, T("<script type=\"text/javascript\">\n"));
	websWrite(wp, T("<!--\n"));
	if (strstr(cleanUrl, "http://"))
		websWrite(wp, T("window.location = '%s'\n"), cleanUrl);
	else
		websWrite(wp, T("window.location = '%s%s'\n"), webLocPrefix, cleanUrl);
	websWrite(wp, T("//-->\n"));
	websWrite(wp, T("</script>\n"));
	websWrite(wp, T("</BODY></HTML>"));

}

static void writeProcOrFlagFile(const char *filePath, const char *val)
{
	FILE *f = fopen(filePath, "w");
	if (f != NULL) {
		fputs(val, f);
		fclose(f);
	}
}

/*******************************************************************************
*
* upldForm - GoForm procedure to handle file uploads for upgrading device
*
* This routine handles http file uploads for upgrading device.
*
* RETURNS: OK, or ERROR if the I/O system cannot install the driver.
*/

void upldForm(webs_t wp, char_t * path, char_t * query) {
    FILE *       fp = NULL;
    char_t *     fn = NULL;
    char_t *     bn = NULL;
	char_t *     mw = NULL;
	char_t *	 cf = NULL;
	char_t *     fnonly;
	int			 configurationfile;
    int          locWrite;
    int          numLeft;
    int          numWrite=0;
	int			 maxWrite;
	size_t		 i=0;
	char_t		 redirectPage[FNAMESIZE]; 
	char_t		 currentPath[FNAMESIZE];
	char_t		 targetFile[FNAMESIZE];
	int			 configHeaderValid = 1;
	int 		rebootNeed=0;
	pid_t		pid;
	int			fd;
	
    a_assert(websValid(wp));
    websHeader(wp);

	mw = websGetVar(wp, T("uploadSizeLimit"), T("0"));
	cf = websGetVar(wp, T("configurationfile"), T("0"));
	configurationfile = atoi(cf);
	maxWrite = atoi(mw) * 1024;
	if (maxWrite < 0) {
		maxWrite = 0;
	} else if (maxWrite > 65536) {
		maxWrite = 65536;
	}
    bn = websGetVar(wp, T("remote_filename"), T(""));

	if (configurationfile)
	{
#if defined(WIN)
		bn = "C:\\configfile.cfg";
#else
		bn = "/tmp/configfile.cfg";
#endif
	}
	else
	{
		/* [BUG-02] Disallow arbitrary user-controlled remote_filename to overwrite system binaries */
		bn = "/tmp/configfile.cfg";
	}

    fn = websGetVar(wp, T("filename"), T(""));
	if (!fn || strlen(fn)==0)
	{
		fn = websGetVar(wp,T("reload_filename"),T(""));
	}
    if ((!bn || strlen(bn) == 0) && fn != NULL && *fn != '\0') {
        if ((int)(bn = gstrrchr(fn, '/') + 1) == 1) {
            if ((int)(bn = gstrrchr(fn, '\\') + 1) == 1) {
                bn = fn;
            }
        }
    }

	// Check if the target file is actually a file or a folder
	ggetcwd(currentPath,256);
	strcpy(targetFile,bn);
	for (i=0;i<strlen(targetFile);i++)
	{
		if (targetFile[i]=='\\')
			targetFile[i]='/';
	}
	for (i=0;i<strlen(fn);i++)
	{
		if (fn[i]=='\\')
			fn[i]='/';
	}


	if (!gchdir(bn))
	{
		gchdir(currentPath);
		if (targetFile[strlen(targetFile)-1] != '/')
			strcat (targetFile,"/");
		fnonly = gstrrchr(fn,'/'); 
		if (fnonly)
		{
			if (fnonly[0] == '/')
				fnonly++;
			strcat(targetFile,fnonly);
		}
		else
		{
			strcat(targetFile,fn);
		}
	}


	if (!configurationfile)
	{
		websWrite(wp, T("Size = %d bytes<br>"), wp->FileContentLen);
	}

    if ((fp = fopen((targetFile == NULL ? "upldForm.bin" : targetFile), "w+b")) == NULL) 
	{
		for (i=0;i<strlen(targetFile);i++)
		{
			if (targetFile[i]=='\\')
				targetFile[i]='/';
		}
		if (configurationfile)
		{
			//Jacky.Yang redirect to management.asp for u-media.
			//sprintf(redirectPage,T("importexport.asp?error=File+%s+could+not+be+opened"),targetFile);
			sprintf(redirectPage,T("admin/management.asp?error=File+%s+could+not+be+opened"),targetFile);
			MT_WriteRedirectScript(wp, redirectPage);
		}
		else
		{
			//sprintf(redirectPage,T("upload.asp?error=Could+Not+Open+File&size=%d&lf=%s&rf=%s"),wp->FileContentLen,fn,targetFile);
			//MT_WriteRedirectScript(wp, redirectPage);
			websWrite(wp, T("Error : File could not be opened<br>"));
		}

    } 
	else 
	{
        locWrite = 0;
        numLeft = wp->FileContentLen;
		if (configurationfile && (wp->FileContent == NULL || numLeft <= (int)MT_CONFIG_FILE_SIG_LEN || numLeft > maxWrite))
		{
			fclose(fp);
			fp = NULL;
			unlink(targetFile);
			if (wp->FileContent == NULL || numLeft <= (int)MT_CONFIG_FILE_SIG_LEN) {
				sprintf(redirectPage, T("admin/management.asp?error=Invalid+Configuration+File"));
			} else {
				sprintf(redirectPage, T("admin/management.asp?error=File+size+limit+exceeded+allowed+maximum+size+of+%d+KB"), maxWrite/1024);
			}
			MT_WriteRedirectScript(wp, redirectPage);
		}
		else
		{
			
			if (configurationfile)
			{
				char_t ConfSIG[MT_CONFIG_FILE_SIG_LEN];
				unsigned int ConfSIGLen;

				//Check Header
				configHeaderValid = 0;
	
				ConfSIGLen = MT_GetConfigHeader(ConfSIG, MT_CONFIG_FILE_SIG_LEN);

				if (wp->FileContent != NULL && ConfSIGLen > 0 && numLeft >= (int)ConfSIGLen && memcmp(wp->FileContent, ConfSIG, ConfSIGLen) == 0)
					configHeaderValid = 1;
				
			}

			while (configHeaderValid && wp->FileContent != NULL && numLeft > 0) {
				numWrite = fwrite(&(wp->FileContent[locWrite]), sizeof(*(wp->postData)), numLeft, fp);
				if (numWrite < numLeft) {
					if (configurationfile)
					{
						//Jacky.Yang redirect to management.asp for u-media.
						//sprintf(redirectPage,T("importexport.asp?error=File+write+failed.<br>+ferror=%d+locWrite=%d+numLeft=%d+numWrite=%d+Size=%d+bytes"), ferror(fp), locWrite, numLeft, numWrite, wp->FileContentLen);
						sprintf(redirectPage,T("admin/management.asp?error=File+write+failed.<br>+ferror=%d+locWrite=%d+numLeft=%d+numWrite=%d+Size=%d+bytes"), ferror(fp), locWrite, numLeft, numWrite, wp->FileContentLen);
						MT_WriteRedirectScript(wp, redirectPage);
					}
					else
					{
						//sprintf(redirectPage,T("upload.asp?error=Could+Not+Write+File&size=%d&lf=%s&rf=%s"),wp->FileContentLen,fn,targetFile);
						//MT_WriteRedirectScript(wp, redirectPage);
						websWrite(wp, T("Error : File could not be written<br>ferror=%d locWrite=%d numLeft=%d numWrite=%d Size=%d bytes"), ferror(fp), locWrite, numLeft, numWrite, wp->FileContentLen);
					}

					break;
				}
				
				locWrite += numWrite;
				numLeft -= numWrite;
			}

			if (configHeaderValid == 0 && configurationfile)
			{
				fclose(fp);
				fp = NULL;
				unlink(targetFile);
				sprintf(redirectPage,T("admin/management.asp?error=Invalid+Configuration+File"));
				MT_WriteRedirectScript(wp, redirectPage);
			}
			else if (numLeft == 0) {
				if (fclose(fp) != 0) {
					fp = NULL;
					unlink(targetFile);
					if (configurationfile)
					{
						sprintf(redirectPage,T("admin/management.asp?error=File+close+failed.<br>+errno=%d+locWrite=%d+numLeft=%d+numWrite=%d+Size=%d+bytes"), errno, locWrite, numLeft, numWrite, wp->FileContentLen);
						MT_WriteRedirectScript(wp, redirectPage);
					}
					else
					{
						websWrite(wp, T("Error : File close failed.<br> errno=%d locWrite=%d numLeft=%d numWrite=%d Size=%d bytes"), errno, locWrite, numLeft, numWrite, wp->FileContentLen);
					}
				} else {
					fp = NULL;
					for (i=0;i<strlen(targetFile);i++)
					{
						if (targetFile[i]=='\\')
							targetFile[i]='/';
					}
					if (configurationfile)
					{
						sprintf(redirectPage,T("reboot_page.asp"));
						MT_WriteRedirectScript(wp, redirectPage);
						rebootNeed = 1;
					}
					else 
					{
						websWrite(wp, T("Upload Complete<br>"));
					}
				}
			} else {
				if (fp != NULL) {
					fclose(fp);
					fp = NULL;
				}
				unlink(targetFile);
				websWrite(wp, T("Error : File upload incomplete - numLeft=%d locWrite=%d Size=%d bytes<br>"), numLeft, locWrite, wp->FileContentLen);
			}
		}
    }
	if (rebootNeed == 1) {
		wp->flags &= ~WEBS_KEEP_ALIVE;
		pid = fork();
		if (pid < 0) {
			unlink(targetFile);
			websWrite(wp, T("Error : unable to fork apply process<br>"));
			websFooter(wp);
			websDone(wp, 200);
		} else if (pid != 0) {
			debug_printf("upldForm: fork 1: pid is %d\n", getpid());
			websFooter(wp);
			websDone(wp, 200);
			debug_printf("upldForm: Leave here.(1) (pid=%d)\n", getpid());
		} else {
			for (fd = 3; fd < 256; fd++) {
				close(fd);
			}
			debug_printf("upldForm: fork 2: pid is %d\n", getpid());
			system("echo 2 > /dev/gpio2");
			loadConfigFromFile(targetFile);
			commitToFlash(wp, path, query);
			debug_printf("upldForm: Leave here.(2) (pid=%d)\n", getpid());
			_exit(0);
		}
	}
	else
	{
		wp->flags &= ~WEBS_KEEP_ALIVE;
		websFooter(wp);
		websDone(wp, 200);
		debug_printf("upldForm: Leave here.(3) (pid=%d)\n", getpid());
	}
}    /* void upldForm(webs_t wp, char_t * path, char_t * query) */


/*
	Implementation of the file stream send for sending files from the web server to
	the client, forcing the Save As dialog to pop up on all MIME types.
*/
void sendFileForm(webs_t wp, char_t * path, char_t * query) 
{
	char_t *	sendFileName;
	char_t *	profileCaption;
	char_t *	profileToolTip;
	char_t *	isConfig;
	char_t *    fileNameWithNoPath;
	FILE *		fileHandle;
	char_t		buff[512];

	size_t		fsize=0;
	long		ftellRet = 0;
	size_t		freadBytes = 0;
	char_t		redirectPage[256];

	a_assert(websValid(wp));

	sendFileName   = websGetVar(wp, T("filename"), T(""));
	profileToolTip = websGetVar(wp, T("Tooltip"), T(""));
	profileCaption = websGetVar(wp, T("Caption"), T(""));
	isConfig = websGetVar(wp, T("isconfig"), T("0"));

	/* [BUG-01] Whitelist path in sendFileForm: enforce strictly /tmp/configfile.conf */
	if (strcmp(isConfig, "1") == 0 || strcmp(sendFileName, "/tmp/configfile.conf") == 0)
	{
		sendFileName = "/tmp/configfile.conf";
		MT_SaveParamsToFile(sendFileName, profileCaption, profileToolTip);
	}
	else
	{
		wp->flags &= ~WEBS_KEEP_ALIVE;
		websHeader(wp);
		snprintf(redirectPage, sizeof(redirectPage), T("admin/management.asp?error=Access+Denied"));
		MT_WriteRedirectScript(wp, redirectPage);
		websFooter(wp);
		websDone(wp, 200);
		return;
	}

	if (strlen(sendFileName) > 0)
	{
		fileHandle = fopen(sendFileName, "rb");
		if (fileHandle != NULL)
		{
			if (strcmp(isConfig, "1") != 0)
			{
				char *slash = strrchr(sendFileName, '/');
				fileNameWithNoPath = slash ? slash + 1 : sendFileName;
			}
			else
			{
				fileNameWithNoPath = "configfile.conf";
			}

			if (fseek(fileHandle, 0, SEEK_END) != 0 || (ftellRet = ftell(fileHandle)) < 0 ||
				fseek(fileHandle, 0, SEEK_SET) != 0)
			{
				fclose(fileHandle);
				wp->flags &= ~WEBS_KEEP_ALIVE;
				websHeader(wp);
				snprintf(redirectPage, sizeof(redirectPage), T("admin/management.asp?error=Internal+Failure+could+not+read+%s"), sendFileName);
				MT_WriteRedirectScript(wp, redirectPage);
				websFooter(wp);
				websDone(wp, 200);
				return;
			}
			fsize = (size_t)ftellRet;

			wp->flags &= ~WEBS_KEEP_ALIVE;
			websWrite(wp, T("HTTP/1.0 200 OK\r\n"));
			websWrite(wp, T("Server: %s\r\n"), WEBS_NAME);
			websWrite(wp, T("Connection: close\r\n"));
			websWrite(wp, T("Pragma: no-cache\r\n"));
			websWrite(wp, T("Cache-control: no-cache\r\n"));
			websWrite(wp, T("Content-Length: %lu\r\n"), (unsigned long)fsize);
			websWrite(wp, T("Content-Type: application/x-unknown\r\n"));
			websWrite(wp, T("Content-Disposition: attachment; filename=\"%s\"\r\n"), fileNameWithNoPath);
			websWrite(wp, T("\r\n"));

			while ((freadBytes = fread(buff, 1, sizeof(buff), fileHandle)) > 0) {
				if (websWriteBlock(wp, buff, (int)freadBytes) < 0) {
					break;
				}
			}

			fclose(fileHandle);
		}
		else
		{
			wp->flags &= ~WEBS_KEEP_ALIVE;
			websHeader(wp);
			snprintf(redirectPage, sizeof(redirectPage), T("admin/management.asp?error=Internal+Failure+could+not+find+%s"), sendFileName);
			MT_WriteRedirectScript(wp, redirectPage);
			websFooter(wp);
		}
	}
	else
	{
		wp->flags &= ~WEBS_KEEP_ALIVE;
		websHeader(wp);
		snprintf(redirectPage, sizeof(redirectPage), T("admin/management.asp?error=Internal+Failure+Invalid+file+name"));
		MT_WriteRedirectScript(wp, redirectPage);
		websFooter(wp);
	}

	websDone(wp, 200);
}

#ifndef WIN

static int MT_ReturnValidationError()
{
	printf("%s\n",gMTBurnParams.errorMessage);
	gMTBurnParams.precentComplete = -1;
	return FALSE; // Bad image
}

// Validate the CRC and HW information in the image
static int MT_ValidateImage(const char_t* imageBuf, int imageSize)
{
	unsigned int crcFromFooter, imageSizeFromFile;
	MT_IMAGE_HEADER imageHeader;
	uLong crc;
	int isLittleEndian = MT_IsLittleEndian();

	if (imageBuf == NULL || imageSize < (int)(sizeof(MT_IMAGE_HEADER) + 16) || imageSize > KERNEL_MTD_PARTITION_SIZE)
	{
		gsprintf(gMTBurnParams.errorMessage,T("Error: Invalid firmware image size (%d bytes)<br>"), imageSize);
		return MT_ReturnValidationError();
	}

	memcpy(&imageHeader, imageBuf, sizeof(MT_IMAGE_HEADER));
	if (!isLittleEndian)
	{
		RevULong(&imageHeader.magic);
		RevULong(&imageHeader.headerID);
		RevULong(&imageHeader.deviceID);
		RevULong(&imageHeader.cksum);
		RevULong(&imageHeader.pcksum2);
	}

	if (imageHeader.magic != 0xEA000006)
	{
		gsprintf(gMTBurnParams.errorMessage,T("Error: This is not an image file (wrong magic code)<br>"));
		return MT_ReturnValidationError();
	}

	if (imageHeader.pcksum2 < sizeof(MT_IMAGE_HEADER) ||
		imageHeader.pcksum2 != (unsigned int)imageSize - 16)
	{
		gsprintf(gMTBurnParams.errorMessage,T("Error: Wrong image size in header %u<br>"), imageHeader.pcksum2);
		return MT_ReturnValidationError();
	}

	if (imageHeader.headerID != 1)
	{
		gsprintf(gMTBurnParams.errorMessage,T("Error: Unsupported image (header version is %d)<br>"), imageHeader.headerID);
		return MT_ReturnValidationError();
	}

	/* Strict zero-fork WAP610N hardware Device ID check (0x0012) */
	if (imageHeader.deviceID != 0x0012)
	{
		gsprintf(gMTBurnParams.errorMessage,
			T("Error: The image is not compatible with WAP610N hardware (device ID 0x%08X instead of 0x00000012)<br>"),
			imageHeader.deviceID);
		return MT_ReturnValidationError();
	}

	memcpy(&crcFromFooter, imageBuf + imageSize - 4, 4);
	if (!isLittleEndian) RevULong(&crcFromFooter);

	imageSizeFromFile = imageHeader.pcksum2 - sizeof(MT_IMAGE_HEADER);

	crc = crc32(0L, Z_NULL, 0);
	crc = crc32(crc, imageBuf + sizeof(MT_IMAGE_HEADER), imageSizeFromFile);
	if ((unsigned int)crc != crcFromFooter)
	{
		gsprintf(gMTBurnParams.errorMessage,T("Error: Bad CRC: CRC is %08X but in the header its %08X !!!<br>"), (unsigned int)crc, crcFromFooter);
		return MT_ReturnValidationError();
	}

	printf("CRC of image: %08X - OK.\n", (unsigned int)crc);

	if (crcFromFooter != imageHeader.cksum)
	{
		gsprintf(gMTBurnParams.errorMessage,T("Error: Wrong image file (header CRC %08X != footer CRC %08X)<br>"), imageHeader.cksum, crcFromFooter);
		return MT_ReturnValidationError();
	}

	return TRUE;
}
#endif // !WIN

void BurnImage(webs_t wp, char_t * path, char_t * query) 
{
	char_t		 redirectPage[MT_MAX_LINE_LENGTH];
    char_t       targetFile[MT_MAX_LINE_LENGTH];
	char_t		 statusPageName[MT_MAX_LINE_LENGTH]; 
#ifndef WIN
	char_t*		 validationFileName;
    char_t*      sourceFile;
#endif
	int			 fileNameValid = 1;
	int			 retryVal = 3;

    a_assert(websValid(wp));

	// Make sure we do not clear the status if we're already in the burn process.
	if (gMTBurnParams.burnState<=error_e)
		MT_InitBurnParams(&gMTBurnParams);

	gstrncpy(statusPageName ,websGetVar(wp, T("page_name"), T("admin/upgrade.asp")),MT_MAX_LINE_LENGTH-1);
	statusPageName[MT_MAX_LINE_LENGTH - 1] = '\0';
	/* [BUG-03] Hardcode targetFile to /dev/mtdblock2; disallow targeting /dev/mtdblock1 */
	gstrncpy(targetFile, "/dev/mtdblock2", MT_MAX_LINE_LENGTH-1);

    websHeader(wp);
		
#ifdef WIN

	gMTBurnParams.precentComplete = -1;
	gMTBurnParams.burnState = error_e;
	gstrncpy(gMTBurnParams.errorMessage,"Image burn is not supported in Windows",MT_MAX_PARAM_VALUE_LENGTH-1);

#else
	printf("BurnImage State=%d\n",gMTBurnParams.burnState);
	validationFileName = websGetVar(wp, T("validate_file_name"), T(""));
    sourceFile =  websGetVar(wp, T("filename"), T(""));
	/*if (gstrlen(validationFileName)>0)
	{
		if (gstrlen(sourceFile)>0)
		{
			char_t* pfName = NULL;
			pfName = gstrrchr(sourceFile,'/');
			if (!pfName)
				pfName = gstrrchr(sourceFile,'\\');
			if (!pfName)
				pfName = sourceFile;
			else
				pfName++;

			if (gstrcmp(pfName,validationFileName) != 0)
			{
				fileNameValid = 0;
			}
		}
		else 
		{
			fileNameValid = 0;
		}
	}*/

    if (wp->FileContentLen < 0)
    {
		/* already in burn process? */
		if (gMTBurnParams.burnState > error_e)
		{
			printf("Already burning !\n");
		}
        /* we couldn't allocate memory */
        else if (wp->FileContentLen == UPLOAD_NO_MEMORY)
		{
			gsprintf(gMTBurnParams.errorMessage,T("Error : Insufficient memory to upload the file.<br>"));
			gMTBurnParams.precentComplete = -1;
			printf("%s\n",gMTBurnParams.errorMessage);
		}
		/* file is too big */
        else if (wp->FileContentLen == UPLOAD_FILE_TO_BIG)
		{
			gsprintf(gMTBurnParams.errorMessage,T("Error : File length exceeded Maximum<br>"));
			gMTBurnParams.precentComplete = -1;
			printf("%s\n",gMTBurnParams.errorMessage);
		}
    }
	else if (gMTBurnParams.burnState > error_e)
	{
		printf("Already burning !\n");
	}
	else
	{
		if (wp->FileContentLen > IMAGE_MAX_SIZE)
		{
			sprintf(gMTBurnParams.errorMessage,T("Error : File length exceeded Maximum<br>"));
			gMTBurnParams.precentComplete = -1;
			printf("%s\n",gMTBurnParams.errorMessage);
		}
		else if (wp->FileContentLen > KERNEL_MTD_PARTITION_SIZE)
		{
			sprintf(gMTBurnParams.errorMessage,T("Error : Firmware is larger than the kernel MTD partition<br>"));
			gMTBurnParams.precentComplete = -1;
			printf("%s\n",gMTBurnParams.errorMessage);
		}
		else if (wp->FileContentLen == 0 || fileNameValid == 0)
		{
			sprintf(gMTBurnParams.errorMessage,T("Error : Invalid image file specified.<br>"));
			gMTBurnParams.precentComplete = -1;
			printf("%s\n",gMTBurnParams.errorMessage);
		}
		else if (!targetFile || !gstrlen(targetFile))
		{
			sprintf(gMTBurnParams.errorMessage,T("Error : File could not be opened<br>"));
			gMTBurnParams.precentComplete = -1;
			printf("%s\n",gMTBurnParams.errorMessage);
		}
		else if (MT_ValidateImage(wp->FileContent, wp->FileContentLen))
		{
			retryVal = gatoi(websGetVar(wp, T("retry_max"), T("3")));
			if (retryVal < 1 || retryVal > 5) {
				retryVal = 3;
			}
			gMTBurnParams.retryCount = retryVal;
			gMTBurnParams.burnState = checksum_e;
			gMTBurnParams.fileContent = wp->FileContent;
			gstrcpy(gMTBurnParams.fName, targetFile);
			gMTBurnParams.imageSize = wp->FileContentLen;
			gMTBurnParams.postData = wp->postData;
			gBurnPostIsSysMalloc = (wp->flags & WEBS_SYS_MALLOC_POST) ? 1 : 0;
			gMTBurnParams.precentComplete = 0;
			wp->postData = NULL;

			/* Write GPIO/flag directly and batch daemon shutdown into one shell call
			 * to avoid repeated fork() spikes under -lpthread while holding ~3.8 MB. */
			writeProcOrFlagFile("/dev/gpio2", "2\n");
			writeProcOrFlagFile("/var/firmware_upgrading", "1\n");
			doSystem("killall upnpd udhcpc wpa_supplicant >/dev/null 2>&1; "
				"pkill -f '[m]tlk_upnpd_monitor\\.sh|[m]tlk_pbc_reboot\\.sh|[W]PS_PBC\\.sh' >/dev/null 2>&1; "
				"cd /root/mtlk/etc && ./mtlk_wps_cmd.tcl stop >/dev/null 2>&1");

			if (pthread_create(&gBurnThread, NULL, &MT_BurnAndVerifyImage, NULL) != 0) {
				unlink("/var/firmware_upgrading");
				gMTBurnParams.burnState = error_e;
				gMTBurnParams.precentComplete = -1;
				gsprintf(gMTBurnParams.errorMessage, T("Error : Failed to start burn thread<br>"));
				wp->postData = gMTBurnParams.postData;
				gMTBurnParams.postData = NULL;
				gMTBurnParams.fileContent = NULL;
				gBurnPostIsSysMalloc = 0;
			} else {
				pthread_detach(gBurnThread);
			}
		}
	}
    
#endif
	printf("BurnImage: gMTBurnParams.precentComplete:%d\n", gMTBurnParams.precentComplete);
	{
		FILE *conFp = fopen("/dev/console", "w");
		if (conFp != NULL) {
			fprintf(conFp, "BurnImage: gMTBurnParams.precentComplete:%d(pid=%d)\n", gMTBurnParams.precentComplete, getpid());
			fclose(conFp);
		}
	}
	if (wp->postData != NULL)
	{
		if (wp->flags & WEBS_SYS_MALLOC_POST) {
			free(wp->postData);
		} else {
			bfree(B_L, wp->postData);
		}
		wp->postData = NULL;
		printf("Freeing wp postdata\n");
	}

    // Redirect to the status page or respond to hidden iframe
	if (statusPageName[0] == '\0' || strcmp(statusPageName, "admin/upgrade.asp") == 0 || strcmp(statusPageName, "/admin/upgrade.asp") == 0) {
		websWrite(wp, T("<head><title>OK</title></head><body>OK</body>\r\n"));
	} else {
		snprintf(redirectPage, sizeof(redirectPage), "%s", statusPageName);
		redirectPage[sizeof(redirectPage) - 1] = '\0';
		MT_WriteRedirectScript(wp, redirectPage);
	}
    wp->flags &= ~WEBS_KEEP_ALIVE;
    websFooter(wp);
    websDone(wp, 200);
}

/*
 * Dump Firmware directly from physical MTD NOR flash partitions.
 * Zero RAM overhead: direct bit-exact streaming over HTTP.
 * Uses select() on raw socket to avoid busy-spin at 100% CPU.
 */
void formDumpFirmware(webs_t wp, char_t * path, char_t * query)
{
	FILE *fp = NULL;
	char buff[4096];
	size_t n = 0;
	char_t *dumpType = websGetVar(wp, T("type"), T("full"));
	const char *devPath = "/dev/mtd0";
	const char *outName = "WAP610N_full_flash_dump.bin";
	unsigned long dumpSize = 0x400000; /* 4MB = 4,194,304 bytes */
	int sockFd = -1;
	socket_t *sp = NULL;

	if (gMTBurnParams.burnState > error_e) {
		websError(wp, 503, T("Firmware upgrade in progress"));
		return;
	}

	if (strcmp(dumpType, "kernel") == 0) {
		devPath = "/dev/mtd2";
		outName = "WAP610N_kernel_rootfs_dump.bin";
		dumpSize = 0x3A0000; /* 3.625MB = 3,801,088 bytes */
	} else if (strcmp(dumpType, "config") == 0) {
		devPath = "/dev/mtd4";
		outName = "WAP610N_config_dump.bin";
		dumpSize = 0x20000; /* 128KB = 131,072 bytes */
	}

	fp = fopen(devPath, "rb");
	if (!fp) {
		if (strcmp(dumpType, "kernel") == 0) devPath = "/dev/mtdblock2";
		else if (strcmp(dumpType, "config") == 0) devPath = "/dev/mtdblock4";
		else devPath = "/dev/mtdblock0";
		fp = fopen(devPath, "rb");
	}

	if (!fp) {
		websError(wp, 500, T("Cannot open MTD device for dumping"));
		return;
	}

	wp->flags &= ~WEBS_KEEP_ALIVE;
	websWrite(wp, T("HTTP/1.0 200 OK\r\n"));
	websWrite(wp, T("Server: %s\r\n"), WEBS_NAME);
	websWrite(wp, T("Connection: close\r\n"));
	websWrite(wp, T("Pragma: no-cache\r\n"));
	websWrite(wp, T("Cache-control: no-cache\r\n"));
	websWrite(wp, T("Content-Type: application/octet-stream\r\n"));
	websWrite(wp, T("Content-Disposition: attachment; filename=\"%s\"\r\n"), outName);
	websWrite(wp, T("Content-Length: %lu\r\n"), dumpSize);
	websWrite(wp, T("\r\n"));

	sockFd = socketGetHandle(wp->sid);
	sp = socketPtr(wp->sid);
	while (sp != NULL && ringqLen(&sp->outBuf) > 0) {
		fd_set wfds;
		struct timeval tv;
		int selRet;

		if (socketFlush(wp->sid) < 0) {
			goto dump_done;
		}
		if (ringqLen(&sp->outBuf) == 0 || sockFd < 0) {
			break;
		}
		FD_ZERO(&wfds);
		FD_SET(sockFd, &wfds);
		tv.tv_sec = 10;
		tv.tv_usec = 0;
		selRet = select(sockFd + 1, NULL, &wfds, NULL, &tv);
		if (selRet < 0 && errno == EINTR) {
			continue;
		}
		if (selRet <= 0) {
			goto dump_done;
		}
	}

	if (sockFd >= 0) {
		while ((n = fread(buff, 1, sizeof(buff), fp)) > 0) {
			size_t written = 0;
			while (written < n) {
				fd_set wfds;
				struct timeval tv;
				int selRet;
				ssize_t w;

				FD_ZERO(&wfds);
				FD_SET(sockFd, &wfds);
				tv.tv_sec = 10;
				tv.tv_usec = 0;

				selRet = select(sockFd + 1, NULL, &wfds, NULL, &tv);
				if (selRet < 0 && errno == EINTR) {
					continue;
				}
				if (selRet <= 0) {
					goto dump_done;
				}

				w = write(sockFd, buff + written, n - written);
				if (w < 0) {
					if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
						continue;
					}
					goto dump_done;
				}
				if (w == 0) {
					goto dump_done;
				}
				written += (size_t)w;
			}
		}
	} else {
		while ((n = fread(buff, 1, sizeof(buff), fp)) > 0) {
			if (websWriteBlock(wp, buff, n) < 0) break;
		}
	}

dump_done:
	fclose(fp);
	websDone(wp, 200);
}

