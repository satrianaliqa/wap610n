#ifndef _HNAP_INTERFACES_H_
#define _HNAP_INTERFACES_H_

#include "webs.h"

#define COMMIT_SYS   "sys"
#define COMMIT_WLAN0 "wlan0"

typedef enum {
	srt_restart = 0,
	srt_reboot = 1,
	srt_servicerestart = 2
} system_restart_types;

extern int websHNAPHandler(webs_t wp, char_t *urlPrefix, char_t *webDir, int arg, char_t *url, char_t *path, char_t *query);
extern int device_set_password(char *password_new);
extern int device_set_string_value(char *name, char *value);
extern int hnap_commit_changes(int argc, char **argv, char *confType);
extern int system_reset(system_restart_types reset_type);
extern unsigned int system_sleep(unsigned int seconds);
extern int device_is_ready(void);

#endif /* _HNAP_INTERFACES_H_ */
