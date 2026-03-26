/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file config.c
    \brief Configuration space related data structures.
*/
/***********************************************************************/
#include <stdint.h>
#include "config.h"
#include "error_codes.h"
#include "flash_access.h"

/***********************************************************************
 * GLOBAL Variables:  Defininition of global variables and constants
***********************************************************************/
/* This is mainly used by bootloader and OTA updates */
__attribute__ ((section(".userpage.1"))) imageConfigHeader_t configHeader;

/* Config header padding */
__attribute__ ((section(".userpage.2"))) uint8_t const configHeaderPadding[CONFIG_HEADER_SIZE - sizeof(imageConfigHeader_t)];

/***********************************************************************
 * Functions:   Declaration of local functions
***********************************************************************/
static uint32_t getImageStartAddressOffset(uint32_t imageSlot);

/***********************************************************************
 * GLOBAL Functions:   Defininition of global functions
***********************************************************************/
uint32_t Config_Get_Metadata_Ver(uint32_t imageSlot)
{
    const uint32_t offset = RT_IMAGE_METADATA_OFFSET + offsetof(rtMetadata_t, metadata_version);
    return *((uint32_t *)(getImageStartAddressOffset(imageSlot) + offset));
}

uint32_t Config_Get_Image_Size_From_Metadata(uint32_t imageSlot)
{
    const uint32_t offset = RT_IMAGE_METADATA_OFFSET + offsetof(rtMetadata_t, image_size);
    return *((uint32_t *)(getImageStartAddressOffset(imageSlot) + offset));
}

uint32_t Config_Get_Supported_Board_Types_From_Metadata(uint32_t imageSlot)
{
    const uint32_t offset = RT_IMAGE_METADATA_OFFSET + offsetof(rtMetadata_t, supported_board_types);
    return *((uint32_t *)(getImageStartAddressOffset(imageSlot) + offset));
}

uint32_t Config_Get_Fw_Ver_From_Metadata(uint32_t imageSlot)
{
    const uint32_t offset = RT_IMAGE_METADATA_OFFSET + offsetof(rtMetadata_t, rt_fw_version);
    return *((uint32_t *)(getImageStartAddressOffset(imageSlot) + offset));
}

uint32_t Config_Get_Compatible_Bl_Ver_From_Metadata(uint32_t imageSlot)
{
    const uint32_t offset = RT_IMAGE_METADATA_OFFSET + offsetof(rtMetadata_t, bl_fw_version);
    return *((uint32_t *)(getImageStartAddressOffset(imageSlot) + offset));
}

uint32_t Config_Get_Sp_Pmic_Interface_Ver_From_Metadata(uint32_t imageSlot)
{
    const uint32_t offset = RT_IMAGE_METADATA_OFFSET + offsetof(rtMetadata_t, sp_pmic_interface_version);
    return *((uint32_t *)(getImageStartAddressOffset(imageSlot) + offset));
}

const char *Config_Get_Image_Hash_From_Metadata(uint32_t imageSlot)
{
    const uint32_t offset = RT_IMAGE_METADATA_OFFSET + offsetof(rtMetadata_t, hash);
    return (const char *)(getImageStartAddressOffset(imageSlot) + offset);
}

uint32_t Config_Get_Checksum_From_Metadata(uint32_t imageSlot)
{
    const uint32_t offset = RT_IMAGE_METADATA_OFFSET + offsetof(rtMetadata_t, checksum);
    return *((uint32_t *)(getImageStartAddressOffset(imageSlot) + offset));
}

uint32_t Config_Get_Build_Type_From_Metadata(uint32_t imageSlot)
{
    const uint32_t offset = RT_IMAGE_METADATA_OFFSET + offsetof(rtMetadata_t, build_type);
    return *((uint32_t *)(getImageStartAddressOffset(imageSlot) + offset));
}

uint32_t *Config_Get_Config_Header(void)
{
    return (uint32_t *)(&configHeader);
}

uint32_t Config_Get_Boot_Slot_From_Config_Header(void)
{
    return configHeader.boot_slot;
}

int Config_Set_Boot_Slot(uint32_t bootSlot)
{
    imageConfigHeader_t tmphdr;
    imageConfigHeader_t *cfghdr = &configHeader;

    /* Backup configuration header data */
    for (uint32_t i = 0; i < (sizeof(imageConfigHeader_t) / sizeof(uint32_t)); i++)
    {
        ((uint32_t *)&tmphdr)[i] = ((const uint32_t *)cfghdr)[i];
    }

    tmphdr.boot_slot = bootSlot;

    /* Erase NVM row (256 bytes) containing configuration header */
    int status = Flash_Access_Erase_Row((uint32_t *)cfghdr);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    /* Update configuration header and write back to flash memeory */
    for (uint32_t i = 0; i < (sizeof(imageConfigHeader_t) / sizeof(uint32_t)); i++)
    {
        ((uint32_t *)cfghdr)[i] = ((const uint32_t *)&tmphdr)[i];
    }
    status = Flash_Access_Write_Page();

    return status;
}

uint32_t Config_Get_Failed_Boot_Cnt_From_Config_Header(void)
{
    return configHeader.boot_failure_counter;
}

int Config_Set_Failed_Boot_Cnt(uint32_t bootCnt)
{
    imageConfigHeader_t tmphdr;
    imageConfigHeader_t *cfghdr = &configHeader;

    /* Backup configuration header data */
    for (uint32_t i = 0; i < (sizeof(imageConfigHeader_t) / sizeof(uint32_t)); i++)
    {
        ((uint32_t *)&tmphdr)[i] = ((const uint32_t *)cfghdr)[i];
    }

    tmphdr.boot_failure_counter = bootCnt;

    /* Erase NVM row (256 bytes) containing configuration header */
    int status = Flash_Access_Erase_Row((uint32_t *)cfghdr);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    /* Update configuration header and write back to flash memeory */
    for (uint32_t i = 0; i < (sizeof(imageConfigHeader_t) / sizeof(uint32_t)); i++)
    {
        ((uint32_t *)cfghdr)[i] = ((const uint32_t *)&tmphdr)[i];
    }

    status = Flash_Access_Write_Page();

    return status;
}

uint32_t Config_Get_Curr_Bl_Ver(void)
{
    const uint32_t blStartAddr = 0;
    const uint32_t blMetadataFwVesrionOffset = BL_IMAGE_METADATA_OFFSET + offsetof(bootloaderMetadata_t, bl_fw_version);
    return *((uint32_t *)(blStartAddr + blMetadataFwVesrionOffset));
}

/***********************************************************************
 * Functions:   Defininition of local functions
***********************************************************************/
static uint32_t getImageStartAddressOffset(uint32_t imageSlot)
{
    uint32_t imgOffset;
    switch (imageSlot)
    {
    case BOOT_SLOT_0:
        imgOffset = SLOT_0_IMGOFFSET;
        break;
    case BOOT_SLOT_1:
        imgOffset = SLOT_1_IMGOFFSET;
        break;    
    case BOOT_STANDALONE:
        imgOffset = STANDALONE_IMGOFFSET;
        break;
    default:
        imgOffset = -1; //only to make compiler happy, can't end up here
        break;
    }
    return imgOffset;
}
