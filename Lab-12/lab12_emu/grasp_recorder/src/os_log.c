#include "os_log.h"

#if OS_LOG_EN > 0

/*
 * Include the port specific part of os_log.
 */
 
#include "os_log_port.c"

/* 
 * Local types
 */

typedef enum {
  kLogEventTaskPreemption = 0,
  kLogEventTaskArrival,
  kLogEventTaskStart,
  kLogEventTaskCompletion,
  kLogEventBufferResize,
  kLogEventBufferPop,
  kLogEventBufferPush,
  kLogEventBufferPush2,
  kLogEventBufferDrop,
  kLogEventMutexAcquired,
  kLogEventMutexReleased,
  kLogEventSemaphoreAcquired,
  kLogEventSemaphoreSuspended,
  kLogEventSemaphoreReleased,
  kLogEventServerReplenished,
  kLogEventServerResumed,
  kLogEventServerPreempted,
  kLogEventServerDepleted,
  kLogEventISREnter,
  kLogEventISRExit,
  kLogEventCommand,
  kLogEventCustomEvent,
  kLogEventMessage,
  kLogEventMessageWithColor
} LogEventType;

typedef struct {
  LogTime time;
  LogEventType event;
  void* arg1;
  void* arg2;
  void* arg3;
} EventInfo;

/*
 * Local functions
 */
 
LogInt32U logGetMaxC(LogTask* task);
LogInt32U logGetMissedDeadlines(void);   
void logEvent(LogEventType event, void* arg1, void* arg2, void* arg3);
void printLogFile(char* logFileName);

/*
 * Global variables
 */

static EventInfo eventLog[OS_LOG_MAX_NUM_EVENTS];
static int eventIndex;
static int numEvents;
static int missedDeadlines;

/*
 * A boolean indicating if the cycle count offset was already dumped. Makes 
 * sure the measurement messages are properly synced with other events.
 */
 
LogBool logCycleCountOffset = 0;

/*
 * Since ucos does not store a list of used synchronizers (such as mutexes,
 * semaphores, ...) or events in general (it only stores a list of free events),
 * we need to store the used synchronizers here.
 */
 
static LogTask* tasks[OS_LOG_MAX_NUM_TASKS];

#if OS_LOG_MUTEX_EN > 0
static LogMutex* mutexes[OS_LOG_MAX_NUM_MUTEXES];
#endif

#if OS_LOG_SEMAPHORE_EN > 0
static LogSemaphore* semaphores[OS_LOG_MAX_NUM_SEMAPHORES];
#endif

#if OS_LOG_BUFFER_EN > 0
static LogBuffer* buffers[OS_LOG_MAX_NUM_BUFFERS];
#endif

#if (OS_RELTEQ_EN > 0 && OS_RELTEQ_HSF_EN > 0) || (OS_TEV_EN > 0 && OS_TEV_SERVER_EN > 0)
static LogServer* servers[OS_LOG_MAX_NUM_SERVERS];
#endif

/*
 * Optional functions, allowing to log additional stuff at the beginning and
 * the end of a simulation.
 */
 
static void (*prologueFunction)(LogFileHandle fp);
static void (*epilogueFunction)(LogFileHandle fp);

/*
 * logInit
 */


void logReset(void) {
  eventIndex = 0;
  numEvents = 0;
  missedDeadlines = 0;
    
  logResetPort();
}

void logInit(void) {
#if OS_LOG_MUTEX_EN > 0 || OS_LOG_SEMAPHORE_EN > 0
  int i;
#endif

  logReset();
  
  // clear all mutexes and other events
#if OS_LOG_MUTEX_EN > 0
  for (i = 0; i < OS_LOG_MAX_NUM_MUTEXES; i++) {
    mutexes[i] = 0;
  }
#endif

#if OS_LOG_SEMAPHORE_EN > 0
  for (i = 0; i < OS_LOG_MAX_NUM_SEMAPHORES; i++) {
    semaphores[i] = 0;
  }
#endif
}

void logResetCycleCountOffset() {
#if OS_PROFILING_EN > 0
	profilePoint("offset");
#endif
  logCycleCountOffset = 1;
}

void logPrologueFunction(void (*f)(LogFileHandle fp)) {
  prologueFunction = f;
}

void logEpilogueFunction(void (*f)(LogFileHandle fp)) {
  epilogueFunction = f;
}

/*
 * Generic event logging function.
 */

