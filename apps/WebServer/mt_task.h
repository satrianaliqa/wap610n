#ifndef _MT_TASK_H_
#define _MT_TASK_H_

#include <time.h>
#include "webs.h"

typedef int (*MT_Task_Func_Type)(void);

typedef struct {
	int m_interval;
	MT_Task_Func_Type m_taskFunc;
	time_t m_lastCallTime;
} MTtask_t;

typedef unsigned int uint_t;

typedef struct {
	time_t m_lastGrabTime;
	uint_t m_lastFileIndex;
	MTtask_t *m_pTask;
} MTStatistics_t;

extern void MT_TaskScheduler(void);
extern void MT_PrepareStatisticsTask(void);
extern int websStatisticsHandler(webs_t wp, char_t *urlPrefix, char_t *webDir, int arg, char_t *url, char_t *path, char_t *query);

#endif /* _MT_TASK_H_ */
