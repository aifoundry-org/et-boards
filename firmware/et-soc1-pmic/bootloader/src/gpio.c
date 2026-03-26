/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file gpio.c
    \brief GPIO definition used in Bootloader
*/
/***********************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include "gpio.h"
#include "samd20.h"

/***********************************************************************
 * Macros:  Defininition of local macros
***********************************************************************/
/**
 * @brief Macros for the pin and port group, lower 5
 * bits are pin number in the group, higher 3
 * bits are the port group A, B etc
 */
#define GPIO_PIN(n)     ((n)&0x1Fu)
#define GPIO_PORT(n)    (((n)&0xFFu) >> 5)
#define GPIO(port, pin) ((((port)&0x7u) << 5) + ((pin)&0x1Fu))

#define GPIO_PIN_FUNCTION_OFF 0xff /**< \brief PORT Function Off (GPIO Mode) */

/***********************************************************************
 * Functions:   Declaration of local functions
***********************************************************************/
static void gpio_set_out_high(const uint8_t portpin);
static void gpio_set_out_low(const uint8_t portpin);
static void gpio_set_config(uint8_t portpin, uint8_t cfg, uint8_t useMask);
static void gpio_set_mux(uint8_t portpin, uint8_t function);

/***********************************************************************
 * GLOBAL Functions:   Defininition of global functions
***********************************************************************/
bool gpio_get_in(const uint8_t portpin)
{
    return (PORT->Group[GPIO_PORT(portpin)].IN.reg >> (portpin & 0x1F)) & 0x01;
}

uint8_t gpio_get_tristate(const uint8_t portpin)
{
    bool upst;
    bool downst;

    /* assume the pin dir has been configured as an input, which is the default */
    /* enable internal pull and input buffer */
    gpio_set_config(portpin, (PORT_PINCFG_INEN | PORT_PINCFG_PULLEN), (PORT_PINCFG_INEN | PORT_PINCFG_PULLEN));

    /* pull-up and read */
    gpio_set_out_high(portpin);
    upst = gpio_get_in(portpin);

    /* pull-down and read */
    gpio_set_out_low(portpin);
    downst = gpio_get_in(portpin);

    /* disable internal pull and input buffer */
    gpio_set_config(portpin, 0, (PORT_PINCFG_INEN | PORT_PINCFG_PULLEN));

    /* determine the pin state */
    if (upst && downst)
    {
        return _P;
    }
    else if (upst && !downst)
    {
        return _U;
    }
    else
    {
        return _G;
    }
}

char gpio_get_tri_char(const uint8_t portpin)
{
    switch (gpio_get_tristate(portpin))
    {
        case _P:
            return 'P';
        case _U:
            return 'U';
        default:
            return 'G';
    }
}

// modified not to change DRVSTR, PULLEN, INEN
void gpio_port_set_pin_function(const uint8_t portpin, const uint8_t pinfunction)
{
    gpio_set_config(portpin, -(pinfunction != GPIO_PIN_FUNCTION_OFF),
        PORT_PINCFG_PMUXEN); // set to PORT_PINCFG_PMUXEN if pinfunction!=GPIO_PIN_FUNCTION_OFF
    gpio_set_mux(portpin, pinfunction);
}

void gpio_set_dir_in(const uint8_t portpin)
{
    PORT->Group[GPIO_PORT(portpin)].DIRCLR.reg = 1U << GPIO_PIN(portpin); // set as input pin
    gpio_set_config(portpin, PORT_PINCFG_INEN, PORT_PINCFG_MASK);         // set INEN, clear DRVSTR, PULLEN, PMUXEN
}

void gpio_set_pull(const uint8_t portpin, const gpio_pull_t pull)
{
    gpio_set_config(portpin, -(pull != GPIO_PULL_OFF), PORT_PINCFG_PULLEN); // 0->0, 1->0xFF
    if (pull == GPIO_PULL_UP)
    {
        gpio_set_out_high(portpin);
    }
    else if (pull == GPIO_PULL_DOWN)
    {
        gpio_set_out_low(portpin);
    }
}

/***********************************************************************
 * Functions:   Definition of local functions
***********************************************************************/
static void gpio_set_out_high(const uint8_t portpin)
{
    PORT->Group[GPIO_PORT(portpin)].OUTSET.reg = 1U << GPIO_PIN(portpin);
}

static void gpio_set_out_low(const uint8_t portpin)
{
    PORT->Group[GPIO_PORT(portpin)].OUTCLR.reg = 1U << GPIO_PIN(portpin);
}

static void gpio_set_config(uint8_t portpin, uint8_t cfg, uint8_t useMask)
{
    uint8_t volatile *pcfg = &PORT->Group[GPIO_PORT(portpin)].PINCFG[GPIO_PIN(portpin)].reg;
    cfg &= PORT_PINCFG_MASK;
    *pcfg = (*pcfg & ~useMask) | (cfg & useMask);
}

static void gpio_set_mux(uint8_t portpin, uint8_t function)
{
    uint8_t volatile *pmux = &PORT->Group[GPIO_PORT(portpin)].PMUX[GPIO_PIN(portpin) >> 1].reg;
    function &= 0x0F;
    if ((portpin & 0x01) == 0)
    {
        *pmux = (*pmux & 0xF0) | function;
    }
    else
    {
        *pmux = (*pmux & 0xF) | (function << 4);
    }
}
