#ifndef OS_LOG_PORT_H
#define OS_LOG_PORT_H

#include "FreeRTOS.h"
#include <stdio.h>
#include "list.h"

#define OS_LOG_EN                   1  /* Enable (1) or Disable (0) logging                            */
#define OS_LOG_MAX_NUM_EVENTS    5000  /* Max. number of logged events in the cyclic array             */
#define OS_LOG_MAX_MESSAGE_LENGTH  128 /* Max. length of a message logged with logMessage()            */
#define OS_LOG_MAX_NUM_TASKS       10   /* Max. number of tasks in the application         */
#define OS_LOG_MAX_NUM_SERVERS      0  /* Max. number of servers in the application                    */
#define OS_LOG_MUTEX_EN             1  /* Enable (1) or Disable (0) logging of mutexes                 */
#define OS_LOG_MUTEX_VERBOSE_EN     1  /* Print a message when a mutex method is entered               */
#define OS_LOG_MAX_NUM_MUTEXES      3  /* Max. number of mutexes in the application                    */
#define OS_LOG_SEMAPHORE_EN         0  /* Enable (1) or Disable (0) logging of semaphores              */
#define OS_LOG_SEMAPHORE_VERBOSE_EN 0  /* Print a message when a semaphore method is entered           */
#define OS_LOG_MAX_NUM_SEMAPHORES 254  /* Max. number of semaphores in the application                 */
#define OS_LOG_BUFFER_EN            0  /* Enable (1) or Disable (0) logging of semaphores              */
#define OS_LOG_MAX_NUM_BUFFERS      4  /* Max. number of servers in the application                    */
#define OS_LOG_IDLE_TASK_EN         0  /* Enable (1) or Disable (0) logging of the system idle task    */
#define OS_LOG_ISR_EN               0  /* Enable (1) or Disable (0) logging of the ISRs                */
#define OS_LOG_IGNORE_TASK_INIT_EN  0  /* Inserts an 'ignore' command to neglect 0th job of each task  */
#define OS_LOG_SCHED_LOCK_EN        0  /* Enable (1) or Disable (0) using OSSchedLock(), insetead of   */
                                       /* OS_ENTER_CRITICAL(), when printing the log.                  */
#define OS_LOG_SCHEDULER_EN         1  /* Enable (1) or Disable (0) logging of the scheduler           */
#define OS_LOG_BEGIN_FRAME          0  /* Number of frames to wait before starting to log after connecting */
#define OS_LOG_NUM_FRAMES           10  /* Number of frames to log                                      */



#if OS_LOG_EN > 0

typedef void*           LogFileHandle   ;

typedef struct tskTaskControlBlock
{
	volatile portSTACK_TYPE	*pxTopOfStack;		/*< Points to the location of the last item placed on the tasks stack.  THIS MUST BE THE FIRST MEMBER OF THE STRUCT. */

	#if ( portUSING_MPU_WRAPPERS == 1 )
		xMPU_SETTINGS xMPUSettings;				/*< The MPU settings are defined as part of the port layer.  THIS MUST BE THE SECOND MEMBER OF THE STRUCT. */
	#endif	
	
	xListItem				xGenericListItem;	/*< List item used to place the TCB in ready and blocked queues. */
	xListItem				xEventListItem;		/*< List item used to place the TCB in event lists. */
	unsigned portBASE_TYPE	uxPriority;			/*< The priority of the task where 0 is the lowest priority. */
	portSTACK_TYPE			*pxStack;			/*< Points to the start of the stack. */
	signed char				pcTaskName[ configMAX_TASK_NAME_LEN ];/*< Descriptive name given to the task when created.  Facilitates debugging only. */

	#if ( portSTACK_GROWTH > 0 )
		portSTACK_TYPE *pxEndOfStack;			/*< Used for stack overflow checking on architectures where the stack grows up from low memory. */
	#endif

	#if ( portCRITICAL_NESTING_IN_TCB == 1 )
		unsigned portBASE_TYPE uxCriticalNesting;
	#endif

	#if ( configUSE_TRACE_FACILITY == 1 )
		unsigned portBASE_TYPE	uxTCBNumber;	/*< This stores a number that increments each time a TCB is created.  It allows debuggers to determine when a task has been deleted and then recreated. */
		unsigned portBASE_TYPE  uxTaskNumber;	/*< This stores a number specifically for use by third party trace code. */
	#endif

	#if ( configUSE_MUTEXES == 1 )
		unsigned portBASE_TYPE uxBasePriority;	/*< The priority last assigned to the task - used by the priority inheritance mechanism. */
	#endif

	#if ( configUSE_APPLICATION_TASK_TAG == 1 )
		pdTASK_HOOK_CODE pxTaskTag;
	#endif

	#if ( configGENERATE_RUN_TIME_STATS == 1 )
		unsigned long ulRunTimeCounter;		/*< Used for calculating how much CPU time each task is utilising. */
	#endif

} tskTCB;

/*typedef extern struct tskTaskControlBlock tskTCB;*/
typedef tskTCB          LogTask         ;
/*#define LogTask         tskTCB*/


typedef struct QueueDefinition
{
	signed char *pcHead;				/*< Points to the beginning of the queue storage area. */
	signed char *pcTail;				/*< Points to the byte at the end of the queue storage area.  Once more byte is allocated than necessary to store the queue items, this is used as a marker. */

	signed char *pcWriteTo;				/*< Points to the free next place in the storage area. */
	signed char *pcReadFrom;			/*< Points to the last place that a queued item was read from. */

	xList xTasksWaitingToSend;				/*< List of tasks that are blocked waiting to post onto this queue.  Stored in priority order. */
	xList xTasksWaitingToReceive;			/*< List of tasks that are blocked waiting to read from this queue.  Stored in priority order. */

	volatile unsigned portBASE_TYPE uxMessagesWaiting;/*< The number of items currently in the queue. */
	unsigned portBASE_TYPE uxLength;		/*< The length of the queue defined as the number of items it will hold, not the number of bytes. */
	unsigned portBASE_TYPE uxItemSize;		/*< The size of each items that the queue will hold. */

	signed portBASE_TYPE xRxLock;			/*< Stores the number of items received from the queue (removed from the queue) while the queue was locked.  Set to queueUNLOCKED when the queue is not locked. */
	signed portBASE_TYPE xTxLock;			/*< Stores the number of items transmitted to the queue (added to the queue) while the queue was locked.  Set to queueUNLOCKED when the queue is not locked. */
	
	#if ( configUSE_TRACE_FACILITY == 1 )
		unsigned char ucQueueNumber;
		unsigned char ucQueueType;
	#endif

} xQUEUE;

/*typedef struct QueueDefinition xQUEUE;*/
typedef xQUEUE          LogMutex        ;


typedef int             LogTime         ;
typedef int             LogInt32U       ;
typedef char            LogBool         ;

#define LOG_INLINE          inline



#endif /* OS_LOG_EN */

#endif /* OS_LOG_PORT_H */