LOG_INLINE void logEvent(LogEventType event, void* arg1, void* arg2, void* arg3) {
#if OS_CRITICAL_METHOD == 3 
    OS_CPU_SR     cpu_sr = 0;
#endif
  logEnterCriticalSection();
  EventInfo* e = &eventLog[eventIndex];
  e->time = logGetTime();
  eventIndex = (eventIndex+1) % OS_LOG_MAX_NUM_EVENTS;
  if (eventIndex < OS_LOG_MAX_NUM_EVENTS) {
    numEvents++;
  }
  logExitCriticalSection();

  e->event = event;
  e->arg1 = arg1;
  e->arg2 = arg2;
  e->arg3 = arg3;  
}

LOG_INLINE void logEventUnsafe(LogEventType event, void* arg1, void* arg2, void* arg3) {
  EventInfo* e = &eventLog[eventIndex];
  e->time = logGetTime();
  eventIndex = (eventIndex+1) % OS_LOG_MAX_NUM_EVENTS;
  if (eventIndex < OS_LOG_MAX_NUM_EVENTS) {
    numEvents++;
  }

  e->event = event;
  e->arg1 = arg1;
  e->arg2 = arg2;
  e->arg3 = arg3;  
}

/*
 * Task
 */

void logTaskCreated(LogTask* task) {
  /*  find a free spot in the semaphores array */
  int i = 0;
  while (tasks[i] != 0 && i < OS_LOG_MAX_NUM_TASKS) i++;

  /*  check if found a spot, and then allocate it by the mutex */
  LOG_ASSERT(i < OS_LOG_MAX_NUM_TASKS, "Not enough tasks (max=%d)", OS_LOG_MAX_NUM_TASKS);
  if (i < OS_LOG_MAX_NUM_TASKS) {
    tasks[i] = task;
  }
}

void logTaskDeleted(LogTask* task) {
  /*  find the mutex in the semaphores array */
  int i = 0;
  while (tasks[i] != task && i < OS_LOG_MAX_NUM_TASKS) i++;
  
  /*  check if found the mutex, and then clear it */
  if (i < OS_LOG_MAX_NUM_TASKS) {
    tasks[i] = 0;
  }
}

void logTaskArrival(LogTask* task) {
  if (task != NULL) {
    logEvent(kLogEventTaskArrival, task, NULL, NULL);
  }
}

void logTaskArrivalUnsafe(LogTask* task) {
  if (task != NULL) {
    logEventUnsafe(kLogEventTaskArrival, task, NULL, NULL);
  }
}

void logTaskStart(LogTask* task) {
  if (task != NULL) {
    logEvent(kLogEventTaskStart, task, NULL, NULL);
  }
}

void logTaskPreemption(LogTask* taskIn, LogTask* taskOut) {
  logEvent(kLogEventTaskPreemption, taskIn, taskOut, NULL);
}

void logTaskCompletion(LogTask* task) {
  if (task != NULL) {
    logEvent(kLogEventTaskCompletion, task, NULL, NULL);
  }
}

void logTaskMissedDeadline(LogTask* task) {
  logMessage("Task '%s' missed its deadline", logTaskName(task), NULL);
  missedDeadlines++;
}

/*
 * Buffer
 */
 
#if OS_LOG_BUFFER_EN > 0

void logBufferCreated(LogBuffer* buffer) {
  /*  find a free spot in the semaphores array */
  int i = 0;
  while (buffers[i] != 0 && i < OS_LOG_MAX_NUM_BUFFERS) i++;
  
  /*  check if found a spot, and then allocate it by the sempahore */
  LOG_ASSERT(i < OS_LOG_MAX_NUM_BUFFERS, "Not enough bufers (max=%d)", OS_LOG_MAX_NUM_BUFFERS);
  if (i < OS_LOG_MAX_NUM_BUFFERS) {
    buffers[i] = buffer;
  }
}

void logBufferDeleted(LogBuffer* buffer) {
  /*  find the mutex in the semaphores array */
  int i = 0;
  while (buffers[i] != buffer && i < OS_LOG_MAX_NUM_BUFFERS) i++;
  
  /*  check if found the mutex, and then clear it */
  if (i < OS_LOG_MAX_NUM_BUFFERS) {
    buffers[i] = 0;
  }
}

void logBufferResize(LogBuffer* buffer, int size) {
  logEvent(kLogEventBufferResize, buffer, (void*)size, NULL);
}

void logBufferRead(LogBuffer* buffer) {
  logEvent(kLogEventBufferPop, buffer, NULL, NULL);
}

