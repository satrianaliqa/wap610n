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

#ifndef WIN
#include "zlib.h"
#include "pthread.h"
pthread_t gBurnThread;
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

static int MT_ReturnCRCClearError()
{
	printf("%s\n",gMTBurnParams.errorMessage);
	gMTBurnParams.precentComplete = -1;
	gMTBurnParams.burnState = error_e;
	return 0; // Bad image
}

void *MT_BurnAndVerifyImage(void* data)
{
	debug_printf("MT_BurnAndVerifyImage:Into MT_BurnAndVerifyImage function.(pid=%d)\n", getpid());
	int locWrite = 0;
	int locVerify = 0;
	int numLeft = 0;
	int numWrite = 0;
	int numRead = 0;
	int retryCount = 0;
	int verifiedOk = 0;
	int i = 0;
	int chunkSize = 0;
	FILE* fp = NULL;
	MT_IMAGE_HEADER imageHeader;
	int isLittleEndian = MT_IsLittleEndian();
	int dummy_crc[] = {0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF};
	int size;

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

			if ((fp = fopen(gMTBurnParams.fName, "ab")) != NULL)
			{
				fseek(fp,imageHeader.pcksum2,SEEK_SET);
				fwrite(dummy_crc,1,4*sizeof(int),fp);
				fclose(fp);
				fp = NULL;
			}
			else
			{
				sprintf(gMTBurnParams.errorMessage,"Error : Could not open flash image for CRC erase");
				MT_ReturnCRCClearError("");
				return 0;
			}
			
		}
		else
		{
			sprintf(gMTBurnParams.errorMessage,"Error : Could not read header (read %d bytes out of %d)",numRead,sizeof(MT_IMAGE_HEADER));
			MT_ReturnCRCClearError("");
			return 0;
		}
	}
	else
	{
		sprintf(gMTBurnParams.errorMessage,"Error : Could not open flash image for reading header.");
		MT_ReturnCRCClearError("");
		return 0;
	}


	gMTBurnParams.precentComplete = 100;


	// Burning
	gMTBurnParams.burnState = burning_e;
	gMTBurnParams.precentComplete = 0;

    locWrite = 0;
    numLeft = gMTBurnParams.imageSize;
	//doSystem("ps | grep webs > /dev/console");
	for (retryCount = 0; retryCount<gMTBurnParams.retryCount && !verifiedOk;retryCount++)
	{
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
                fflush(fp);
				if (!numWrite) 
				{
					sprintf(gMTBurnParams.errorMessage,T("Error : File could not be written<br>ferror=%d locWrite=%d numLeft=%d numWrite=%d Size=%d bytes"), ferror(fp), locWrite, numLeft, numWrite, gMTBurnParams.imageSize);
					gMTBurnParams.precentComplete = -1;
					gMTBurnParams.burnState = error_e;
					printf("Error numWrite=0");
					break;
				}
        
				locWrite += numWrite;
				numLeft -= numWrite;
				debug_printf("MT_BurnAndVerifyImage: locWrite=%d, numLeft=%d\n", locWrite, numLeft);
				gMTBurnParams.precentComplete = locWrite*100/gMTBurnParams.imageSize;

				//////////////////////////////////////////////////////////////////////////
				//////////////////////////////////////////////////////////////////////////
				chunkSize += numWrite;
				if (chunkSize >= (FILE_SINGLE_WRITE)) 
				{
					debug_printf("MT_BurnAndVerifyImage: into chunkSize >= (FILE_SINGLE_WRITE)\n");
					chunkSize = 0;

					// close the file to allow it to flush all data.
					if (fclose(fp) != 0) {
						sprintf(gMTBurnParams.errorMessage,T("Error : File close failed  (reopen for flushing).<br> errno=%d locWrite=%d numLeft=%d numWrite=%d Size=%d bytes"), errno, locWrite, numLeft, numWrite, gMTBurnParams.imageSize);             
						gMTBurnParams.precentComplete = -1;
						gMTBurnParams.burnState = error_e;
						printf("Error can not close file  (reopen for flushing)\n");
						break;
					}

					fp = NULL;
					
					// reopen the file and seek to the last location to continue writing the next chunk.
					if ((fp = fopen(gMTBurnParams.fName, "w+b")) == NULL)
					{
						sprintf(gMTBurnParams.errorMessage,T("Error : Flash File could not be Opened (reopen for flushing)"));
						gMTBurnParams.precentComplete = -1;
						gMTBurnParams.burnState = error_e;
						printf("can not open file  (reopen for flushing)\n");
						break;
					}
					
					fseek(fp,locWrite,SEEK_SET);
				}
				//////////////////////////////////////////////////////////////////////////
				//////////////////////////////////////////////////////////////////////////

				debug_printf("MT_BurnAndVerifyImage: Next while loop. numLeft > 0(numLef=%d)\n", numLeft);
			}
			doSystem("echo %d,%d >> /var/fw_upgrade_status", gMTBurnParams.burnState, gMTBurnParams.precentComplete);
			debug_printf("MT_BurnAndVerifyImage: Exit loop, numLeft > 0(numLef=%d)\n", numLeft);
			
			if (numLeft == 0) {
				if (fclose(fp) != 0) {
					sprintf(gMTBurnParams.errorMessage,T("Error : File close failed.<br> errno=%d locWrite=%d numLeft=%d numWrite=%d Size=%d bytes"), errno, locWrite, numLeft, numWrite, gMTBurnParams.imageSize);             
					gMTBurnParams.precentComplete = -1;
					gMTBurnParams.burnState = error_e;
					printf("Error can not close file\n");
				}
			} else {
				sprintf(gMTBurnParams.errorMessage,T("Error : File upload incomplete - numLeft=%d locWrite=%d Size=%d bytes<br>"), numLeft, locWrite,  gMTBurnParams.imageSize);
				gMTBurnParams.precentComplete = -1;
				gMTBurnParams.burnState = error_e;
				printf("Error numLeft=0\n");
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
			debug_printf("MT_BurnAndVerifyImage: gMTBurnParams.precentComplete = 100!!\n");
			char_t verBuff[1024];

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
				verifiedOk = 1;
				while (numLeft > 0 && verifiedOk == 1) {
					printf("Reading file\n");
					numRead = fread(verBuff,sizeof(char_t),1024,fp);
                    // make sure you don't exceed buffer , because this isn't really a file ...
                    if (numRead > numLeft) 
                         numRead = numLeft;
					
                    if (numRead>0)
					{
						for (i = 0; i < numRead;i++)
						{
							if (verBuff[i]!=gMTBurnParams.fileContent[locVerify]) {
								verifiedOk = 0;
								printf ("ERROR FOUND !\n");
								break;
							}
							locVerify++;
							gMTBurnParams.precentComplete = locVerify*100/gMTBurnParams.imageSize;
						}
						numLeft-=numRead;
						printf("%d\n",numLeft);
					}
				}
				debug_printf("MT_BurnAndVerifyImage: Closing\n");
				printf ("Closing\n");
				fclose(fp);
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
	if (gMTBurnParams.postData)
		free(gMTBurnParams.postData);

	gMTBurnParams.fileContent = NULL;

	debug_printf("MT_BurnAndVerifyImage: Check verifiedOk != 1\n");
	if (verifiedOk != 1)
	{
		debug_printf("MT_BurnAndVerifyImage: verifiedOk != 1\n");
		gMTBurnParams.precentComplete = -1;
		gMTBurnParams.burnState = error_e;
		sprintf(gMTBurnParams.errorMessage,T("Error : Error verifying image at location %d<br>ferror=%d"), locVerify, ferror(fp));
	}
	else
	{
		debug_printf("MT_BurnAndVerifyImage: verifiedOk = 1\n");
		gMTBurnParams.errorMessage[0]='\0';
		gMTBurnParams.burnState = done_e;
		gMTBurnParams.precentComplete = 100;
		
		
		doSystem("rm -f /var/firmware_upgrading"); //Ricky add, I'll also use this flag.
		debug_printf("MT_BurnAndVerifyImage: Start to reboot\n");
		checkStartToWait();
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
	char_t *	 webLocPrefix="/";

//	websWrite(wp, T("content-type: text/html"));
	websWrite(wp, T("<HTML><HEAD></HEAD><BODY>"));
	websWrite(wp, T("<script type=\"text/javascript\">\n"));
	websWrite(wp, T("<!--\n"));
	if (strstr(newUrl,"http://"))
		websWrite(wp, T("window.location = '%s'\n"),newUrl);
	else
		websWrite(wp, T("window.location = '%s%s'\n"),webLocPrefix,newUrl);
	websWrite(wp, T("//-->\n"));
	websWrite(wp, T("</script>\n"));
	websWrite(wp, T("</BODY></HTML>"));

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
	
    a_assert(websValid(wp));
    websHeader(wp);

	mw = websGetVar(wp, T("uploadSizeLimit"), T("0"));
	cf = websGetVar(wp, T("configurationfile"), T("0"));
	configurationfile = atoi(cf);
	maxWrite = atoi(mw) * 1024;
    bn = websGetVar(wp, T("remote_filename"), T(""));

	if (configurationfile)
	{
#if defined(WIN)
		bn = "C:\\configfile.cfg";
#else
		bn = "/tmp/configfile.cfg";
#endif
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
		//sprintf(redirectPage,T("upload.asp?error=OK&size=%d&lf=%s&rf=%s&reload=%s"),wp->FileContentLen,fn,targetFile,fn);
		//MT_WriteRedirectScript(wp, redirectPage);
		websWrite(wp, T("Filename = %s<br>Filename on dongle = %s<BR>Size = %d bytes<br>"), fn,bn, wp->FileContentLen);

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
		if (configurationfile && numLeft>maxWrite)
		{
			//Jacky.Yang redirect to management.asp for u-media.
			//sprintf(redirectPage,T("importexport.asp?error=File+size+limit+exceeded+allowed+maximum+size+of+%d+KB"),maxWrite/1024);
			sprintf(redirectPage,T("admin/management.asp?error=File+size+limit+exceeded+allowed+maximum+size+of+%d+KB"),maxWrite/1024);
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
	
				ConfSIGLen = MT_GetConfigHeader(ConfSIG,MT_CONFIG_FILE_SIG_LEN);

				if (memcmp(wp->FileContent,ConfSIG,ConfSIGLen) == 0)
					configHeaderValid = 1;
				
			}

			while (configHeaderValid && numLeft > 0) {
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
				//sprintf(redirectPage,T("importexport.asp?error=Invalid+Configuration+File"));
				sprintf(redirectPage,T("admin/management.asp?error=Invalid+Configuration+File"));
				MT_WriteRedirectScript(wp, redirectPage);
			}
			else if (numLeft == 0) {
				if (fclose(fp) != 0) {
					if (configurationfile)
					{
						//Jacky.Yang redirect to management.asp for u-media.
						//sprintf(redirectPage,T("importexport.asp?error=File+close+failed.<br>+errno=%d+locWrite=%d+numLeft=%d+numWrite=%d+Size=%d+bytes"), errno, locWrite, numLeft, numWrite, wp->FileContentLen);
						sprintf(redirectPage,T("admin/management.asp?error=File+close+failed.<br>+errno=%d+locWrite=%d+numLeft=%d+numWrite=%d+Size=%d+bytes"), errno, locWrite, numLeft, numWrite, wp->FileContentLen);
						MT_WriteRedirectScript(wp, redirectPage);
					}
					else
					{
						//sprintf(redirectPage,T("upload.asp?error=Could+No+Close+File&size=%d&lf=%s&rf=%s"),wp->FileContentLen,fn,targetFile);
						websWrite(wp, T("Erro : File close failed.<br> errno=%d locWrite=%d numLeft=%d numWrite=%d Size=%d bytes"), errno, locWrite, numLeft, numWrite, wp->FileContentLen);
					}

				} else {
					for (i=0;i<strlen(targetFile);i++)
					{
						if (targetFile[i]=='\\')
							targetFile[i]='/';
					}
					if (configurationfile)
					{
						//Jacky.Yang
						//sprintf(redirectPage,T("commit.asp?ProfileName=%s"), targetFile);
						sprintf(redirectPage,T("reboot_page.asp"), "");
						MT_WriteRedirectScript(wp, redirectPage);
						rebootNeed = 1;
					}
					else 
					{
						websWrite(wp, T("Upload Complete<br>"));	
						//sprintf(redirectPage,T("upload.asp?error=Complete&size=%d&lf=%s&rf=%s"),wp->FileContentLen,fn,targetFile);
						//MT_WriteRedirectScript(wp, redirectPage);
					}					

				}
			} else {
				websWrite(wp, T("Error : File upload incomplete - numLeft=%d locWrite=%d Size=%d bytes<br>"), numLeft, locWrite, wp->FileContentLen);
			}
		}
    }
	if (rebootNeed == 1) {
		if (fork() != 0) {
			debug_printf("upldForm: fork 1: pid is %d\n", getpid());
    		websFooter(wp);
    		websDone(wp, 200);
    		debug_printf("upldForm: Leave here.(1) (pid=%d)\n", getpid());
    	}
    	else
    	{
    		//Jacky.Yang 13-Aug-2008, add LED blinking when the DUT rebooting
    		debug_printf("upldForm: fork 2: pid is %d\n", getpid());
			system("echo 2 > /dev/gpio2");
    		printf("jacky - targetFile:%s\n", targetFile);
			loadConfigFromFile(targetFile);
			commitToFlash(wp, &path, &query);
    		//system("sleep 3 ; reboot");
    		websFooter(wp);
    		websDone(wp, 200);
    		debug_printf("upldForm: Leave here.(2) (pid=%d)\n", getpid());
    	}
    }
    else
    {
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
	size_t		freadBytes = 0;
	char_t		redirectPage[256]; 

    a_assert(websValid(wp));

	sendFileName   = websGetVar(wp, T("filename"), T(""));
	profileToolTip = websGetVar(wp, T("Tooltip"), T(""));
	profileCaption = websGetVar(wp, T("Caption"), T(""));
	isConfig = websGetVar(wp, T("isconfig"), T("0"));

	if (strcmp(isConfig,"1") == 0)
	{
		MT_SaveParamsToFile(sendFileName,profileCaption,profileToolTip);
	}

	if (strlen(sendFileName)>0)	
	{
		fileHandle = fopen(sendFileName,"rb");
		if (fileHandle != NULL)	
		{
			if (strcmp(isConfig,"1") != 0)
			{
				char *slash = strrchr(sendFileName, '/');
				fileNameWithNoPath = slash ? slash + 1 : sendFileName;
			}
			else 
			{
				fileNameWithNoPath = "configfile.conf";
			}

			
			/*
				Find file size
			*/
			fseek(fileHandle,0,SEEK_END);
			fsize = ftell(fileHandle);
			fseek(fileHandle,0,SEEK_SET);

			/*
				Write the force file send headers.
			*/
			websWrite(wp, T("HTTP/1.0 200 OK\n"));
			websWrite(wp, T("Server: %s\r\n"), WEBS_NAME);
			websWrite(wp, T("Pragma: no-cache\n"));
			websWrite(wp, T("Cache-control: no-cache\n"));
			websWrite(wp, T("Content-Length: %d\n"),fsize);
			websWrite(wp, T("Content-Type: application/x-unknown\n"));
			websWrite(wp, T("content-disposition: attachment; filename=\"%s\"\n"),fileNameWithNoPath);
			websWrite(wp, T("\n"));

			/*
				Send the file as binary block
			*/
			do {
				freadBytes=fread(buff,1,512,fileHandle);
				websWriteBlock(wp,buff,freadBytes);
			}
			while (freadBytes>0);

			fclose(fileHandle);
		} 
		else // Error - can not find file
		{
			if (strcmp(isConfig,"1") == 0)
			{
				//Jacky.Yang redirect to management.asp for u-media.
				//sprintf(redirectPage,T("importexport.asp?error=Internal+Failure+could+not+find+%s"),sendFileName);
				sprintf(redirectPage,T("admin/management.asp?error=Internal+Failure+could+not+find+%s"),sendFileName);
				MT_WriteRedirectScript(wp, redirectPage);
			}

		}
	}
	else // Error - file name is invalid
	{
		//Jacky.Yang redirect to management.asp for u-media.
		//sprintf(redirectPage,T("importexport.asp?error=Internal+Failure+Invalid+file+name+%s"),sendFileName);
		sprintf(redirectPage,T("admin/management.asp?error=Internal+Failure+Invalid+file+name+%s"),sendFileName);
		MT_WriteRedirectScript(wp, redirectPage);
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
	char tmpOutputFile[MT_MAX_PATH_LENGTH];
	char mtdBlockCmnd[MT_MAX_PATH_LENGTH];
	FILE* tmpFile;
	int mtdDeviceID = 0;
	int isLittleEndian = MT_IsLittleEndian();

	// Validate that header is correct
	if (imageSize < sizeof(MT_IMAGE_HEADER) + 8)
	{
		gsprintf(gMTBurnParams.errorMessage,T("Error: This is not an image file (too small)!!!<br>"));
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

	if (imageHeader.pcksum2 + 16 != imageSize) // 12 empty bytes, then CRC
	{
		gsprintf(gMTBurnParams.errorMessage,T("Error: Wrong image size in header %d<br>"), imageHeader.pcksum2);
		return MT_ReturnValidationError();
	}

	if (imageHeader.headerID != 1)
	{
		gsprintf(gMTBurnParams.errorMessage,T("Error: Unsupported image (header version is %d)<br>"), imageHeader.headerID);
		return MT_ReturnValidationError();
	}

	// Check if the device ID is the same as in mtdblock3:
	sprintf(tmpOutputFile, "%s/web_device_id.tmp", MT_WebTmpDir);
	sprintf(mtdBlockCmnd, "get_env_param device_id >%s", tmpOutputFile);
	system(mtdBlockCmnd);
	tmpFile = fopen(tmpOutputFile, "rt");
	if (tmpFile)
	{
		fscanf(tmpFile, "%x", &mtdDeviceID);
		fclose(tmpFile);
	}

	if (mtdDeviceID != 0) // If there's information in mtdblock3, compare device ID
	{
		if (imageHeader.deviceID != mtdDeviceID)
		{
//june.chen, 2011-01-17, support Device ID for both WES610N (0x13) and WET610N (0x11). This is because WET610N and WES610N fw is interchangeable
#if 1
			if(((imageHeader.deviceID == 0x11) || (imageHeader.deviceID == 0x13)) && ((mtdDeviceID == 0x11) || (mtdDeviceID == 0x13))){
			}
			else{
				gsprintf(gMTBurnParams.errorMessage,T("Error: The image is not compatible with this hardware (device ID 0x%08X instead of 0x%08X)<br>"), imageHeader.deviceID, mtdDeviceID);
				return MT_ReturnValidationError();	
			}
#else
			gsprintf(gMTBurnParams.errorMessage,T("Error: The image is not compatible with this hardware (device ID 0x%08X instead of 0x%08X)<br>"), imageHeader.deviceID, mtdDeviceID);
			return MT_ReturnValidationError();
#endif
//june.chen end
		}
	}
	else printf("Warning: device ID not stored in mtdblock3 (or not stored correctly)\n");

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

    a_assert(websValid(wp));

	// Make sure we do not clear the status if we're already in the burn process.
	if (gMTBurnParams.burnState<=error_e)
		MT_InitBurnParams(&gMTBurnParams);

	gstrncpy(statusPageName ,websGetVar(wp, T("page_name"), T("admin/upgrade.asp")),MT_MAX_LINE_LENGTH-1);
	gstrncpy(targetFile , websGetVar(wp, T("remote_filename"), T("")),MT_MAX_LINE_LENGTH-1);

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
		if (gMTBurnParams.burnState>error_e)
		{
			printf("Allready burning !\n");
		}
        /* we couldn't allocate memory */
        else if (wp->FileContentLen == UPLOAD_NO_MEMORY)
		{
			gsprintf(gMTBurnParams.errorMessage,T("Error : Insufficient memory to upload the file.<br>"));
			gMTBurnParams.precentComplete = -1;
			printf("%s\n",gMTBurnParams.errorMessage);
		}
		/* file is too big */
        else  if (wp->FileContentLen == UPLOAD_FILE_TO_BIG)
		{
			gsprintf(gMTBurnParams.errorMessage,T("Error : File length exceeded Maximum<br>"));
			gMTBurnParams.precentComplete = -1;
			printf("%s\n",gMTBurnParams.errorMessage);
		}

		wp->postData = NULL;
    }
	else
	{
		if (wp->FileContentLen > IMAGE_MAX_SIZE)
		{
			sprintf(gMTBurnParams.errorMessage,T("Error : File length exceeded Maximum<br>"));
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
			gMTBurnParams.retryCount = gatoi(websGetVar(wp, T("retry_max"), T("3")));

			gMTBurnParams.burnState = idle_e;
			gMTBurnParams.fileContent = wp->FileContent;
			gstrcpy(gMTBurnParams.fName ,targetFile);
			gMTBurnParams.imageSize = wp->FileContentLen;
			gMTBurnParams.postData = wp->postData;
			gMTBurnParams.precentComplete = 0;
			printf("Creating thread\n");
			wp->postData = NULL;
			//Jacky.Yang 13-Aug-2008, add LED blinking when the user upgrade firmware.
			system("echo 2 > /dev/gpio2");
			
			//Jacky.Yang 7-Nov-2008, f/w upgrade prcess, delete other process.
			doSystem("killall upnpd");
			doSystem("killall udhcpc");	
			doSystem("cd /root/mtlk/etc/; /root/mtlk/etc/mtlk_wps_cmd.tcl stop");
			doSystem("killall wpa_supplicant");
			doSystem("kill `ps |grep 'mtlk_pbc_reboot.sh'|grep -v 'grep'|awk '{print $1}'`");
			doSystem("kill `ps |grep 'WPS_PBC.sh'|grep -v 'grep'|awk '{print $1}'`");
			doSystem("echo 1 > /var/firmware_upgrading"); //Ricky add, I'll also use this flag.
			
			pthread_create( &gBurnThread, NULL, &MT_BurnAndVerifyImage, NULL);
		}
		
	}
    
#endif
	printf("BurnImage: gMTBurnParams.precentComplete:%d\n", gMTBurnParams.precentComplete);
	doSystem("echo \"BurnImage: gMTBurnParams.precentComplete:%d(pid=%d)\" > /dev/console", gMTBurnParams.precentComplete, getpid());
	doSystem("ps | grep webs");
	if (wp->postData)
	{
		free(wp->postData);
		wp->postData = NULL;
		printf("Freeing wp postdata\n");
	}

    // Redirect to the status page
	sprintf(redirectPage,T("%s"),statusPageName);
	//printf("target file '%s' redirct to '%s' (%s)\n",targetFile,redirectPage,statusPageName);
	MT_WriteRedirectScript(wp,redirectPage);
    websFooter(wp);
    websDone(wp, 200);
}

/*
 * Dump Firmware directly from physical MTD NOR flash partitions in 4KB streaming chunks.
 * Zero RAM overhead: direct bit-exact streaming over HTTP.
 */
void formDumpFirmware(webs_t wp, char_t * path, char_t * query)
{
	FILE *fp = NULL;
	char buff[4096];
	size_t n = 0;
	char_t *dumpType = websGetVar(wp, T("type"), T("full"));
	const char *devPath = "/dev/mtdblock0";
	const char *outName = "WAP610N_full_flash_dump.bin";

	if (strcmp(dumpType, "kernel") == 0) {
		devPath = "/dev/mtdblock1";
		outName = "WAP610N_kernel_rootfs_dump.bin";
	} else if (strcmp(dumpType, "config") == 0) {
		devPath = "/dev/mtdblock2";
		outName = "WAP610N_config_dump.bin";
	}

	fp = fopen(devPath, "rb");
	if (!fp) {
		if (strcmp(dumpType, "kernel") == 0) devPath = "/dev/mtd1";
		else if (strcmp(dumpType, "config") == 0) devPath = "/dev/mtd2";
		else devPath = "/dev/mtd0";
		fp = fopen(devPath, "rb");
	}

	if (!fp) {
		websError(wp, 500, T("Cannot open MTD device for dumping"));
		return;
	}

	websWrite(wp, T("HTTP/1.0 200 OK\r\n"));
	websWrite(wp, T("Server: %s\r\n"), WEBS_NAME);
	websWrite(wp, T("Pragma: no-cache\r\n"));
	websWrite(wp, T("Cache-control: no-cache\r\n"));
	websWrite(wp, T("Content-Type: application/octet-stream\r\n"));
	websWrite(wp, T("Content-Disposition: attachment; filename=\"%s\"\r\n"), outName);
	websWrite(wp, T("\r\n"));

	while ((n = fread(buff, 1, sizeof(buff), fp)) > 0) {
		if (websWriteBlock(wp, buff, n) < 0) break;
	}
	fclose(fp);
	websDone(wp, 200);
}

