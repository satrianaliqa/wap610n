#ifndef _MANAGEMENT_H_
#define _MANAGEMENT_H_

extern void formDefineManagement(void);
extern void websInitRealm(void);
extern void childFinishAndExit(void);
extern void syncRootShadowPassword(const char *plainPass);
extern void websPreserveCustomSysConf(const char *wirelessMgmtOverride);

#endif /* _MANAGEMENT_H_ */
