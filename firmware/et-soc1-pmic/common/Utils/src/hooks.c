/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/
/**
 * @file hooks.c
 * @brief Hooks for Free RTOS
 * @details Link this with the rest of RTOS, this provides the hooks for
 * various configurations we use. Sample from FreeRTOS for static allocation
 *
 **/
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "compiler.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "hooks.h"
#if ( configSUPPORT_STATIC_ALLOCATION == 1 )
/* configSUPPORT_STATIC_ALLOCATION is set to 1, so the application must provide an
implementation of vApplicationGetIdleTaskMemory() to provide the memory that is
used by the Idle task. */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer,
                                    StackType_t **ppxIdleTaskStackBuffer,
                                    uint32_t *pulIdleTaskStackSize )
{
/* If the buffers to be provided to the Idle task are declared inside this
function then they must be declared static - otherwise they will be allocated on
the stack and so not exists after this function exits. */
static StaticTask_t xIdleTaskTCB;
static StackType_t uxIdleTaskStack[ configMINIMAL_STACK_SIZE ];

    /* Pass out a pointer to the StaticTask_t structure in which the Idle task's
    state will be stored. */
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

    /* Pass out the array that will be used as the Idle task's stack. */
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;

    /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
    Note that, as the array is necessarily of type StackType_t,
    configMINIMAL_STACK_SIZE is specified in words, not bytes. */
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}
/*-----------------------------------------------------------*/
#if ( configUSE_TIMERS == 1 )

/* configSUPPORT_STATIC_ALLOCATION and configUSE_TIMERS are both set to 1, so the
application must provide an implementation of vApplicationGetTimerTaskMemory()
to provide the memory that is used by the Timer service task. */
void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer,
                                     StackType_t **ppxTimerTaskStackBuffer,
                                     uint32_t *pulTimerTaskStackSize )
{
/* If the buffers to be provided to the Timer task are declared inside this
function then they must be declared static - otherwise they will be allocated on
the stack and so not exists after this function exits. */
static StaticTask_t xTimerTaskTCB;
static StackType_t uxTimerTaskStack[ configTIMER_TASK_STACK_DEPTH ];

    /* Pass out a pointer to the StaticTask_t structure in which the Timer
    task's state will be stored. */
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;

    /* Pass out the array that will be used as the Timer task's stack. */
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;

    /* Pass out the size of the array pointed to by *ppxTimerTaskStackBuffer.
    Note that, as the array is necessarily of type StackType_t,
    configTIMER_TASK_STACK_DEPTH is specified in words, not bytes. */
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}
#endif
#endif

void vAssertCalled( unsigned long ulLine, const char * const pcFileName )
{
volatile uint32_t ulNonZeroToContinue = 0;

    /* Parameters are not used. But useful in the debugger */
    volatile unsigned long linenumber =  ulLine;
    volatile const char * const filename = pcFileName;
    ( void ) linenumber;
    ( void ) filename;
    taskENTER_CRITICAL();
    {
        /* You can step out of this function to debug the assertion by using
        the debugger to set ulNonZeroToContinue to a non-zero
        value. */
        while( ulNonZeroToContinue == 0 ) {}
    }
    taskEXIT_CRITICAL();
}

#if configCHECK_FOR_STACK_OVERFLOW > 0
    extern void vApplicationStackOverflowHook( TaskHandle_t xTask, char *pcTaskName );

void vApplicationStackOverflowHook( TaskHandle_t xTask,
                                     char *pcTaskName )
{
    (void) xTask;
    (void) pcTaskName;

    taskDISABLE_INTERRUPTS();
    while(true);

}
#endif
void vMainConfigureTimerForRunTimeStats( void )
{

}
/*-----------------------------------------------------------*/

unsigned long ulMainGetRunTimeCounterValue( void )
{

    return ( unsigned long ) 0;
}


void *_sbrk(int32_t inc)
{
    configASSERT(0);
    return NULL;
}
