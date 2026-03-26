/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file common_defs.h
    \brief Common definition for Bring up Board (BUB) and PCIe.
*/
/***********************************************************************/

#ifndef __COMMON_DEFS_H__
#define __COMMON_DEFS_H__

/*!
 * @fn system_init
 * @brief initializes all peripherals prior to main loop
 */
void system_init1(void);
void system_init2(void);
int systemEnableHwInterrupts(void);
void systemDisableHwInterrupts(void);

#endif // __COMMON_DEFS_H__
