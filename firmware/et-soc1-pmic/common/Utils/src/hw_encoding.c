/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file hw_encoding.c
    \brief Functions related to hardware info encoding
by reading gpio pins connected to resistors.
*/
/***********************************************************************/

#include <stdint.h>
#include "hw_encoding.h"
#include "gpio.h"

/***********************************************************************
 * Data types:      Defininition of local data types
***********************************************************************/
typedef enum {
    HW_ENCODING_MODIFICATION_REV_0 = 0,
    HW_ENCODING_MODIFICATION_REV_1,
    HW_ENCODING_MODIFICATION_REV_2,
    HW_ENCODING_MODIFICATION_REV_3,
    HW_ENCODING_MODIFICATION_REV_4,
    HW_ENCODING_MODIFICATION_REV_5,
} hwEncodingModificationRevision_t;

#define BOARD_ID(b2, b1, b0) ((uint8_t)(((b2) << 4) | ((b1) << 2) | (b0)))
typedef struct {
    uint8_t boardId;
    hwEncodingBoardType_t boardType;
    hwEncodingDesignRevision_t designRevision;
    hwEncodingModificationRevision_t modificationRevision;
    const char *boardName;
} hwEncodingBoardVersion_t;

/***********************************************************************
 * Variables:  Defininition of local variables and constants
***********************************************************************/
// clang-format off
static const hwEncodingBoardVersion_t hwBoardVersion[HW_ENCODING_VERSION_MAX_COUNT] = {
    [HW_ENCODING_RESERVE_0] = { BOARD_ID(_G,_G,_G), HW_ENCODING_BOARD_TYPE_BUB,  HW_ENCODING_DESIGN_REV_0, HW_ENCODING_MODIFICATION_REV_0, "BUB"       }, // Hardware encoding GGG/0b000 - unused
    [HW_ENCODING_PCIE_1088] = { BOARD_ID(_U,_G,_P), HW_ENCODING_BOARD_TYPE_PCIE, HW_ENCODING_DESIGN_REV_2, HW_ENCODING_MODIFICATION_REV_0, "PCIE-1088" }, // Hardware encoding UGP/0b?01 - Penguin RevB; New SRAM VR, TPSM8D6C24
    [HW_ENCODING_RESERVE_2] = { BOARD_ID(_G,_P,_G), HW_ENCODING_BOARD_TYPE_UNKNOWN, HW_ENCODING_DESIGN_REV_0, HW_ENCODING_MODIFICATION_REV_0, "UNKNOWN TYPE" }, // Hardware encoding GPG/0b010 - unused
    [HW_ENCODING_PCIE_V3_2] = { BOARD_ID(_G,_P,_P), HW_ENCODING_BOARD_TYPE_PCIE, HW_ENCODING_DESIGN_REV_3, HW_ENCODING_MODIFICATION_REV_2, "PCIE"      }, // Hardware encoding GPP/0b011 - PCIE v3.2
    [HW_ENCODING_RESERVE_4] = { BOARD_ID(_P,_G,_G), HW_ENCODING_BOARD_TYPE_UNKNOWN, HW_ENCODING_DESIGN_REV_0, HW_ENCODING_MODIFICATION_REV_0, "UNKNOWN TYPE" }, // Hardware encoding PGG/0b100 - unused
    [HW_ENCODING_PCIE_V3_3] = { BOARD_ID(_P,_G,_P), HW_ENCODING_BOARD_TYPE_PCIE, HW_ENCODING_DESIGN_REV_3, HW_ENCODING_MODIFICATION_REV_3, "PCIE"      }, // Hardware encoding PGP/0b101 - PCIE v3.3
    [HW_ENCODING_BUB_REV_2] = { BOARD_ID(_P,_P,_G), HW_ENCODING_BOARD_TYPE_BUB,  HW_ENCODING_DESIGN_REV_6, HW_ENCODING_MODIFICATION_REV_0, "BUB"       }, // Hardware encoding PPG/0b110 - BUB2
};
// clang-format on

static uint32_t hwVersionEncoding;

/***********************************************************************
 * Functions:   Declaration of local functions
***********************************************************************/
static uint8_t readHwEncodingResistors(void);

/***********************************************************************
 * GLOBAL Functions:   Defininition of global functions
***********************************************************************/
void Hw_Encoding_Read_And_Store_Hw_Version_Info(void)
{
    hwVersionEncoding = readHwEncodingResistors();
}

hwEncodingVersion_t Hw_Encoding_Get_Hw_Version_Encoding(void)
{
    return hwVersionEncoding;
}

uint32_t Hw_Encoding_Get_Hw_Board_Type(void)
{
    hwEncodingBoardType_t boardType = hwBoardVersion[hwVersionEncoding].boardType;
    if (boardType == HW_ENCODING_BOARD_TYPE_UNKNOWN)
    {
        boardType =
            HW_ENCODING_BOARD_TYPE_BUB; //BUB board encoding is not unified, so we assume BUB if hw encoding is unknown
    }
    return boardType;
}

const char *Hw_Encoding_Get_Hw_Board_Name(void)
{
    return hwBoardVersion[hwVersionEncoding].boardName;
}

uint32_t Hw_Encoding_Get_Hw_Design_Revision(void)
{
    return hwBoardVersion[hwVersionEncoding].designRevision;
}

uint32_t Hw_Encoding_Get_Hw_Modification_Revision(void)
{
    return hwBoardVersion[hwVersionEncoding].modificationRevision;
}

uint32_t Hw_Encoding_Get_Hw_Board_Unique_Id(void)
{
    //TODO SW-18226 Read board unique id from dedicated memory location.
    return 0;
}

uint32_t Hw_Encoding_Get_Supported_Board_Types(void)
{
    uint32_t supportedBoardTypes = 0;
    for (int i = 0; i < HW_ENCODING_VERSION_MAX_COUNT; i++)
    {
        if (hwBoardVersion[i].boardType != HW_ENCODING_BOARD_TYPE_UNKNOWN)
        {
            supportedBoardTypes |= 1 << i;
        }
    }
    return supportedBoardTypes;
}

/***********************************************************************
 * Functions:   Defininition of local functions
***********************************************************************/
static uint8_t readHwEncodingResistors(void)
{
    uint8_t boardid = BOARD_ID(gpio_get_tristate(BRDID_2), gpio_get_tristate(BRDID_1), gpio_get_tristate(BRDID_0));

    for (uint8_t i = 0; i < HW_ENCODING_VERSION_MAX_COUNT; i++)
    {
        if (hwBoardVersion[i].boardId == boardid)
        {
            return i;
        }
    }
    return HW_ENCODING_BUB_REV_2; // default to be BUB v2
}
