#ifndef GRASP_TASK_H
#define GRASP_TASK_H


/***** lab *****/
#define traceSTART()                                    my_traceSTART()
#define traceEND()                                      my_traceEND()
//#define traceTASK_CREATE(pxNewTCB)                      my_traceTASK_CREATE(pxNewTCB)
#define traceTASK_CREATE( pxNewTCB ) \
    printf("\n[grasp]newTask task%d -priority %d -name \"%s\"\n", pxNewTCB->uxTCBNumber, pxNewTCB->uxPriority, pxNewTCB->pcTaskName);       /* need semihosting*/ \
    printf("\n[grasp]plot %d jobArrived job%d.1 task%d\n", xTaskGetTickCount(), pxNewTCB->uxTCBNumber,  pxNewTCB->uxTCBNumber);       /* need semihosting */
#define traceTASK_SWITCHED_IN()                         my_traceTASK_SWITCHED_IN()
#define traceTASK_SWITCHED_OUT()                        my_traceTASK_SWITCHED_OUT()
#define traceMOVED_TASK_TO_READY_STATE( pxTCB )         my_traceMOVED_TASK_TO_READY_STATE( pxTCB )
//#define traceCREATE_MUTEX( pxNewQueue )                 my_traceCREATE_MUTEX( pxNewQueue )
#define traceGIVE_MUTEX_RECURSIVE( pxMutex )            my_traceGIVE_MUTEX_RECURSIVE( pxMutex )
#define traceTAKE_MUTEX_RECURSIVE( pxMutex )            my_traceTAKE_MUTEX_RECURSIVE( pxMutex )


/***** lab *****/


#define my_traceSTART()
//    printf("my_traceSTART\n");              // need semihosting
//    printf("newTask task%d -priority %d -name \"%s\"\n", pxNewTCB->uxTaskNumber, pxNewTCB->uxPriority, pxNewTCB->pcTaskName);       // need semihosting
//    printf("newTask task0 -priority 4 -name \"LED Flash\"\n");
//    printf("newTask task1 -priority 4 -name \"Serial Write 1\"\n");
//    printf("newTask task2 -priority 4 -name \"Serial Write 2\"\n");
//    printf("newTask task3 -priority 2 -name \"Serial Xmit Str\"\n");
//    printf("newTask task4 -priority 4 -name \"Serial Read/Wri\"\n");
//    printf("newTask task5 -priority 0 -name \"IDLE\"\n");


//void my_traceEND(void){
//    printf("my_traceEND\n");              // need semihosting
//}


#define my_traceTASK_SWITCHED_IN() \
    printf("\n[grasp]plot %d jobPreempted job%d.1 -target job%d.1\n", xTaskGetTickCount(), old_task->uxTCBNumber, pxCurrentTCB->uxTCBNumber); \
    printf("\n[grasp]plot %d jobResumed job%d.1\n", xTaskGetTickCount(), pxCurrentTCB->uxTCBNumber);       // need semihosting
//    printf("%d my_traceTASK_SWITCHED_IN\n", xTaskGetTickCount() );              // need semihosting
//    printf("plot %d jobArrived job%d.1 task%d -name \"%s\"\n", xTaskGetTickCount(), pxNewTCB->uxTaskNumber, pxNewTCB->uxTaskNumber, pxNewTCB->pcTaskName);       // need semihosting


#define my_traceTASK_SWITCHED_OUT() \
    static tskTCB * old_task; \
    old_task = pxCurrentTCB; 
//    printf("%d my_traceTASK_SWITCHED_OUT\n", xTaskGetTickCount() );              // need semihosting

#define my_traceMOVED_TASK_TO_READY_STATE( pxTCB ) 
//    printf("%d my_traceMOVED_TASK_TO_READY_STATE\n", xTaskGetTickCount() );              // need semihosting

#define my_traceGIVE_MUTEX_RECURSIVE(pxMutex) 
//    printf("%d my_traceGIVE_MUTEX_RECURSIVE\n", xTaskGetTickCount() );              // need semihosting


#define my_traceTAKE_MUTEX_RECURSIVE(pxMutex) 
//    printf("%d my_traceTAKE_MUTEX_RECURSIVE\n", xTaskGetTickCount() );              // need semihosting

#endif
