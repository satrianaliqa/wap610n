#ifndef _MT_API_H_
#define _MT_API_H_

#define MT_MAX_PARAM_VALUE_LENGTH 512
#define MT_DONT_SET_FIRST_VALUE 0
#define MT_SET_FIRST_VALUE 1

extern char MTretValue[MT_MAX_PARAM_VALUE_LENGTH];
extern int MT_Set_Param(char *name, char *val, int flag);
extern int MT_Get_Param(char *name, char *buf, int maxlen);
extern void MT_DefineAPIFuncs(void);
extern void MT_DefineValidationFuncs(void);

#endif
