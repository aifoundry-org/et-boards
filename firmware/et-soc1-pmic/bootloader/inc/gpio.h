/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file gpio.h
    \brief GPIO definition used in Bootloader
*/
/***********************************************************************/

#ifndef __GPIO_H__
#define __GPIO_H__

#include <stdint.h>
#include <stdbool.h>
#include "samd20.h"

/***********************************************************************
 * GLOBAL Macros:  Defininition of global macros
***********************************************************************/
//Pins used for hw encoding
#define BRDID_0 PIN_PB13
#define BRDID_1 PIN_PB14
#define BRDID_2 PIN_PB15

/***********************************************************************
 * GLOBAL Data types:      Defininition of global data types
***********************************************************************/
typedef enum { GPIO_PULL_OFF = 0, GPIO_PULL_UP, GPIO_PULL_DOWN } gpio_pull_t;

enum {
    _G = 0, // Grounded
    _P,     // Pulled-up
    _U      // Unconnected
};

/***********************************************************************
 * GLOBAL Functions:   Declaration of global functions
***********************************************************************/
bool gpio_get_in(const uint8_t portpin);
uint8_t gpio_get_tristate(const uint8_t portpin);
char gpio_get_tri_char(const uint8_t portpin);
void gpio_port_set_pin_function(const uint8_t portpin, const uint8_t pinfunction);
void gpio_set_dir_in(const uint8_t portpin);
void gpio_set_pull(const uint8_t portpin, const gpio_pull_t pull);

#endif //__GPIO_H__
