/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file etsoc_cmd_handler_event.h
    \brief 
*/
/***********************************************************************/
#ifndef _ETSOC_CMD_HANDLER_EVENT_H_
#define _ETSOC_CMD_HANDLER_EVENT_H_

#include <stdint.h>
#include <stdbool.h>

/***********************************************************************
 * GLOBAL data types:  Defininition of global data types
***********************************************************************/
/** Structure holding info of the command received from the ET-SOC. */
typedef struct {
    bool isDataReadRequested; /**< Is ET-SOC read or write operation requested. */
    uint8_t cmdIdx;           /**< Command index according to the protocol spec. */
    uint32_t data;            /**< Data received from the ET-SOC. Relevant only for the write commands. */
} etsocCommandEvent_t;

/***********************************************************************
 * GLOBAL Functions:   Declaration of global functions
***********************************************************************/
int etsocCommandSendEvent(bool isCmdRead, uint8_t cmdIdx, uint32_t data);

#endif //_ETSOC_CMD_HANDLER_EVENT_H_