void logBufferWrite(LogBuffer* buffer, int data) {
  logEvent(kLogEventBufferPush, buffer, (void*)data, NULL);
}

void logBufferWrite2(LogBuffer* buffer, int data1, int data2) {
  logEvent(kLogEventBufferPush2, buffer, (void*)data1, (void*)data2);
}

void logBufferDrop(LogBuffer* buffer, int index) {
  logEvent(kLogEventBufferDrop, buffer, (void*)index, NULL);
}

#endif

/*
 * Mutex
 *
 * Since mutexes can be created and deleted during runtime, we can 
 * get "gaps" in the mutexes array, hence we need to search the array
 * when creating and deleting mutexes.
 */

#if OS_LOG_MUTEX_EN > 0

void logMutexCreated(LogMutex* mutex) {
  /*  find a free spot in the mutexes array */
  int i = 0;
  while (mutexes[i] != 0 && i < OS_LOG_MAX_NUM_MUTEXES) i++;
  
  /*  check if found a spot, and then allocate it by the mutex */
  LOG_ASSERT(i < OS_LOG_MAX_NUM_MUTEXES, "Not enough mutexes (max=%d)", OS_LOG_MAX_NUM_MUTEXES);
  if (i < OS_LOG_MAX_NUM_MUTEXES) {
    mutexes[i] = mutex;
  }
}

void logMutexDeleted(LogMutex* mutex) {
  /*  find the mutex in the mutexes array */
  int i = 0;
  while (mutexes[i] != mutex && i < OS_LOG_MAX_NUM_MUTEXES) i++;
  
  /*  check if found the mutex, and then clear it */
  if (i < OS_LOG_MAX_NUM_MUTEXES) {
    mutexes[i] = 0;
  }
}

void logMutexAcquired(LogMutex* mutex, LogTask* task) {
  logEvent(kLogEventMutexAcquired, mutex, task, NULL);
}

void logMutexReleased(LogMutex* mutex, LogTask* task) {
  logEvent(kLogEventMutexReleased, mutex, task, NULL);
}

void logMutexAcquiredUnsafe(LogMutex* mutex, LogTask* task) {
  logEventUnsafe(kLogEventMutexAcquired, mutex, task, NULL);
}

void logMutexReleasedUnsafe(LogMutex* mutex, LogTask* task) {
  logEventUnsafe(kLogEventMutexReleased, mutex, task, NULL);
}

#endif

/*
 * Semaphore
 */

#if OS_LOG_SEMAPHORE_EN > 0

void logSemaphoreCreated(LogSemaphore* semaphore) {
  /*  find a free spot in the semaphores array */
  int i = 0;
  while (semaphores[i] != 0 && i < OS_LOG_MAX_NUM_SEMAPHORES) i++;
  
  /*  check if found a spot, and then allocate it by the sempahore */
  LOG_ASSERT(i < OS_LOG_MAX_NUM_SEMAPHORES, "Not enough semaphores (max=%d)", OS_LOG_MAX_NUM_SEMAPHORES);
  if (i < OS_LOG_MAX_NUM_SEMAPHORES) {
    semaphores[i] = semaphore;
  }
}

void logSemaphoreDeleted(LogSemaphore* semaphore) {
  /*  find the mutex in the semaphores array */
  int i = 0;
  while (semaphores[i] != semaphore && i < OS_LOG_MAX_NUM_SEMAPHORES) i++;
  
  /*  check if found the mutex, and then clear it */
  if (i < OS_LOG_MAX_NUM_SEMAPHORES) {
    semaphores[i] = 0;
  }
}

void logSemaphoreAcquired(LogSemaphore* semaphore, LogTask* task) {
  task = logCurrentTask();
  logEvent(kLogEventSemaphoreAcquired, semaphore, task, (void*)logSemaphoreCount(semaphore));
}

void logSemaphoreSuspended(LogSemaphore* semaphore, LogTask* task) {
  task = logCurrentTask();
  logEvent(kLogEventSemaphoreSuspended, semaphore, task, (void*)logSemaphoreCount(semaphore));
}

void logSemaphoreReleased(LogSemaphore* semaphore, LogTask* task) {
  task = logCurrentTask();
  logEvent(kLogEventSemaphoreReleased, semaphore, task, (void*)logSemaphoreCount(semaphore));
}

void logSemaphoreAcquiredUnsafe(LogSemaphore* semaphore, LogTask* task) {
  task = logCurrentTask();
  logEventUnsafe(kLogEventSemaphoreAcquired, semaphore, task, (void*)logSemaphoreCount(semaphore));
}

