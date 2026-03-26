/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file gpiopins_penguin_revb.h
    \brief GPIO pins definition for Penguin RevB board
*/
/***********************************************************************/

#ifndef __GPIOPINS_PENGUIN_REVB_H__
#define __GPIOPINS_PENGUIN_REVB_H__

#include <stdbool.h>
#include <stdint.h>
#include "gpio.h"

/***********************************************************************
 * GLOBAL Functions:   Declaration of global functions
***********************************************************************/
const gpioConfig_t *getGpioConfig_penguin_revb(void);
const size_t getGpioConfigSize_penguin_revb(void);
const gpioPinIdMap_t *getGpioPinIdMap_penguin_revb(void);
const size_t getGpioPinIdMapSize_penguin_revb(void);
void setEicHandler_penguin_revb(uint8_t pin, EIC_handler_callback handler);
void setAdcHandler_penguin_revb(uint8_t pin, ADC_handler_callback handler);

uint8_t pgPinFromEnPinId_penguin_revb(gpioId_t enablePinId);
#endif //__GPIOPINS_PENGUIN_REVB_H__