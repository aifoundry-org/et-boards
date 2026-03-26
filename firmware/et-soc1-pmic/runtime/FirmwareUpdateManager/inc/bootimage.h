/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file bootimage.h
    \brief Bootloader/image update related.
*/
/***********************************************************************/

#ifndef __BOOTIMAGE_H__
#define __BOOTIMAGE_H__

#include <stdint.h>
#include "config.h"
#include "globals.h"

/***********************************************************************
 * GLOBAL data types:  Defininition of global data types
***********************************************************************/
typedef enum {
    BT_bootloader = 0,
    BT_current,
    BT_golden,

    BT_unsupported, //must be the last
} bootloaderTarget_t;

typedef enum {
    E_BC_startUpdate = 0,
    E_BC_completeUpdate,
    E_BC_wordUpdate,
    E_BC_chksumread,
    E_BC_setDfltBootSlot,
    E_BC_setBootCounter,
} fwUpdateManagerEventType_t;

typedef struct {
    fwUpdateManagerEventType_t eventType;
    uint32_t data;
} fwUpdateManagerEvent_t;

/***********************************************************************
 * GLOBAL functions:  Declaration of global functions
***********************************************************************/
void bootToImage(bootloaderTarget_t target);
void fwUpdateManagerTask(void *pvParameters);
int wrUpdateCmd(ETSOCVirtualRegisterIdx_t regIdx);
void wrUpdateData(uint32_t data);
void rdUpdatePostCmd(void);
void rdUpdatePostData(void);

#endif // __BOOTIMAGE_H__