void logSemaphoreSuspendedUnsafe(LogSemaphore* semaphore, LogTask* task) {
  task = logCurrentTask();
  logEventUnsafe(kLogEventSemaphoreSuspended, semaphore, task, (void*)logSemaphoreCount(semaphore));
}

void logSemaphoreReleasedUnsafe(LogSemaphore* semaphore, LogTask* task) {
  task = logCurrentTask();
  logEventUnsafe(kLogEventSemaphoreReleased, semaphore, task, (void*)logSemaphoreCount(semaphore));
}

#endif

/*
 * Server
 */

#if (OS_RELTEQ_EN > 0 && OS_RELTEQ_HSF_EN > 0) || (OS_TEV_EN > 0 && OS_TEV_SERVER_EN > 0)

void logServerCreated(LogServer* server) {
  /*  find a free spot in the semaphores array */
  int i = 0;
  while (servers[i] != 0 && i < OS_LOG_MAX_NUM_SERVERS) i++;
  
  /*  check if found a spot, and then allocate it by the mutex */
  LOG_ASSERT(i < OS_LOG_MAX_NUM_SERVERS, "Not enough servers (max=%d)", OS_LOG_MAX_NUM_SERVERS);
  if (i < OS_LOG_MAX_NUM_SERVERS) {
    servers[i] = server;
  }
}

void logServerDeleted(LogServer* server) {
  /*  find the mutex in the semaphores array */
  int i = 0;
  while (servers[i] != server && i < OS_LOG_MAX_NUM_SERVERS) i++;
  
  /*  check if found the mutex, and then clear it */
  if (i < OS_LOG_MAX_NUM_SERVERS) {
    servers[i] = 0;
  }
}

void logServerReplenished(LogServer* server, int amount) {
  logEvent(kLogEventServerReplenished, server, (void*)amount, NULL);
}

void logServerResumed(LogServer* server) {
  logEvent(kLogEventServerResumed, server, NULL, NULL);
}

void logServerPreempted(LogServer* server) {
  logEvent(kLogEventServerPreempted, server, NULL, NULL);
}

void logServerDepleted(LogServer* server) {
  logEvent(kLogEventServerDepleted, server, NULL, NULL);
}

void logServerReplenishedUnsafe(LogServer* server, int amount) {
  logEventUnsafe(kLogEventServerReplenished, server, (void*)amount, NULL);
}

void logServerResumedUnsafe(LogServer* server) {
  logEventUnsafe(kLogEventServerResumed, server, NULL, NULL);
}

void logServerPreemptedUnsafe(LogServer* server) {
  logEventUnsafe(kLogEventServerPreempted, server, NULL, NULL);
}

void logServerDepletedUnsafe(LogServer* server) {
  logEventUnsafe(kLogEventServerDepleted, server, NULL, NULL);
}

void logPrintServerHeader(FILE* fp, LogServer* server) {
	if ( logServerIsActive(server) ) {
    logFilePrint(fp, "newServer server%p -priority %d -budget %d -capacity %d", 
      server,
      logServerPriority(server),
      logServerBudget(server),
      logServerBudget(server)
    );
  
#if OS_RELTEQ_NAME_SIZE > 1 || OS_TEV_EN > 0	
    if (logServerName(server)) logFilePrint(fp, " -name \"%s\"", logServerName(server));
#else
    logFilePrint(fp, " -name \"Server%d\"", logServerPriority(server));
#endif
    logFilePrint(fp, "\n");
  }
}

#endif

/*
 * ISR
 */

#if OS_LOG_ISR_EN > 0

void logISREnterUnsafe(void) {
  logEventUnsafe(kLogEventISREnter, NULL, NULL, NULL);
}

void logISRExitUnsafe(void) {
  logEventUnsafe(kLogEventISRExit, NULL, NULL, NULL);
} 

#endif /* OS_LOG_ISR_EN > 0 */

/*
 * Custom events and messages
 */

void logCommand(char* format, void* arg1, void* arg2) {
  logEvent(kLogEventCommand, format, arg1, arg2);
}

void logCustomEvent(char* format, void* arg1, void* arg2) {
  logEvent(kLogEventCustomEvent, format, arg1, arg2);
}

void logMessage(char* format, void* arg1, void* arg2) {
  logEvent(kLogEventMessage, format, arg1, arg2);
}

void logMessageWithColor(char* color, char* format, void* arg1) {
  logEvent(kLogEventMessageWithColor, color, format, arg1);
}


