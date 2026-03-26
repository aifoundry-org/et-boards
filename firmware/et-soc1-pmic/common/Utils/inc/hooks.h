/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file hooks.h
    \brief For use with <processor> and <library>
*/
/***********************************************************************/

#ifndef __HOOKS_H__
#define __HOOKS_H__

#if ( configSUPPORT_STATIC_ALLOCATION == 1 )
/*
 * @fn vApplicationGetIdleTaskMemory
 * @brief
 *
 * @param[in] StaticTask_t **ppxIdleTaskTCBBuffer
 * @param[in] StackType_t **ppxIdleTaskStackBuffer
 * @param[in] uint32_t *pulIdleTaskStackSize
 */
 void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer,
                                    StackType_t **ppxIdleTaskStackBuffer,
                                    uint32_t *pulIdleTaskStackSize );
#if ( configUSE_TIMERS == 1 )
/*
 * @fn vApplicationGetTimerTaskMemory
 * @brief
 *
 * @param[in] StaticTask_t **ppxTimerTaskTCBBuffer
 * @param[in] StackType_t **ppxTimerTaskStackBuffer
 * @param[in] uint32_t *pulTimerTaskStackSize
 */
 void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer,
                                     StackType_t **ppxTimerTaskStackBuffer,
                                     uint32_t *pulTimerTaskStackSize );
#endif
#endif

void *_sbrk(int32_t inc);


#endif // __HOOKS_H__
