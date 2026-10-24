#ifndef _UTILS_H_
#define _UTILS_H_

extern int openDebug;
extern void debug_printf(const char *fmt, ...);
extern int doSystem(const char *fmt, ...);
extern void saveOldWebPid(int pid, const char *page);
extern void waitRedirectPage(void);
extern void stopWPS_PBC(void);
extern void startWPS_PBC(void);
extern int checkLoadDefault(void);
extern void submitToFlash(void);
extern void formDefineUtils(void);
extern int lang_init(void);

#endif /* _UTILS_H_ */