void logCommandUnsafe(char* format, void* arg1, void* arg2) {
  logEventUnsafe(kLogEventCommand, format, arg1, arg2);
}

void logCustomEventUnsafe(char* format, void* arg1, void* arg2) {
  logEventUnsafe(kLogEventCustomEvent, format, arg1, arg2);
}

void logMessageUnsafe(char* format, void* arg1, void* arg2) {
  logEventUnsafe(kLogEventMessage, format, arg1, arg2);
}

void logMessageWithColorUnsafe(char* color, char* format, void* arg1) {
  logEventUnsafe(kLogEventMessageWithColor, color, format, arg1);
}


LogInt32U logGetMissedDeadlines(void) {
#if OS_CRITICAL_METHOD == 3 
    OS_CPU_SR     cpu_sr = 0;
#endif
  LogInt32U out;

  logEnterCriticalSection();
  out = missedDeadlines;
  logExitCriticalSection();
  
  return out;
}

void logPrintTaskHeader(LogFileHandle fp, LogTask* task, int currentJob) {
  currentJob = currentJob;
  // make sure that the task priority does not exceed the task number, otherwise arrivedJobForTask
  // will overflow.
  LOG_ASSERT(logTaskPriority(task) < OS_LOG_MAX_NUM_TASKS, "task '%s' has priority %d, which is outside the range [0, %d]. It cannot be higher than the number of tasks.", logTaskName(task), logTaskPriority(task), OS_LOG_MAX_NUM_TASKS);

#if OS_LOG_IDLE_TASK_EN == 0
  /*  print the idle task header too, only if logging idle task is enabled */
  if (!logTaskIsIdleTask(task)) {
#endif
    logFilePrint(fp, "newTask task%p -priority %d", task, logTaskPriority(task));
    if (logTaskName(task)) logFilePrint(fp, " -name \"%s\"", logTaskName(task));
    logFilePrint(fp, "\n");
    /*  Note that we need to log a first job, to handle switching into the job during  */
    /*  task initialization, i.e. before a call to logTaskStart(). */
/*
  #if OS_LOG_IGNORE_TASK_INIT_EN > 0
    if (!logTaskIsIdleTask(task)) {
      logFilePrint(fp, "ignore job%p.%d\n", task, currentJob);
    }
  #endif
     logFilePrint(fp, "newJob job%p.%d task%p -name \"%s %d\"\n", 
       task,
       currentJob,
       task,
       logTaskName(task),
       currentJob);
*/
#if OS_LOG_IDLE_TASK_EN == 0
  }
#endif      
}

