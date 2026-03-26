/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file nvm.h
    \brief Non-volatile memory controller
*/
/***********************************************************************/

#ifndef __NVM_H__
#define __NVM_H__

#include <stdbool.h>
#include <stdint.h>

#define NVM_PAGE_SIZE 64                  // (bytes)
#define NVM_ROW_SIZE  (NVM_PAGE_SIZE * 4) // (bytes)

int nvmEraseRow(uint32_t *dst, bool notUserRow);
int nvmExecuteWriteUserPage(void);
int nvmExecuteWriteUserConfigRow(void);
int nvmClearPageBuffer(void);

#endif // __NVM_H__
