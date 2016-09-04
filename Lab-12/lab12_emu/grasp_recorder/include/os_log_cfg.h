                                       /* ------------------------- LOGGING -------------------------- */
#define OS_LOG_EN                   1  /* Enable (1) or Disable (0) logging                            */
#define OS_LOG_MAX_NUM_EVENTS    50000 /* Max. number of logged events in the cyclic array             */
#define OS_LOG_MAX_MESSAGE_LENGTH  128 /* Max. length of a message logged with logMessage()            */
#define OS_LOG_MAX_NUM_TASKS      OS_LOWEST_PRIO    /* Max. number of tasks in the application         */
#define OS_LOG_MAX_NUM_SERVERS      4  /* Max. number of servers in the application                    */
#define OS_LOG_MUTEX_EN             0  /* Enable (1) or Disable (0) logging of mutexes                 */
#define OS_LOG_MUTEX_VERBOSE_EN     0  /* Print a message when a mutex method is entered               */
#define OS_LOG_MAX_NUM_MUTEXES      3  /* Max. number of mutexes in the application                    */
#define OS_LOG_SEMAPHORE_EN         0  /* Enable (1) or Disable (0) logging of semaphores              */
#define OS_LOG_SEMAPHORE_VERBOSE_EN 0  /* Print a message when a semaphore method is entered           */
#define OS_LOG_MAX_NUM_SEMAPHORES 254  /* Max. number of semaphores in the application                 */
#define OS_LOG_BUFFER_EN            0  /* Enable (1) or Disable (0) logging of semaphores              */
#define OS_LOG_MAX_NUM_BUFFERS      4  /* Max. number of servers in the application                    */
#define OS_LOG_IDLE_TASK_EN         1  /* Enable (1) or Disable (0) logging of the system idle task    */
#define OS_LOG_ISR_EN               1  /* Enable (1) or Disable (0) logging of the ISRs                */
#define OS_LOG_IGNORE_TASK_INIT_EN  1  /* Inserts an 'ignore' command to neglect 0th job of each task  */
#define OS_LOG_SCHED_LOCK_EN        0  /* Enable (1) or Disable (0) using OSSchedLock(), insetead of   */
                                       /* OS_ENTER_CRITICAL(), when printing the log.                  */
#define OS_LOG_SCHEDULER_EN         0  /* Enable (1) or Disable (0) logging of the scheduler           */
#define OS_LOG_BEGIN_FRAME      0  /* Number of frames to wait before starting to log after connecting */
#define OS_LOG_NUM_FRAMES           10  /* Number of frames to log                                      */
