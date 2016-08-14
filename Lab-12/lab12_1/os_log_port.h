#ifndef OS_LOG_PORT_H
#define OS_LOG_PORT_H

#ifndef  OS_MASTER_FILE
#include <FreeRTOS.h>
#endif

#if OS_LOG_EN > 0

/*
 * Redefine the number of semaphores to be logged, to avoid annoying errors.
 */
 
#undef OS_LOG_MAX_NUM_SEMAPHORES
#define OS_LOG_MAX_NUM_SEMAPHORES OS_MAX_EVENTS

#if OS_RELTEQ_EN > 0
#undef OS_LOG_MAX_NUM_SERVERS
#define OS_LOG_MAX_NUM_SERVERS OS_RELTEQ_MAX_SERVERS
#endif

/*
 * Define the "inline" cimpiler directive, as it may not be supported by the compiler.
 */
 
#define LOG_INLINE inline

/*
 * Simple types
 */

typedef INT32U LogInt32U;
typedef INT8U LogBool;
typedef INT32U LogTime;

/*
 * Task Control Block (TCB) structure
 */
 
typedef OS_TCB LogTask;

/*
 * Semaphore Control Block structure
 */
 
typedef OS_EVENT LogSemaphore;

/*
 * Mutex Control Block structure
 */

typedef OS_EVENT LogMutex;

/*
 * Buffer Control Block structure
 */

typedef struct {
  char* name;
} LogBuffer;

/*
 * Server Control Block structure
 */

#if OS_TEV_EN > 0
  #define LogServer OS_SCB
#elif OS_RELTEQ_EN > 0
  #define LogServer struct RelteqServer
#endif


/*
 * File handle type. All methods for handling files are port specific.
 */

typedef void* LogFileHandle;

/*
 * Checks if an assertion is ture. Of it is false, then it prints a message and exits.
 *
 * void ASSERT(bool condition, char* format, ...)
 */
 
#ifndef ASSERT
  #if OS_ARG_CHK_EN > 0
    #define ASSERT( b , ...) \
      if ( !(b) ) { \
        printf( "ASSERT @ %s # %d, ", __FILE__, __LINE__); \
        printf(__VA_ARGS__); \
        printf("\n"); \
        logPrint("error_trace.log");\
        exit(0);\
      };
  #else
    #define ASSERT(...) ((void) 0) 
  #endif
#endif /* ASSERT */

/*
 * Return the current time
 *
 * LogInt32U logGetTime(void)
 */
 
#define logGetTime() OSTimeGet()

/*
 * Return the priority of a task
 *
 * int logTaskPriority(LogTask* task)
 */
 
#define logTaskPriority(task) ((task)->OSTCBPrio)

/*
 * Return the name of a task
 *
 * char* logTaskName(LogTask* task)
 */
 
#define logTaskName(task) ((task)->OSTCBTaskName)

/*
 * Return if the task is the idle task
 *
 * int logTaskIsIdleTask(LogTask* task)
 */
 
#define logTaskIsIdleTask(task) ((task)->OSTCBPrio == OS_TASK_IDLE_PRIO)

/*
 * Perform some custom action after a task is started.
 *
 * void logTaskStarted(LogTime time, LogTask* task)
 */ 
 
#define logTaskStarted(time, task) \
  logFilePrint(fp, "plot %d jobResumed job%p.%d\n", \
    time, \
    task, \
    currentJobForTask[logTaskPriority(task)]);

/*
 * Return the name of a mutex (tupe: char*) 
 *
 * char* logMutexName(LogMutex* mutex) 
 */
 
#define logBufferName(buffer) (((buffer))->name)
 
/*
 * Return the name of a mutex (tupe: char*) 
 *
 * char* logMutexName(LogMutex* mutex) 
 */
 
#define logMutexName(mutex) ((mutex)->OSEventName)

/*
 * Return the name of a semaphore
 *
 * char* logSemaphoreName(LogSemaphore* sempahore)
 */
 
#define logSemaphoreName(semaphore) ((semaphore)->OSEventName)

/*
 * Return the current value of the semaphore.
 *
 * int logSemaphoreCount(LogSemaphore* semaphore)
 */

#define logSemaphoreCount(semaphore) ((semaphore)->OSEventCnt)

/*
 * Return the priority of a server.
 *
 * int logTaskPriority(LogServer* server)
 */
 
