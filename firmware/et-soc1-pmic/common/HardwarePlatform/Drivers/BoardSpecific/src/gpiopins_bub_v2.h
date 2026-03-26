/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file gpiopins_bub_v2.h
    \brief GPIO pins definition for BUB v2 board
*/
/***********************************************************************/

#ifndef __GPIOPINS_BUB_V2_H__
#define __GPIOPINS_BUB_V2_H__

#include <stdbool.h>
#include <stdint.h>
#include "gpio.h"

/***********************************************************************
 * GLOBAL Functions:   Declaration of global functions
***********************************************************************/
const gpioConfig_t *getGpioConfig_bub_v2(void);
const size_t getGpioConfigSize_bub_v2(void);
const gpioPinIdMap_t *getGpioPinIdMap_bub_v2(void);
const size_t getGpioPinIdMapSize_bub_v2(void);
void setEicHandler_bub_v2(uint8_t pin, EIC_handler_callback handler);
void setAdcHandler_bub_v2(uint8_t pin, ADC_handler_callback handler);

uint8_t pgPinFromEnPinId_bub_v2(gpioId_t enablePinId);

#endif //__GPIOPINS_BUB_V2_H__