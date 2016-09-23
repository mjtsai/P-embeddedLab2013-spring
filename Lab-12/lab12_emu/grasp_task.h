#ifndef GRASP_TASK_H
#define GRASP_TASK_H

#include "queue.h"
#include <stdio.h>


/***** lab *****/


void my_traceSTART(void){
    printf("my_traceSTART\n");              // need semihosting

}

void my_traceEND(void){
    printf("my_traceEND\n");              // need semihosting
}


void my_traceTASK_SWITCHED_IN(void){
    printf("my_traceTASK_SWITCHED_IN\n");              // need semihosting

}

void my_traceTASK_SWITCHED_OUT(void){
    printf("my_traceTASK_SWITCHED_OUT\n");              // need semihosting
}


void my_traceGIVE_MUTEX_RECURSIVE(xQueueHandle pxMutex){
    printf("my_traceGIVE_MUTEX_RECURSIVE\n");              // need semihosting

}

void my_traceTAKE_MUTEX_RECURSIVE(xQueueHandle pxMutex){
    printf("my_traceTAKE_MUTEX_RECURSIVE\n");              // need semihosting
}

#endif
