/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file config.h
    \brief Configuration space related data structures.
*/
/***********************************************************************/

#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <stdint.h>
#include <stddef.h>

/***********************************************************************
 * GLOBAL Macros:  Defininition of global macros
***********************************************************************/
/*! \def NUM_BOOT_SLOTS
    \brief Define for total number of boot slots
*/
#define NUM_BOOT_SLOTS 2

/*! \def BOOT_SLOT_0
    \brief Define for boot slot 0
*/
#define BOOT_SLOT_0 0

/*! \def BOOT_SLOT_1
    \brief Define for boot slot 1
*/
#define BOOT_SLOT_1 1

/*! \def BOOT_STANDALONE
    \brief Define for boot standalone image without bootloader
*/
#define BOOT_STANDALONE 2

/*! \def SLOT_0_IMGOFFSET
    \brief Define for image 0 address offset
*/
#define SLOT_0_IMGOFFSET BLSIZE //0x2000

/*! \def SLOT_1_IMGOFFSET
    \brief Define for image 1 address offset
*/
#define SLOT_1_IMGOFFSET (BLSIZE + IMAGESIZE) // 0x20800

/*! \def STANDALONE_IMAGE_IMGOFFSET
    \brief Define for standalone image address offset
*/
#define STANDALONE_IMGOFFSET (0)

/*! \def BLSIZE
    \brief Define for bootloader image size
*/
#define BLSIZE (8 << 10) // 8K

/*! \def IMAGESIZE
    \brief Define for PMIC firmware image size
*/
#define IMAGESIZE (122 << 10) // 122K

/*! \def FRUSIZE
    \brief Define for FRU info storage size as multiple of the flash ROW size
*/
#define FRUSIZE (1 * FLASH_ROW_SIZE)

/* FRU info is stored at the end of flash */
#define FRUOFFSET (FLASH_SIZE - FRUSIZE)

/*! \def CONFIG_HEADER_SIZE
    \brief Define for PMIC config header size
*/
#define CONFIG_HEADER_SIZE 0x100

/* Padding needs to be added to IVT to align it to 256 bytes
   configuration header is placed next to IVT which should be aligned to
   NVM row size(256). */
#define IVT_PAD  0x5C
#define IVT_SIZE (0xA4 + IVT_PAD) // D20 is 0xA4, D21 is 0xB4 reserve some extra space

_Static_assert(IVT_SIZE == CONFIG_HEADER_SIZE, "PCIe IVT size must be equal to CONFIG_HEADER_SIZE!");

/*! \def RT_IMAGE_METADATA_OFFSET
    \brief Offset of the PMIC image metadata calculated from the image start address 
*/
#define RT_IMAGE_METADATA_OFFSET (IVT_SIZE)

/*! \def BL_IMAGE_METADATA_OFFSET
    \brief Offset of the PMIC image metadata calculated from the image start address 
*/
#define BL_IMAGE_METADATA_OFFSET (IVT_SIZE)

/*! \def BLMAGICNUM
    \brief Used to determine which fw to boot in forced one time reboot - bootloader, slot0, or slot1
*/
#define BLMAGICNUM 0xB007B007

/***********************************************************************
 * GLOBAL data types:  Defininition of global data types
***********************************************************************/
/*! \typedef imageConfigHeader_
    \brief A structure that defines the different fields inside
    the image configuration header. This header is used primarily
    by the bootloader to read properties of the loadable images.
    \warning Config header has to be aligned to 256 byes which is NVM row size.
    This is needed so that the header can later be modified after erasing whole row.
    Config header is part of flash image and NVM can erase on row basis.
*/
typedef struct imageConfigHeader_ {
    uint32_t boot_slot;
    uint32_t boot_failure_counter;
} imageConfigHeader_t;

_Static_assert(
    sizeof(imageConfigHeader_t) <= CONFIG_HEADER_SIZE, "size of imageConfigHeader_t exceeded CONFIG_HEADER_SIZE!");

/*! \typedef bootloaderMetadata_
    \brief A structure that stores the PMIC bootloader image read only metadata.
*/
typedef struct bootloaderMetadata_ {
    uint32_t bl_fw_version; //set at compile time
    char hash[48];          //set at compile time
} bootloaderMetadata_t;

/*! \typedef rtMetadata_
    \brief A structure that stores the PMIC application image read only metadata.
*/
typedef struct rtMetadata_ {
    uint32_t
        start_addr; //set at compile time /* Todo: Might not be required in future after moving to relocatable code */
    uint32_t rt_fw_version;             //set at compile time
    uint32_t supported_board_types;     //set at compile time
    char hash[16];                      //set at compile time
    uint32_t checksum;                  //set in post build step
    uint32_t image_size;                //set in post build step
    uint32_t bl_fw_version;             //set at compile time
    uint32_t sp_pmic_interface_version; //set at compile time
    uint32_t metadata_version;          //set at compile time
    uint32_t build_type;                //set at compile time
} rtMetadata_t;

/***********************************************************************
 * GLOBAL functions:  Declaration of global functions
***********************************************************************/
uint32_t Config_Get_Metadata_Ver(uint32_t imageSlot);
uint32_t Config_Get_Image_Size_From_Metadata(uint32_t imageSlot);
uint32_t Config_Get_Supported_Board_Types_From_Metadata(uint32_t imageSlot);
uint32_t Config_Get_Fw_Ver_From_Metadata(uint32_t imageSlot);
uint32_t Config_Get_Compatible_Bl_Ver_From_Metadata(uint32_t imageSlot);
const char *Config_Get_Image_Hash_From_Metadata(uint32_t imageSlot);
uint32_t Config_Get_Sp_Pmic_Interface_Ver_From_Metadata(uint32_t imageSlot);
uint32_t Config_Get_Checksum_From_Metadata(uint32_t imageSlot);
uint32_t Config_Get_Build_Type_From_Metadata(uint32_t imageSlot);

uint32_t *Config_Get_Config_Header(void);
uint32_t Config_Get_Boot_Slot_From_Config_Header(void);
int Config_Set_Boot_Slot(uint32_t bootSlot);
uint32_t Config_Get_Failed_Boot_Cnt_From_Config_Header(void);
int Config_Set_Failed_Boot_Cnt(uint32_t bootCnt);

uint32_t Config_Get_Curr_Bl_Ver(void);

#endif // __CONFIG_H__
