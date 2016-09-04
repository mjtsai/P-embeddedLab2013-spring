#ifndef OS_LOG_H
#define OS_LOG_H

#include "os_log_port.h"  

#ifndef OS_LOG_EN
  #error  "Missing OS_LOG_EN: Enable (1) or Disable (0) the Grasp trace recorder."
#elif   OS_LOG_EN > 0

#ifndef OS_LOG_MAX_NUM_EVENTS
  #error  "Missing OS_LOG_MAX_NUM_EVENTS: Determines the max number of events in the log."
#endif

#ifndef OS_LOG_MAX_MESSAGE_LENGTH
  #error  "Missing OS_LOG_MAX_MESSAGE_LENGTH: Determines the max length of a text message in the log (in chars)."
#endif

#ifndef OS_LOG_MAX_NUM_MUTEXES
  #error  "Missing OS_LOG_MAX_NUM_MUTEXES: Determines the max number of mutexes in the system."
#endif

#ifndef OS_LOG_MAX_NUM_MUTEXES
  #error  "Missing OS_LOG_MAX_NUM_MUTEXES: Determines the max number of mutexes in the system."
#endif    

/*
 * Initialize the logger. It creates the internal data structures for tasks, semaphores, ... .
 * This method must be called before any other log method. Also, this method should be called
 * only once.
 */
 
void logInit(void);

/*
 * Reset the logged trace. Unlike logInit(), this method only clears the events which have been
 * logged so far. It does not delete the internal data strucutres for tasks, semaphores, ... .
 * logReset() can be called multiple times.
 */
 
void logReset(void);

/*
 * Resets the offset of the inital event in the trace. In effect, the moment this method is called
 * will have time 0 in the trace. This methods is useful if the actual trace starts after some
 * initialization time.
 */
 
void logResetCycleCountOffset(void);

/*
 * These methods are called before and after the trace is written to a file, respectively. They
 * allow to add custom text to a trace, e.g. task parameters visualized in the trace or other
 * statistics.
 */
 
void logPrologueFunction(void (*f)(LogFileHandle fp));
void logEpilogueFunction(void (*f)(LogFileHandle fp));

/*
 * Print the trace to a file with a given file name.
 */
 
void logPrint(char* fileName);

/*
 * Log the entry and exit from an Interrupt Service Routine (ISR). Note that only unsafe methods
 * are provided, because an ISR is entered and exited within a critical section.
 */
 
#if OS_LOG_ISR_EN > 0
void logISREnterUnsafe(void);
void logISRExitUnsafe(void);
#endif

/*
 * Log other events. Many events are provided in two flavors: thread safe and unsafe. The unsafe
 * methods avoid entering and exiting a critical section to improve performance. However, make sure
 * to call the unsafe methods ONLY inside of a critical section.
 */
 
void logTaskCreated(LogTask* task);
void logTaskDeleted(LogTask* task);
void logTaskArrival(LogTask* task);
void logTaskArrivalUnsafe(LogTask* task);
void logTaskStart(LogTask* task);
void logTaskPreemption(LogTask* taskIn, LogTask* taskOut);
void logTaskCompletion(LogTask* task);
void logTaskMissedDeadline(LogTask* task);

#if OS_LOG_BUFFER_EN > 0
void logBufferCreated(LogBuffer* buffer);
void logBufferDeleted(LogBuffer* buffer);
void logBufferResize(LogBuffer* buffer, int size);
void logBufferRead(LogBuffer* buffer);
void logBufferWrite(LogBuffer* buffer, int data);
void logBufferWrite2(LogBuffer* buffer, int data1, int data2);
void logBufferDrop(LogBuffer* buffer, int index);  
#endif

#if OS_LOG_MUTEX_EN > 0
void logMutexCreated(LogMutex* mutex);
void logMutexDeleted(LogMutex* mutex);
void logMutexAcquired(LogMutex* mutex, LogTask* task);
void logMutexAcquiredUnsafe(LogMutex* mutex, LogTask* task);
void logMutexReleased(LogMutex* mutex, LogTask* task);
void logMutexReleasedUnsafe(LogMutex* mutex, LogTask* task);
#endif

#if OS_LOG_SEMAPHORE_EN > 0
void logSemaphoreCreated(LogSemaphore* semaphore);
void logSemaphoreDeleted(LogSemaphore* semaphore);
void logSemaphoreAcquired(LogSemaphore* semaphore, LogTask* task);
void logSemaphoreAcquiredUnsafe(LogSemaphore* semaphore, LogTask* task);
void logSemaphoreSuspended(LogSemaphore* semaphore, LogTask* task);
void logSemaphoreSuspendedUnsafe(LogSemaphore* semaphore, LogTask* task);
void logSemaphoreReleased(LogSemaphore* semaphore, LogTask* task);
void logSemaphoreReleasedUnsafe(LogSemaphore* semaphore, LogTask* task);
#endif

#if (OS_RELTEQ_EN > 0 && OS_RELTEQ_HSF_EN > 0) || (OS_TEV_EN > 0 && OS_TEV_SERVER_EN > 0)
void logServerCreated(LogServer* server);
void logServerDeleted(LogServer* server);
void logServerReplenished(LogServer* server, int amount);
void logServerResumed(LogServer* server);
void logServerPreempted(LogServer* server);
void logServerDepleted(LogServer* server);
#endif

void logMessage(char* format, void* arg1, void* arg2);
void logMessageUnsafe(char* format, void* arg1, void* arg2);
void logMessageWithColor(char* color, char* format, void* arg1);
void logMessageWithColorUnsafe(char* color, char* format, void* arg1);

void logCommand(char* format, void* arg1, void* arg2);
void logCommandUnsafe(char* format, void* arg1, void* arg2);
void logCustomEvent(char* format, void* arg1, void* arg2);
void logCustomEventUnsafe(char* format, void* arg1, void* arg2);

#endif /* OS_LOG_EN */

#endif /* OS_LOG_H */
