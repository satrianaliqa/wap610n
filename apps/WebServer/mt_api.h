#ifndef _MT_API_H_
#define _MT_API_H_

#include "webs.h"

#define MT_MAX_PARAM_VALUE_LENGTH 512
#define MT_MAX_PARAM_NAME_LENGTH  128
#define MT_MAX_PATH_LENGTH        254
#define MT_MAX_LINE_LENGTH        512
#define MT_CONFIG_FILE_SIG_LEN    25

#define MT_DONT_SET_FIRST_VALUE   0
#define MT_SET_FIRST_VALUE        1

#define MTSECURITY_USER_NAME_VAR_NAME   "AdminUsername"
#define MTSECURITY_USER_PASSWD_VAR_NAME "AdminPassword"
#define MTSECURITY_INACTIVE_TIMEOUT     "AuthenticationTimeout"
#define MTSECURITY_ADMINPASS_OPEN       "AdminPasswordOpen"

typedef enum {
	idle_e = 0,
	done_e = 1,
	error_e = 2,
	uploading_e = 3,
	checksum_e = 4,
	burning_e = 5,
	verifying_e = 6
} MT_BurnState_e;

typedef int (*MTParamChangeCbFunc_t)(char_t *paramName, char_t *paramValue);

typedef struct {
	char_t fName[MT_MAX_PATH_LENGTH];
	char_t errorMessage[MT_MAX_PARAM_VALUE_LENGTH];
	int imageSize;
	int burnState;
	int precentComplete;
	char_t *fileContent;
	char_t *postData;
	int retryCount;
} MTImageBurnParams_t;

typedef struct {
	char_t *name;
	char_t *string;
	unsigned int size;
} MTmanagedStr_t;

typedef struct {
	char_t name[MT_MAX_PARAM_NAME_LENGTH];
	char_t confFile[MT_MAX_PATH_LENGTH];
	MTmanagedStr_t value;
	MTmanagedStr_t firstValue;
	unsigned char hasChanged;
	unsigned char used;
	unsigned char internalOnly;
	unsigned char initialized;
} MTparam_t;

typedef struct {
	char_t paramName[MT_MAX_PARAM_NAME_LENGTH];
	MTParamChangeCbFunc_t pCbFunc;
} MTParamChangeCb_t;

extern char MTretValue[MT_MAX_PARAM_VALUE_LENGTH];
extern MTImageBurnParams_t gMTBurnParams;
extern char_t MT_WebConfDir[MT_MAX_PATH_LENGTH];
extern char_t MT_WebPlatform[MT_MAX_PATH_LENGTH];
extern char_t MT_WebRootDir[MT_MAX_PATH_LENGTH];
extern char_t MT_WebSavedConfigsDir[MT_MAX_PATH_LENGTH];
extern char_t MT_WebTmpDir[MT_MAX_PATH_LENGTH];
extern char_t MT_WebVirtualTmpDir[MT_MAX_PATH_LENGTH];
extern char_t MT_WebWPADir[MT_MAX_PATH_LENGTH];
extern int MT_CardCount;
extern int MT_CardIndex;
extern int MT_CardIndexToWrite;
extern int MT_DebugLevel;
extern int websPort;
extern int updateLinkStatusTag;
extern int wpsArrayIndex;
extern int wpsErrorCount;
extern int wpsStatusArray[10];

extern int MT_Set_Param(char_t *name, char_t *val, int flag);
extern int MT_Get_Param(char_t *name, char_t *buf, int maxlen);
extern void MT_DefineAPIFuncs(void);
extern int MT_Read_All_Params(void);
extern int MT_Free_Params(void);
extern int MT_InitBurnParams(MTImageBurnParams_t *pParams);
extern int MT_GetConfigHeader(char_t *buf, int maxLen);
extern int MT_WriteConfFiles(int argc, char_t **argv);
extern int MT_SaveParamsToFile(char_t *fileName, char_t *caption, char_t *toolTip);
extern int MT_CommitTo(int dest, char_t *confName);
extern void commitToFlash(webs_t wp, char_t **path, char_t **query);
extern int loadConfigFromFile(char_t *fileName);
extern int MT_PrintFileToWeb(webs_t wp, char_t *fileName);
extern int MT_Exec(webs_t wp, char_t *cmd, char_t *retBuf, int maxLen);
extern int MT_CollectExecWebParams(webs_t wp, char_t *query);
extern int MT_ParseArguments(int argc, char_t **argv);
extern int MT_ParseOnLoad(void);
extern int MT_PreParsePage(webs_t wp, char_t *path);
extern int MT_PostParsePage(webs_t wp, char_t *path);
extern int MT_ReadCountryDB(void);
extern int MT_ReadDatabase(void);
extern int Tokenizer(char_t *str, char_t *delim, int index, char_t *outBuf, int maxLen);
extern void MT_WriteRedirectScript(webs_t wp, char_t *redirectPage);
extern void BurnImage(webs_t wp, char_t *path, char_t *query);
extern void upldForm(webs_t wp, char_t *path, char_t *query);
extern void sendFileForm(webs_t wp, char_t *path, char_t *query);
extern char_t *WebImageUpdateState(void);
extern char_t *WebImageUpdateProgress(void);
extern char_t *WebImageUpdateErr(void);

#endif /* _MT_API_H_ */

