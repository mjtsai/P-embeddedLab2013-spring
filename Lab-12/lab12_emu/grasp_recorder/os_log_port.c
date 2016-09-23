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

#define logResetPort()                  ((void) 0)
#define logGetTime()                    xTaskGetTickCount()
#define logTaskIsIdleTask(task)         (0)
#define logTaskStarted(time, task) \
  logFilePrint(fp, "plot %d jobResumed job%p.%d\n", \
    time, \
    task, \
    currentJobForTask[logTaskPriority(task)]);



/***/
#define LOG_ASSERT(...)             ((void) 0) 
#define logStringPrint(s, ...)      sprintf(s, __VA_ARGS__)
#define logFilePrint(fp, ...)       printf(__VA_ARGS__)
#define logErrorPrint(format, ...)  printf(format, __VA_ARGS__)
#define logFileFlush(fp)            ((void) 0)

static int fileOpened = 0;

LogFileHandle logFileOpen(const char* path, const char* mode){
    if (fileOpened == 0) {
        fileOpened = 1;
        return (LogFileHandle)1;
    } else {
        return (LogFileHandle)0;
    }
}

#define logFileClose(fp)            ((void) 0)
#define logAfterPrint(fp)           ((void) 0)

