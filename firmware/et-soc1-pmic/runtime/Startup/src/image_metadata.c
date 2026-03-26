/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file image_metadata.c
    \brief Definition of the image metadata structure
*/
/***********************************************************************/

#include <stdint.h>
#include "image_metadata.h"
#include "config.h"
#include "version.h"
#include "board_defs.h"
#include "commitinfo.h"
#include "hw_encoding.h"

/***********************************************************************
 * GLOBAL Variables:  Defininition of global variables and constants
***********************************************************************/

/* ----------------------------------- Truncated loadable images start here ------------------------------------------------ */

#if (BOOT_IMAGE_SLOT == BOOT_SLOT_1)
/* Empty space reserved if image is placed to slot 1 */
__attribute__((section(".emptyslot"))) uint8_t const dummyBlock[IMAGESIZE];
#endif

/* Reserved space for vectors (.vectors). These are placed in startup_samd20.c file. */

/* Read-only metadata containing properties of the image.
   Image metadata is at offset RT_IMAGE_METADATA_OFFSET = 0x100 */
#define RT_METADATA_VERSION_H 1
#define RT_METADATA_VERSION_M 0
#define RT_METADATA_VERSION_L 0
__attribute__((section(".rtmetadata"))) rtMetadata_t const volatile rtMetadata = {
    .supported_board_types =
        ((1 << HW_ENCODING_RESERVE_0) | (1 << HW_ENCODING_PCIE_1088) | (1 << HW_ENCODING_PCIE_V3_2) |
            (1 << HW_ENCODING_PCIE_V3_3) | (1 << HW_ENCODING_BUB_REV_2)),
    .start_addr = START_OFFSET,
    .rt_fw_version = (RT_FW_VERSION_H << 16) | (RT_FW_VERSION_M << 8) | (RT_FW_VERSION_L),
    .hash = { LASTCOMMITID_SHORT },
    .bl_fw_version = (BL_FW_VERSION_H << 16) | (BL_FW_VERSION_M << 8) | (BL_FW_VERSION_L),
    .metadata_version = (RT_METADATA_VERSION_H << 16) | (RT_METADATA_VERSION_M << 8) | (RT_METADATA_VERSION_L),
    .build_type = (BUILD_TYPE_DEFINED << BUILD_TYPE_DEFINED_BIT_SHIFT) |
                  (UNCOMMITTED_CHANGES << BUILD_TYPE_UNCOMMITTTED_CHANGES_BIT_SHIFT) | (BUILD_TYPE),
};

/* diffinfo string follows, defined in diffinfo.c */

/***********************************************************************
 * GLOBAL Functions:   Defininition of global functions
***********************************************************************/
uint32_t Image_Metadata_Get_Curr_Fw_Ver(void)
{
    return rtMetadata.rt_fw_version;
}

buildType_t Image_Metadata_Get_Curr_Build_Type(void)
{
    return (buildType_t)rtMetadata.build_type;
}

const char *Image_Metadata_Decode_Build_Type(buildType_t buildType)
{
    switch (buildType)
    {
        case UNKNOWN:
            return "Unknown";
        case DEBUG_BUILD:
            return "Debug";
        case RELEASE_BUILD:
            return "Release";
        case DEBUG_BUILD_UNCOMMITED_CHANGES:
            return "Debug with uncommitted changes";
        case RELEASE_BUILD_UNCOMMITED_CHANGES:
            return "Release with uncommitted changes";
        default:
            return "Undefined";
    }
}