void logPrintLogFile(char* logFileName) {  
#if OS_LOG_SCHED_LOCK_EN == 0 && OS_CRITICAL_METHOD == 3 
    OS_CPU_SR     cpu_sr = 0;
#endif

  FILE *fp;
  int i, j;
  LogTask* task;
  char message[OS_LOG_MAX_MESSAGE_LENGTH];
  int arrivedJobForTask[OS_LOG_MAX_NUM_TASKS];
  int currentJobForTask[OS_LOG_MAX_NUM_TASKS];

#if OS_LOG_ISR_EN > 0
  int currentJobForISR = 0;
#endif
    
#ifndef NO_STDIO
  /* chdir("log/"); */
  /* remove(logFileName); */
#endif

  /*
   * enter a critical section here, so that no one else writes to the stdout
   * while we are writing "to the file" (via the stdout)
   */
  
#if OS_LOG_SCHED_LOCK_EN > 0
  logLockScheduler();
#else
  logEnterCriticalSection();
#endif

  /* add a message stating that we have started logging */
  logMessageWithColor("gray", "begin printing the log", NULL);
  
  /*  initialize the current jobs */
  for (i = 0; i < OS_LOG_MAX_NUM_TASKS; i++) {
    arrivedJobForTask[i] = 0;
    currentJobForTask[i] = 1;
  }

  fp = logFileOpen(logFileName, "wt");
  if (fp) {

    /*  initialize any additional plots and other custom stuff */
    if (prologueFunction) prologueFunction(fp);

    /* print task headers */
    for (i = 0; i < OS_LOG_MAX_NUM_TASKS; i++) {
      if (tasks[i]) {
          logPrintTaskHeader(fp, tasks[i], arrivedJobForTask[logTaskPriority(tasks[i])]);
      }
    }

    /* print server headers */
#if (OS_RELTEQ_EN > 0 && OS_RELTEQ_HSF_EN > 0) || (OS_TEV_EN > 0 && OS_TEV_SERVER_EN > 0)
		for ( i = 0; i < OS_LOG_MAX_NUM_SERVERS; i++ ) {
  		if (servers[i] != 0) {
    		logPrintServerHeader(fp, servers[i]);
			}
		}
#endif

#if OS_LOG_ISR_EN > 0
    /* print ISR header */
    logFilePrint(fp, "newTask isr -priority 0 -kind isr -name \"ISR\"\n");
#endif

    /*  print mutex headers */
#if OS_LOG_MUTEX_EN > 0
    for (i = 0; i < OS_LOG_MAX_NUM_MUTEXES; i++) {
      if (mutexes[i] != 0) {
        logFilePrint(fp, "newMutex Mutex%p -name \"%s\"\n", mutexes[i], logMutexName(mutexes[i]));
      }
    }
#endif

    /*  print semaphore headers */
#if OS_LOG_SEMAPHORE_EN > 0
    for (i = 0; i < OS_LOG_MAX_NUM_SEMAPHORES; i++) {
      if (semaphores[i] != 0) {
        logFilePrint(fp, "newSemaphore Semaphore%p -name \"%s\"\n", semaphores[i], logSemaphoreName(semaphores[i]));
      }
    }
#endif

    /*  print buffer headers */
#if OS_LOG_BUFFER_EN > 0
    for (i = 0; i < OS_LOG_MAX_NUM_BUFFERS; i++) {
      if (buffers[i] != 0) {
        logFilePrint(fp, "newBuffer Buffer%p -name \"%s\"\n", buffers[i], logBufferName(buffers[i]));
      }
    }
#endif

    /* print all the events */  
    for (j = 0; j < numEvents; j++) {
      i = (eventIndex - numEvents + j) % OS_LOG_MAX_NUM_EVENTS;
      EventInfo e = eventLog[i];
      
      LogTask* source;
      LogTask* target;
      
      switch (e.event) {
      
      case kLogEventTaskPreemption:
        /*  skip the initialization of tasks, where tasks are null */
        //if (e.arg2 == NULL || e.arg1 == NULL) break;
        source = e.arg2;
        target = e.arg1;
        
        // check if the job has arrived, i.e. if arrived >= current. if not, it proably means that
        // the task is executing its initial code, before it had the chance to call the period
        // method. we therefore create a temporary job for it.
        if (target && arrivedJobForTask[logTaskPriority(target)] < currentJobForTask[logTaskPriority(target)]) {
          arrivedJobForTask[logTaskPriority(target)] = currentJobForTask[logTaskPriority(target)];
          logFilePrint(fp, "newJob job%p.%d task%p -name \"%s %d\"\n", 
             target,
             currentJobForTask[logTaskPriority(target)],
             target,
             logTaskName(target),
             currentJobForTask[logTaskPriority(target)]);
        }
        
        if (source) {
          logFilePrint(fp, "plot %d jobPreempted job%p.%d", 
            e.time,
            source,
            currentJobForTask[logTaskPriority(source)]
          );
          if (target) {
            logFilePrint(fp, " -target job%p.%d", target, currentJobForTask[logTaskPriority(target)]);
          }
          logFilePrint(fp, "\n");
        }

        if (target) {
          logFilePrint(fp, "plot %d jobResumed job%p.%d\n", 
            e.time,
            target,
            currentJobForTask[logTaskPriority(target)]          
          );
        }

        break;
/*        
      case kLogEventTaskArrival:
        logFilePrint(fp, "plot %d taskArrived task%p\n", e.time, e.arg1);
        break;

      case kLogEventTaskStart:
        task = e.arg1;
        currentJobForTask[logTaskPriority(task)]++;
        logFilePrint(fp, "newJob job%p.%d task%p -name \"%s %d\"\n", 
            task, 
            currentJobForTask[logTaskPriority(task)],
            task, 
            logTaskName(task),
            currentJobForTask[logTaskPriority(task)]);
        logFilePrint(fp, "plot %d jobResumed job%p.%d\n", 
            (e.time == 0 ? 0 : e.time),
            (task == NULL ? 0 : task),
            (currentJobForTask[logTaskPriority(task)] == 0 ? 0 : currentJobForTask[logTaskPriority(task)])  
         );
        break;
*/       
      case kLogEventTaskArrival:
        task = e.arg1;
        arrivedJobForTask[logTaskPriority(task)]++;
        logFilePrint(fp, "plot %d jobArrived job%p.%d task%p -name \"%s %d\"\n", 
            e.time,
            task, 
            arrivedJobForTask[logTaskPriority(task)],
            task, 
            logTaskName(task),
            arrivedJobForTask[logTaskPriority(task)]);
        break;

      case kLogEventTaskStart:
        task = e.arg1;
        currentJobForTask[logTaskPriority(task)]++;
        // check if the job has arrived, i.e. if arrived >= current. if not, it proably means that
        // the task is executing its initial code, before it had the chance to call the period
        // method. we therefore create a temporary job for it.
        if (task && arrivedJobForTask[logTaskPriority(task)] < currentJobForTask[logTaskPriority(task)]) {
          arrivedJobForTask[logTaskPriority(task)] = currentJobForTask[logTaskPriority(task)];
          logFilePrint(fp, "newJob job%p.%d task%p -name \"%s %d\"\n", 
             task,
             currentJobForTask[logTaskPriority(task)],
             task,
             logTaskName(task),
             currentJobForTask[logTaskPriority(task)]);
        }
        // in some cases (e.g. uC/OS-II) a task will call logTaskStart() itself. consequently, it
        // will resume before being able to make the call, meaning that it will resume with the old
        // job number. In this case we need to resume the job again with the updated job number.
        logTaskStarted(e.time, task);
        break;

      case kLogEventTaskCompletion:
        task = e.arg1;
        logFilePrint(fp, "plot %d jobCompleted job%p.%d\n", e.time, task, currentJobForTask[logTaskPriority(task)]);
        break;

      case kLogEventBufferResize:
        logFilePrint(fp, "bufferplot %d resize buffer%p %d\n", e.time, e.arg1, (LogInt32U)e.arg2);
        break;
        
      case kLogEventBufferPop:
        logFilePrint(fp, "bufferplot %d pop buffer%p\n", e.time, e.arg1);
        break;

      case kLogEventBufferPush:
        logFilePrint(fp, "bufferplot %d push buffer%p %d\n", e.time, e.arg1, (LogInt32U)e.arg2);
        break;

      case kLogEventBufferPush2:
        logFilePrint(fp, "bufferplot %d push buffer%p \"%d\\n%c\"\n", e.time, e.arg1, (LogInt32U)e.arg2, (LogInt32U)e.arg3);
        break;

      case kLogEventBufferDrop:
        logFilePrint(fp, "bufferplot %d drop buffer%p %d\n", e.time, e.arg1, (LogInt32U)e.arg2);
        break;

#if OS_LOG_MUTEX_EN > 0
      case kLogEventMutexAcquired:
        task = e.arg2;
        logFilePrint(fp, "plot %d jobAcquiredMutex job%p.%d Mutex%p\n", e.time, task, currentJobForTask[logTaskPriority(task)], e.arg1);
        break;

      case kLogEventMutexReleased:
        task = e.arg2;
        logFilePrint(fp, "plot %d jobReleasedMutex job%p.%d Mutex%p\n", e.time, task, currentJobForTask[logTaskPriority(task)], e.arg1);
        break;
#endif

#if OS_LOG_SEMAPHORE_EN > 0
      case kLogEventSemaphoreAcquired:
        task = e.arg2;
#if OS_LOG_ISR_EN > 0
        if (!task) {
          logFilePrint(fp, "plot %d jobAcquiredSemaphore isr.%d Semaphore%p -state %d\n", e.time, currentJobForISR, e.arg1, e.arg3);
          break;
        }
#endif        
        logFilePrint(fp, "plot %d jobAcquiredSemaphore job%p.%d Semaphore%p -state %d\n", e.time, task, currentJobForTask[logTaskPriority(task)], e.arg1, (int)e.arg3);
        break;

      case kLogEventSemaphoreSuspended:
        task = e.arg2;
#if OS_LOG_ISR_EN > 0
        if (!task) {
          logFilePrint(fp, "plot %d jobSuspendedOnSemaphore isr.%d Semaphore%p -state %d\n", e.time, currentJobForISR, e.arg1, (int)e.arg3);
          break;
        }
#endif        
        logFilePrint(fp, "plot %d jobSuspendedOnSemaphore job%p.%d Semaphore%p -state %d\n", e.time, task, currentJobForTask[logTaskPriority(task)], e.arg1, (int)e.arg3);
        break;

      case kLogEventSemaphoreReleased:
        task = e.arg2;
#if OS_LOG_ISR_EN > 0
        if (!task) {
          logFilePrint(fp, "plot %d jobReleasedSemaphore isr.%d Semaphore%p -state %d\n", e.time, currentJobForISR, e.arg1, (int)e.arg3);
          break;
        }
#endif        
        logFilePrint(fp, "plot %d jobReleasedSemaphore job%p.%d Semaphore%p -state %d\n", e.time, task, currentJobForTask[logTaskPriority(task)], e.arg1, (int)e.arg3);
        break;
#endif

#if (OS_RELTEQ_EN > 0 && OS_RELTEQ_HSF_EN > 0) || (OS_TEV_EN > 0 && OS_TEV_SERVER_EN > 0)

      case kLogEventServerReplenished:
        logFilePrint(fp, "plot %d serverReplenished server%p %d\n", e.time, e.arg1, (LogInt32U)e.arg2);
        break;

      case kLogEventServerResumed:
        logFilePrint(fp, "plot %d serverResumed server%p\n", e.time, e.arg1);
        break;

      case kLogEventServerPreempted:
        logFilePrint(fp, "plot %d serverPreempted server%p\n", e.time, e.arg1);
        break;

      case kLogEventServerDepleted:
        logFilePrint(fp, "plot %d serverDepleted server%p\n", e.time, e.arg1);
        break;
#endif

#if OS_LOG_ISR_EN > 0

      case kLogEventISREnter:
        currentJobForISR++;
        logFilePrint(fp, "plot %d jobArrived isr.%d isr\n", e.time, currentJobForISR);
        logFilePrint(fp, "plot %d jobResumed isr.%d\n", e.time, currentJobForISR);
        break;

      case kLogEventISRExit:
        logFilePrint(fp, "plot %d jobCompleted isr.%d\n", e.time, currentJobForISR);
        break;
#endif

      case kLogEventCommand:
        if (!e.arg1) break;
        logStringPrint(message, (char*)e.arg1, e.arg2, e.arg3);
        logFilePrint(fp, "%s %d\n", message, e.time);
        break;

      case kLogEventCustomEvent:
        if (!e.arg1) break;
        logStringPrint(message, (char*)e.arg1, e.arg2, e.arg3);
        logFilePrint(fp, "plot %d %s\n", e.time, message);
        break;

      case kLogEventMessage:
        if (!e.arg1) break;
        logStringPrint(message, (char*)e.arg1, e.arg2, e.arg3);
        logFilePrint(fp, "plot %d message {%s}\n", e.time, message);
        break;
        
      case kLogEventMessageWithColor:
        if (!e.arg2) break;
        logStringPrint(message, (char*)e.arg2, e.arg3);
        logFilePrint(fp, "plot %d message {%s} -color %s\n", e.time, message, (char*)e.arg1);
        break;
        
      default: 
        logFilePrint(fp, "error \"grasp recorder encountered unknown event %d at time %d\"\n", i, e.time);
        break;
      }
    
      logFileFlush(fp);
    }

    /*  initialize any additional plots and other custom stuff */
    if (epilogueFunction) epilogueFunction(fp);

    /* indicate when we have finished printing the log */
    logFilePrint(fp, "plot %d message {end printing the log} -color gray\n", logGetTime());

#if OS_RELTEQ_EN > 0 && OS_RELTEQ_STATS_EN > 0
    logFilePrint(fp, "#\n# Relteq Statistics:\n#\n");    
    logFilePrint(fp, "# RelteqStatsNumUsedEvents=%d\n", RelteqStatsNumUsedEvents);    
    logFilePrint(fp, "# RelteqStatsMaxUsedEvents=%d\n", RelteqStatsMaxUsedEvents);    
    logFilePrint(fp, "# RelteqStatsNumCreatedEvents=%d\n", RelteqStatsNumCreatedEvents);    
    logFilePrint(fp, "# RelteqStatsNumUsedQueues=%d\n", RelteqStatsNumUsedQueues);    
    logFilePrint(fp, "# RelteqStatsMaxUsedQueues=%d\n", RelteqStatsMaxUsedQueues);    
    logFilePrint(fp, "# RelteqStatsNumCreatedQueues=%d\n#\n", RelteqStatsNumCreatedQueues);
#endif

    logFileClose(fp);
  } else {
    logErrorPrint("could not open log file %s for writing\n", logFileName);
  }

#if OS_LOG_SCHED_LOCK_EN > 0
  logUnlockScheduler();
#else
  logExitCriticalSection();
#endif
}

void logPrint(char* fileName) {
  logPrintLogFile(fileName);
  logAfterPrint();
}

#endif /* OS_LOG_EN */
