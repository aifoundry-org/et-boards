/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file version.h
    \brief Versioning related.
*/

#ifndef VERSION_H_
#define VERSION_H_

#include <stdint.h>

//Bootloader
#define BL_FW_VERSION_H 1
#define BL_FW_VERSION_M 0
#define BL_FW_VERSION_L 1

//Runtime application
#define RT_FW_VERSION_H 1
#define RT_FW_VERSION_M 7
#define RT_FW_VERSION_L 0

#define GET_VERSION_MAJOR(fw_ver) (((fw_ver) >> 16) & 0xFF)
#define GET_VERSION_MINOR(fw_ver) (((fw_ver) >> 8) & 0xFF)
#define GET_VERSION_PATCH(fw_ver) ((fw_ver)&0xFF)

#endif /* VERSION_H_ */
