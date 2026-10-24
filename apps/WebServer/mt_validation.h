#ifndef _MT_VALIDATION_H_
#define _MT_VALIDATION_H_

#include "mt_api.h"

extern void MT_DefineValidationFuncs(void);
extern int MT_ASPreturn(webs_t wp, int val);
extern int isInteger(const char *str);

#endif /* _MT_VALIDATION_H_ */