#if OS_TEV_EN > 0 && OS_TEV_SERVER_EN > 0
  #define logServerPriority(server) ((server)->OSServerPrio)
#elif OS_RELTEQ_EN > 0 && OS_RELTEQ_HSF_EN > 0
  #define logServerPriority(server) ((server)->ServerPriority)
#endif 

/*
 * Return the name of a server.
 *
 * char* logServerName(LogServer* server)
 */
 
#if OS_TEV_EN > 0 && OS_TEV_SERVER_EN > 0
  #define logServerName(server) ((server)->OSSCBServerName)
#elif OS_RELTEQ_EN > 0 && OS_RELTEQ_HSF_EN > 0
  #define logServerName(server) ((server)->ServerName)
#endif 

/*
 * Return the budget of a server.
 *
 * char* logServerName(LogServer* server)
 */

#if OS_TEV_EN > 0 && OS_TEV_SERVER_EN > 0
  #define logServerBudget(server) ((server)->OSServerBudget)
#elif OS_RELTEQ_EN > 0 && OS_RELTEQ_HSF_EN > 0
  #define logServerBudget(server) ((server)->ServerBudget)
#endif

/*
 * Return if a server is active or not.
 *
 * char* logServerName(LogServer* server)
 */

#if OS_TEV_EN > 0 && OS_TEV_SERVER_EN > 0
  #define logServerIsActive(server) ((server)->OSServerFlags != OS_TEV_SERVER_FLAG_UNUSED)
#elif OS_RELTEQ_EN > 0 && OS_RELTEQ_HSF_EN > 0
  #define logServerIsActive(server) ((server)->ServerType != kRelteqServerNone)
#endif

/*
 * Enter or exit a critical section. Note that during a critical section interrupts
 * are disabled.
 *
 * void logEnterCriticalSection(void)
 * void logExitCriticalSection(void)
 */
 
#define logEnterCriticalSection() OS_ENTER_CRITICAL()
#define logExitCriticalSection() OS_EXIT_CRITICAL()

/*
 * Lock and unlock a scheduler. Note that during the time the scheduler is locked
 * interrupts can arrive.
 *
 * void logLockScheduler(void)
 * void logUnlockScheduler(void)
 */
 
#define logLockScheduler() OSSchedLock()
#define logUnlockScheduler() OSSchedUnlock()

/*
 * Reset the port. Called at the end of logReset().
 *
 * void logResetPort(void)
 */
 
#define logResetPort() ((void) 0)

/*
 * This method is called after a trace is printed. This is optional.
 *
 * void logAfterPrint(void)
 */

extern void terminateSimulation(void);

#define logAfterPrint() terminateSimulation()

/*
 * Return the pointer to the current task, or NULL if the it is an ISR.
 *
 * LogTask* logCurrentTask(void);
 */

#if OS_LOG_ISR_EN > 0
  #define logCurrentTask() (OSIntNesting > 0 ? NULL : OSTCBCur)
#else
  #define logCurrentTask() OSTCBCur
#endif

/*
 * Print to a string.
 *
 * void logString(char* string, char* format, ...)
 */
 
#define logStringPrint(s, ...) sprintf(s, __VA_ARGS__)

/*
 * Open a file for writing and return a handle to it. This handle will later be used
 * by other file handling methods.
 *
 * LogFileHandle logFileOpen(const char* path, const char* mode)
 */
 
LogFileHandle logFileOpen(const char* path, const char* mode);

/*
 * Write a string to a file, which was previousely returned by logFileOpen().
 *
 * void logFilePrint(LogFileHandle fp, char* format, ...)
 */
 
#define logFilePrint(fp, ...) printf(__VA_ARGS__)

/*
 * Flush any buffered data to a file.
 *
 * void logFileFlush(LogFileHandle fp)
 */
 
#define logFileFlush(fp) ((void) 0)

/*
 * Close a file with a given handle, which was previousely returned by logFileOpen().
 * 
 * int logFileClose(LogFileHandle fp)
 */
 
int logFileClose(LogFileHandle fp);

/*
 * Print a string to the standard error.
 *
 * void logErrorPrint(char* format, ...)
 */
 
#define logErrorPrint(format, ...) printf(format, __VA_ARGS__)


#endif /* OS_LOG_EN */

#endif /* OS_LOG_PORT_H */
