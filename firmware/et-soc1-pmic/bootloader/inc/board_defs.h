/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file board_defs.h
    \brief Local board definition for Bring up Board (BUB).
    For use with SAMD20J17A libraries asf3, CMSIS and FreeRTOS.
*/
/***********************************************************************/

#ifndef __BOARD_DEFS_H__
#define __BOARD_DEFS_H__

#include "samd20.h"

void system_init1(void);

#define GCLK3_FREQUENCY      32768UL
#define DFLL_MULTIPLY_FACTOR 1464
#define CPU_FREQUENCY        48000000
#define GCLK4_FREQUENCY      12000000
#define GCLK6_FREQUENCY      2000000

#define NVM_PAGE_COUNT FLASH_NB_OF_PAGES

#define EXTRACT_NVM_CALIB(pos, len) \
    ((*((uint32_t *)(NVMCTRL_OTP4) + ((pos) / 32)) >> ((pos) % 32)) & ((1 << (len)) - 1))

/*! \brief NVM CALIBRATION Offset Position and Size
*/
#define NVM_OSC32K_CAL_Pos       38
#define NVM_OSC32K_CAL_Len       7
#define NVM_DFLL_COARSE_Pos      58
#define NVM_DFLL_COARSE_Len      6
#define NVM_DFLL_FINE_Pos        64
#define NVM_DFLL_FINE_Len        10
#define NVM_ADC_LINEARITY_LO_Pos 27
#define NVM_ADC_LINEARITY_LO_Len 5
#define NVM_ADC_LINEARITY_HI_Pos 32
#define NVM_ADC_LINEARITY_HI_Len 3
#define NVM_ADC_BIAS_Pos         35
#define NVM_ADC_BIAS_Len         2

#endif // __BOARD_DEFS_H__
