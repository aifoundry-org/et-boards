/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file flash_access.h
    \brief API that provides access to the MCU internal flash memory.
*/
/***********************************************************************/

#ifndef __FLASH_ACCESS_H__
#define __FLASH_ACCESS_H__

#include "nvm.h"
#include <stdint.h>
#include <stdbool.h>

/***********************************************************************
 * GLOBAL Macros:        Defininition of global macros
***********************************************************************/
#define NVM_PAGE_WORDS (NVM_PAGE_SIZE / sizeof(uint32_t))

#define NVM_USERROW_SIZE  (8) // (bytes)
#define NVM_USERROW_WORDS (NVM_USERROW_SIZE / sizeof(uint32_t))

/***********************************************************************
 * GLOBAL Functions:   Declaration of global functions
***********************************************************************/
int Flash_Access_Erase_Row(uint32_t *addr);
int Flash_Access_Write_Page(void);
bool Flash_Access_Is_Addr_Page_Aligned(uint32_t addr);
bool Flash_Access_Is_Addr_Row_Aligned(uint32_t addr);
bool Flash_Access_Is_Row_Erase_Required(uint32_t startAddr, uint32_t endAddr);

volatile uint8_t *Flash_Access_Get_User_Row(void);
int Flash_Access_Write_User_Row(uint32_t offset, uint32_t v, uint32_t lng);

#endif //__FLASH_ACCESS_H__