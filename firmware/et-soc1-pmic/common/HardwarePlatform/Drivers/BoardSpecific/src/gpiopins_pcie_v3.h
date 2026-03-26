/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file gpiopins_pcie_v3.h
    \brief GPIO pins definition for PCIE v3 board
*/
/***********************************************************************/

#ifndef __GPIOPINS_PCIE_V3_H__
#define __GPIOPINS_PCIE_V3_H__

#include <stdbool.h>
#include <stdint.h>
#include "gpio.h"

/***********************************************************************
 * GLOBAL Functions:   Declaration of global functions
***********************************************************************/
const gpioConfig_t *getGpioConfig_pcie_v3(void);
const size_t getGpioConfigSize_pcie_v3(void);
const gpioPinIdMap_t *getGpioPinIdMap_pcie_v3(void);
const size_t getGpioPinIdMapSize_pcie_v3(void);
void setEicHandler_pcie_v3(uint8_t pin, EIC_handler_callback handler);
void setAdcHandler_pcie_v3(uint8_t pin, ADC_handler_callback handler);

#endif //__GPIOPINS_PCIE_V3_H__