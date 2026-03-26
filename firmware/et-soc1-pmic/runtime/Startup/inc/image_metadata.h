/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file image_metadata.h
    \brief Image metadata related defines
*/
/***********************************************************************/

#include <stdint.h>
#include "config.h"

/***********************************************************************
 * GLOBAL Macros:  Defininition of global macros
***********************************************************************/
#if (BOOT_IMAGE_SLOT == BOOT_SLOT_0)
#define START_OFFSET SLOT_0_IMGOFFSET
#elif (BOOT_IMAGE_SLOT == BOOT_SLOT_1)
#define START_OFFSET SLOT_1_IMGOFFSET
#elif (BOOT_IMAGE_SLOT == BOOT_STANDALONE)
#define START_OFFSET STANDALONE_IMGOFFSET
#else
#error "Slot must be defined"
#endif

#define BUILD_TYPE_DEFINED                        1
#define BUILD_TYPE_DEFINED_BIT_SHIFT              2
#define BUILD_TYPE_UNCOMMITTTED_CHANGES_BIT_SHIFT 1

/***********************************************************************
 * GLOBAL Data types:      Defininition of global data types
***********************************************************************/
/** Build type is added to image metadata at compile time.
 *  Encoding:
 *  bit 0: 0 = debug build, 1 = release build 
 *  bit 1: 0 = clean, 1 = uncommitted changes present
 *  bit 2: 0 = build type is unknown, 1 = build type is defined with bits 0 and 1.
 */
typedef enum {
    UNKNOWN = 0b000,                          //0  Note: has to be 0 for backward compatibility
    DEBUG_BUILD = 0b100,                      //4
    RELEASE_BUILD = 0b101,                    //5
    DEBUG_BUILD_UNCOMMITED_CHANGES = 0b110,   //6
    RELEASE_BUILD_UNCOMMITED_CHANGES = 0b111, //7
} buildType_t;

/***********************************************************************
 * GLOBAL Functions:   Decaration of global functions
***********************************************************************/
uint32_t Image_Metadata_Get_Curr_Fw_Ver(void);
buildType_t Image_Metadata_Get_Curr_Build_Type(void);
const char *Image_Metadata_Decode_Build_Type(buildType_t buildType);
