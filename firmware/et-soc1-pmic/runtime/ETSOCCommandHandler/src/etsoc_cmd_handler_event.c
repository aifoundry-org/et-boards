/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file etsoc_cmd_handler_event.c
    \brief
*/
/***********************************************************************/
#include <stdint.h>
#include <stdbool.h>

#include "etsoc_cmd_handler_event.h"
#include "FreeRTOS.h"
#include "globals.h"
#include "utils.h"
#include "error_codes.h"

/***********************************************************************
 * GLOBAL Functions:   Defininition of global functions
***********************************************************************/
/** \brief ET-SOC command send event.
 *
 * Called from the I2C slave interrupt
 * when command is received from the ET-SOC.
 * Relevant command information are sent to the queue
 * which is received and handled by the ETSOC command handler task.
 * Called from the CLI task when sp command write request is sent.
 *
 * \param[in] isCmdRead ET-SOC read data or write data is requested.
 * \param[in] cmdIdx    Index of the read or write ET-SOC command to be executed.
 * \param[in] data      Data sent from the ET-SOC in the case of the write operation.
 */
int etsocCommandSendEvent(bool isCmdRead, uint8_t cmdIdx, uint32_t data)
{
    BaseType_t status;
    etsocCommandEvent_t etsocCommand = {
        .isDataReadRequested = isCmdRead,
        .cmdIdx = cmdIdx,
        .data = data,
    };

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    status = xQueueSendToBackFromISR(globals.etsocCmdHandlerTaskQueueHandle, &etsocCommand, &xHigherPriorityTaskWoken);
    if (status == errQUEUE_FULL)
    {
        printfFromInterrupt("ET SOC command handler task queue full, event can't be handled", 0, 0);
        return CMD_HANDLER_ERROR_TASK_QUEUE_FULL;
    }
    else if (status != pdPASS)
    {
        printfFromInterrupt("ET SOC command handler task queue send error: %d", status, 0);
        return CMD_HANDLER_ERROR_QUEUE_SEND;
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

    return STATUS_SUCCESS;
}