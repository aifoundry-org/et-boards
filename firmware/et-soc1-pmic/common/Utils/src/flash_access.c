/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file flash_access.c
    \brief API that provides access to the MCU internal flash memory.
*/
/***********************************************************************/

#include "flash_access.h"
#include "config.h"
#include "error_codes.h"
#include "nvm.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

/***********************************************************************
 * Functions:   Declaration of local functions
***********************************************************************/
static bool Flash_Access_Is_Addr_Not_User_Row(uint32_t addr);

/***********************************************************************
 * GLOBAL Functions:   Defininition of global functions
***********************************************************************/
int Flash_Access_Erase_Row(uint32_t *addr)
{
    if (addr == NULL)
    {
        return INVALID_ARGUMENT;
    }

    uint32_t *sa = (uint32_t *)((uint32_t)addr & ~(NVM_ROW_SIZE - 1));
    return (nvmEraseRow(sa, Flash_Access_Is_Addr_Not_User_Row((uint32_t)sa)));
}

int Flash_Access_Write_Page(void)
{
    return nvmExecuteWriteUserPage();
}

bool Flash_Access_Is_Addr_Page_Aligned(uint32_t addr)
{
    const uint32_t sa = addr & ~(NVM_PAGE_SIZE - 1);
    return (addr == sa);
}

bool Flash_Access_Is_Addr_Row_Aligned(uint32_t addr)
{
    const uint32_t sa = addr & ~(NVM_ROW_SIZE - 1);
    return (addr == sa);
}

bool Flash_Access_Is_Row_Erase_Required(uint32_t startAddr, uint32_t endAddr)
{
    return (Flash_Access_Is_Addr_Row_Aligned(startAddr) && startAddr < endAddr);
}

/* NVM User Row */
__attribute__((section(".userrow"))) volatile uint8_t userRow[NVM_USERROW_SIZE];

volatile uint8_t *Flash_Access_Get_User_Row(void)
{
    return userRow;
}

int Flash_Access_Write_User_Row(uint32_t offset, uint32_t v, uint32_t lng)
{
    int status = STATUS_SUCCESS;

    if ((offset + lng) >= NVM_USERROW_SIZE)
    {
        return NVM_ERROR_INVALID_DST_ADDR;
    }

    // Clear page buffer
    status = nvmClearPageBuffer();
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    static uint32_t ramBuf[NVM_USERROW_WORDS];
    for (uint32_t i = 0; i < NVM_USERROW_WORDS; i++)
    {
        ramBuf[i] = ((volatile uint32_t *)(volatile void *)userRow)[i];
    }

    memcpy(((uint8_t *)ramBuf) + offset, &v, lng);

    status = Flash_Access_Erase_Row((uint32_t *)(void *)userRow); // known to be on 4 byte boundary
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    for (uint32_t i = 0; i < NVM_USERROW_WORDS; i++)
    {
        // avoid memcpy just in case it might do a 1 byte copy, known to be on 4 byte boundary
        ((volatile uint32_t *)(volatile void *)userRow)[i] = ramBuf[i];
    }

    // Write to the user row flash memory
    status = nvmExecuteWriteUserConfigRow();

    return status;
}

/***********************************************************************
 * Functions:   Defininition of local functions
***********************************************************************/
static bool Flash_Access_Is_Addr_Not_User_Row(uint32_t addr)
{
    bool notUserRow = (uint32_t)&userRow > addr || addr >= ((uint32_t)&userRow + sizeof(userRow));
    return notUserRow;
}