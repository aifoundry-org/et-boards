/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file nvm.c
    \brief C file for erasing and writing a block to NVM flash
*/
/***********************************************************************/

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "board_defs.h"
#include "FreeRTOS.h"
#include "hooks.h"
#include "task.h"
#include "globals.h"
#include "utils.h"
#include "nvm.h"

/***********************************************************************
 * Data types:      Defininition of local data types
***********************************************************************/
typedef struct
{
    NVMCTRL_INTFLAG_Type intrFlags;
} nvmInfo_t;

/***********************************************************************
 * Variables:  Defininition of local variables and constants
***********************************************************************/
static volatile nvmInfo_t nvmInfo;

/***********************************************************************
 * Macros:        Defininition of local macros
***********************************************************************/
#define NMV_ENABLE_INTERRUPTS 0  //todo SW-17341: remove when NVMC interrupts bug is fixed

// 64 byte page and four pages to a 256 byte row
#define NVM_MEMORY                  ((volatile uint32_t *)FLASH_ADDR)
#define NMVCTRL_READY_TIMEOUT_MS    (20)

#define NMV_ERROR_MASK    (NVMCTRL_STATUS_NVME | NVMCTRL_STATUS_LOCKE | NVMCTRL_STATUS_PROGE)

/***********************************************************************
 * Functions:   Declaration of local functions
***********************************************************************/
static int nvmExecuteWrite(bool notUserRow);
static bool nvmWaitReady(uint32_t timeoutMs);
static bool isRowErased(uint32_t *src);
static void clearErrorStatus(void);

/***********************************************************************
 * GLOBAL Functions:   Defininition of global functions
***********************************************************************/
void nvmInit(void)
{
#if NMV_ENABLE_INTERRUPTS
    NVIC_ClearPendingIRQ(NVMCTRL_IRQn);
    NVIC_EnableIRQ(NVMCTRL_IRQn);
#endif
}

void NVMCTRL_Handler(void)
{
    UBaseType_t uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    nvmInfo.intrFlags.reg = NVMCTRL->INTFLAG.reg;
    NVMCTRL->INTFLAG.reg = nvmInfo.intrFlags.reg; // clear

    vTaskNotifyGiveFromISR(globals.fwUpdateManagerTaskHandle, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

    taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);
}

int nvmEraseRow(uint32_t *dst, bool notUserRow)
{
    int status = STATUS_SUCCESS;

    if( (((uint32_t)dst > (NVM_PAGE_SIZE * NVM_PAGE_COUNT)) || (((uint32_t)dst & (NVM_ROW_SIZE - 1)) != 0))
        && notUserRow )
    {
        return NVM_ERROR_INVALID_DST_ADDR;
    }

    while(NVMCTRL->INTFLAG.bit.READY == 0);

    if(isRowErased(dst))
    {
        return STATUS_SUCCESS;
    }

    clearErrorStatus();

#if NMV_ENABLE_INTERRUPTS
    //NVMCTRL->INTFLAG.reg |= NVMCTRL_INTFLAG_ERROR; //clear interrupt flags
    NVMCTRL->INTENSET.reg = NVMCTRL_INTFLAG_READY | NVMCTRL_INTFLAG_ERROR; // enable interrupts
#endif

    NVMCTRL->CTRLB.reg |= NVMCTRL_CTRLB_CACHEDIS;

    NVMCTRL->ADDR.reg = (uint32_t) dst/2;
    /* Erase the row */
    NVMCTRL->CTRLA.reg = (NVMCTRL_CTRLA_CMDEX_KEY | (notUserRow ? NVMCTRL_CTRLA_CMD_ER : NVMCTRL_CTRLA_CMD_EAR ) );
    if(!nvmWaitReady(NMVCTRL_READY_TIMEOUT_MS))
    {
        NVMCTRL->CTRLB.reg &= ~NVMCTRL_CTRLB_CACHEDIS;
        return NVM_ERROR_WAIT_TIMEOUT;
    }

    status = ((NVMCTRL->STATUS.reg & NMV_ERROR_MASK) == 0) ? STATUS_SUCCESS : NVM_ERROR_CONTROL_REG;
    clearErrorStatus();

    NVMCTRL->CTRLB.reg &= ~NVMCTRL_CTRLB_CACHEDIS;

    return status;
}

int nvmExecuteWriteUserPage(void)
{
    //Writes user page to NVM main address space
    return nvmExecuteWrite(true);
}

int nvmExecuteWriteUserConfigRow(void)
{
    //Writes to NVM user row space (addr 0x804000)
    return nvmExecuteWrite(false);
}

int nvmClearPageBuffer(void)
{
    NVMCTRL->CTRLA.reg = (NVMCTRL_CTRLA_CMDEX_KEY | NVMCTRL_CTRLA_CMD_PBC);
    if (!nvmWaitReady(NMVCTRL_READY_TIMEOUT_MS))
    {
        return NVM_ERROR_WAIT_TIMEOUT;
    }
    return STATUS_SUCCESS;
}

/***********************************************************************
 * Functions:   Defininition of local functions
***********************************************************************/
static void clearErrorStatus(void)
{
    NVMCTRL->STATUS.bit.NVME = 1;
    NVMCTRL->STATUS.bit.PROGE = 1;
    NVMCTRL->STATUS.bit.LOCKE = 1;
}

static int nvmExecuteWrite(bool notUserRow)
{
    int status = STATUS_SUCCESS;

#if NMV_ENABLE_INTERRUPTS
    //NVMCTRL->INTFLAG.reg |= NVMCTRL_INTFLAG_ERROR; //clear interrupt flags
    NVMCTRL->INTENSET.reg = NVMCTRL_INTFLAG_READY | NVMCTRL_INTFLAG_ERROR; // enable interrupts
#endif

    clearErrorStatus();

    while(NVMCTRL->INTFLAG.bit.READY == 0);

    NVMCTRL->CTRLA.reg = (NVMCTRL_CTRLA_CMDEX_KEY | (notUserRow ? NVMCTRL_CTRLA_CMD_WP : NVMCTRL_CTRLA_CMD_WAP));

    //wait for write command to finish
    if(!nvmWaitReady(NMVCTRL_READY_TIMEOUT_MS))
    {
        return NVM_ERROR_WAIT_TIMEOUT;
    }

    status = ((NVMCTRL->STATUS.reg & NMV_ERROR_MASK) == 0) ? STATUS_SUCCESS : NVM_ERROR_CONTROL_REG;
    clearErrorStatus();

    return status;
}

static bool nvmWaitReady(uint32_t timeoutMs)
{
#if NMV_ENABLE_INTERRUPTS
    if (NVMCTRL->INTFLAG.bit.READY)
    {
        return true;
    }

    //writting or erasing in progress
    return (ulTaskNotifyTake(pdFALSE, pdMS_TO_TICKS(timeoutMs)) != 0);
#else
    uint64_t startTimeMs = getTimer2();
    while(NVMCTRL->INTFLAG.bit.READY != 1)
    {
        if((getTimer2() - startTimeMs) > timeoutMs)
        {
            return false;
        }
    }
    return true;
#endif
}

static bool isRowErased(uint32_t *src)
{
    src = (uint32_t *)((uint32_t)src & ~(NVM_ROW_SIZE - 1));
    for (uint32_t i = 0; i < NVM_ROW_SIZE / sizeof(uint32_t); i++)
    {
        if (*src++ != 0xFFFFFFFF)
        {
            return false;
        }
    }
    return true;
}

