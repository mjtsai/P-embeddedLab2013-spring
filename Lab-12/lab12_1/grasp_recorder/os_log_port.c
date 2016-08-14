/*
 * os_log.c port for uC/OS-II on OpenRISC.
 */

#include "os_log_port.h"
#include "task.h"


#define logEnterCriticalSection()       taskENTER_CRITICAL()
#define logExitCriticalSection()        taskEXIT_CRITICAL()


#define logTaskName(task)               ((task)->pcTaskName)
#define logTaskPriority(task)           ((task)->uxPriority)
#define logMutexName(mutex)             ""
