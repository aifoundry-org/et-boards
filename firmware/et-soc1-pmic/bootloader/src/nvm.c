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

#include "nvm.h"
#include "board_defs.h"
#include "error_codes.h"
#include <stdbool.h>
#include <stdint.h>

/***********************************************************************
 * Macros:        Defininition of local macros
***********************************************************************/
// 64 byte page and four pages to a 256 byte row
#define NVM_MEMORY               ((volatile uint32_t *)FLASH_ADDR)
#define NMVCTRL_READY_TIMEOUT_MS (20)

#define NMV_ERROR_MASK (NVMCTRL_STATUS_NVME | NVMCTRL_STATUS_LOCKE | NVMCTRL_STATUS_PROGE)

/***********************************************************************
 * Functions:   Declaration of local functions
***********************************************************************/
static int nvmExecuteWrite(bool notUserRow);
static bool isRowErased(uint32_t *src);
static void clearErrorStatus(void);

/***********************************************************************
 * GLOBAL Functions:   Defininition of global functions
***********************************************************************/
int nvmEraseRow(uint32_t *dst, bool notUserRow)
{
    int status = STATUS_SUCCESS;

    if ((((uint32_t)dst > (NVM_PAGE_SIZE * NVM_PAGE_COUNT)) || (((uint32_t)dst & (NVM_ROW_SIZE - 1)) != 0)) &&
        notUserRow)
    {
        return NVM_ERROR_INVALID_DST_ADDR;
    }

    while (NVMCTRL->INTFLAG.bit.READY == 0)
        ;

    if (isRowErased(dst))
    {
        return STATUS_SUCCESS;
    }

    clearErrorStatus();

    NVMCTRL->CTRLB.reg |= NVMCTRL_CTRLB_CACHEDIS;

    NVMCTRL->ADDR.reg = (uint32_t)dst / 2;
    /* Erase the row */
    NVMCTRL->CTRLA.reg = (NVMCTRL_CTRLA_CMDEX_KEY | (notUserRow ? NVMCTRL_CTRLA_CMD_ER : NVMCTRL_CTRLA_CMD_EAR));

    while (NVMCTRL->INTFLAG.bit.READY == 0)
        ;

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
    while (NVMCTRL->INTFLAG.bit.READY == 0)
        ;

    return STATUS_SUCCESS;
}

/***********************************************************************
 * Functions:   Defininition of local functions
***********************************************************************/
static int nvmExecuteWrite(bool notUserRow)
{
    int status = STATUS_SUCCESS;

    clearErrorStatus();

    while (NVMCTRL->INTFLAG.bit.READY == 0)
        ;

    NVMCTRL->CTRLA.reg = (NVMCTRL_CTRLA_CMDEX_KEY | (notUserRow ? NVMCTRL_CTRLA_CMD_WP : NVMCTRL_CTRLA_CMD_WAP));

    while (NVMCTRL->INTFLAG.bit.READY == 0)
        ;

    status = ((NVMCTRL->STATUS.reg & NMV_ERROR_MASK) == 0) ? STATUS_SUCCESS : NVM_ERROR_CONTROL_REG;
    clearErrorStatus();

    return status;
}

static void clearErrorStatus(void)
{
    NVMCTRL->STATUS.bit.NVME = 1;
    NVMCTRL->STATUS.bit.PROGE = 1;
    NVMCTRL->STATUS.bit.LOCKE = 1;
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
