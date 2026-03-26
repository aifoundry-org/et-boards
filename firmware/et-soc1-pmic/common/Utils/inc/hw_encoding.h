/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file hw_encoding.h
    \brief Functions related to hardware info encoding
by reading gpio pins connected to resistors.
*/
/***********************************************************************/

#include <stdint.h>

/***********************************************************************
 * GLOBAL data types:      Defininition of global data types
***********************************************************************/
typedef enum {
    HW_ENCODING_RESERVE_0 = 0, // Hardware encoding GGG/0b000 - unused
    HW_ENCODING_PCIE_1088,     // Hardware encoding UGP/0b?01 - Penguin RevB
    HW_ENCODING_RESERVE_2,     // Hardware encoding GPG/0b010 - unused
    HW_ENCODING_PCIE_V3_2,     // Hardware encoding GPP/0b011 - PCIE v3.2
    HW_ENCODING_RESERVE_4,     // Hardware encoding PGG/0b100 - unused
    HW_ENCODING_PCIE_V3_3,     // Hardware encoding PGP/0b101 - PCIE v3.3
    HW_ENCODING_BUB_REV_2,     // Hardware encoding PPG/0b110 - BUB2

    //must be the last
    HW_ENCODING_VERSION_MAX_COUNT
} hwEncodingVersion_t;

typedef enum {
    HW_ENCODING_BOARD_TYPE_PCIE_TEST = 0,
    HW_ENCODING_BOARD_TYPE_BUB = 1,
    HW_ENCODING_BOARD_TYPE_PCIE = 2,

    //must be the last
    HW_ENCODING_BOARD_TYPE_UNKNOWN = 100,
} hwEncodingBoardType_t;

typedef enum {
    HW_ENCODING_DESIGN_REV_0 = 0,
    HW_ENCODING_DESIGN_REV_1 = 1,
    HW_ENCODING_DESIGN_REV_2 = 2,
    HW_ENCODING_DESIGN_REV_3 = 3,
    HW_ENCODING_DESIGN_REV_4 = 4,
    HW_ENCODING_DESIGN_REV_6 = 6,
} hwEncodingDesignRevision_t;

/***********************************************************************
 * GLOBAL Functions:   Declaration of global functions
***********************************************************************/
void Hw_Encoding_Read_And_Store_Hw_Version_Info(void);
hwEncodingVersion_t Hw_Encoding_Get_Hw_Version_Encoding(void);
uint32_t Hw_Encoding_Get_Hw_Board_Type(void);
const char *Hw_Encoding_Get_Hw_Board_Name(void);
uint32_t Hw_Encoding_Get_Hw_Design_Revision(void);
uint32_t Hw_Encoding_Get_Hw_Modification_Revision(void);
uint32_t Hw_Encoding_Get_Hw_Board_Unique_Id(void);
uint32_t Hw_Encoding_Get_Supported_Board_Types(void);
