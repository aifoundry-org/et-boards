/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file system_init.c
    \brief C file for SAMD20 initialization for Bring Up Board (BUB).
*/
/***********************************************************************/

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "gpio.h"
#include "board_defs.h"
#include "system_clock.h"
#include "boardinfo.h"

#define UART_BAUD(rate, freq) (65536 - ((1024 * (rate)) / ((freq) / 1024)))

// I2C Baud rate calculations
#define TRISE 300
#define I2CM_BAUDTOTAL(rate, freq) \
    (((freq) - (rate * 10) - (TRISE * ((rate) / 1000) * ((freq) / 1000)) / 1000) / (rate))
#define I2CM_BAUDLOW(rate, freq) ((2 * I2CM_BAUDTOTAL(rate, freq)) / 3)
#define I2CM_BAUD(rate, freq)    (I2CM_BAUDTOTAL(rate, freq) - I2CM_BAUDLOW(rate, freq))

/**
 * @brief Call system_init() prior to anything else in main()
 * this configures all peripherals with the exception of NVIC
 * NVIC enables are located in seperate init functions usually located
 * with the service routine. These inits must be called from main after system_init()
 */
void system_init1(void)
{
    SYSCTRL->INTFLAG.reg = SYSCTRL_INTFLAG_BOD33RDY | SYSCTRL_INTFLAG_BOD33DET | SYSCTRL_INTFLAG_DFLLRDY;
    // Start 32KHz oscillator for DFLL Reference
    system_clock_osc32k_setup();
    // flash wait states at 3.3V, 1 for 48MHz Clock, 0 for 8MHz or less
    // flash wait states at 1.8V, 3 for 48MHz Clock, 0 for 8MHz or less

    NVMCTRL->CTRLB.bit.RWS = 3;

    /* Initialize GCLK */
    // GCLK 0 48MHz DFLL to CPU
    // GCLK 3 OSC32 (32.768KHz) to DFLL Reference
    gclk_write_GENDIV(GCLK_GENDIV_ID_GCLK3 | GCLK_GENDIV_DIV(1));
    gclk_write_GENCTRL(GCLK_GENCTRL_ID_GCLK3 | GCLK_GENCTRL_GENEN | GCLK_GENCTRL_SRC_OSC32K);
    gclk_wait_for_sync();
    gclk_write_CLKCTRL(GCLK_CLKCTRL_GEN_GCLK3 | GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_ID_DFLL48M);

    system_clock_dfll_setup();
    gclk_write_GENDIV(GCLK_GENDIV_ID_GCLK0 | GCLK_GENDIV_DIV(1));
    gclk_write_GENCTRL(GCLK_GENCTRL_ID_GCLK0 | GCLK_GENCTRL_GENEN | GCLK_GENCTRL_SRC_DFLL48M);
    gclk_wait_for_sync();

    // GCLK 4 (12MHz)
    gclk_write_GENDIV(GCLK_GENDIV_ID_GCLK4 | GCLK_GENDIV_DIV(4));
    gclk_write_GENCTRL(GCLK_GENCTRL_ID_GCLK4 | GCLK_GENCTRL_GENEN | GCLK_GENCTRL_OE | GCLK_GENCTRL_SRC_DFLL48M);
    gclk_wait_for_sync();

    // GCLK6 (1MHz)
    gclk_write_GENDIV(GCLK_GENDIV_ID_GCLK6 | GCLK_GENDIV_DIV(48));
    gclk_write_GENCTRL(GCLK_GENCTRL_ID_GCLK6 | GCLK_GENCTRL_GENEN | GCLK_GENCTRL_SRC_DFLL48M);
    gclk_wait_for_sync();
    // Slow clock for all SERCOMs - not really needed
    gclk_write_CLKCTRL(GCLK_CLKCTRL_GEN_GCLK6 | GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_ID_SERCOMX_SLOW);
    // EIC
    gclk_write_CLKCTRL(GCLK_CLKCTRL_GEN_GCLK4 // 12MHz
                       | GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_ID_EIC);

    // Enable power to modules
    PM->APBAMASK.reg |= PM_APBAMASK_EIC;
    PM->APBBMASK.reg |= PM_APBBMASK_NVMCTRL;
    PM->APBCMASK.reg |= 0 | PM_APBCMASK_CONSOLE; // terminate PM->APBCMASK.reg |=
                                                 /*
gclk_write_CLKCTRL(GCLK_CLKCTRL_GEN_GCLK6
                | GCLK_CLKCTRL_CLKEN
                | GCLK_CLKCTRL_ID_EVSYS_CHANNEL_1); */

    //NVIC_SetPriority(SERCOM0_IRQn, 3);

    /* Console UART */
    // 12MHz to Console UART
    gclk_write_CLKCTRL(CONSOLE_GCLK | GCLK_CLKCTRL_CLKEN | CONSOLE_GCLK_ID);
    // reset Console UART and wait for it to complete, enable is cleared
    CONSOLE_PORT->USART.CTRLA.reg = SERCOM_USART_CTRLA_SWRST;
    while (CONSOLE_PORT->USART.CTRLA.reg & SERCOM_USART_CTRLA_SWRST)
        ;
    CONSOLE_PORT->USART.CTRLA.reg = SERCOM_USART_CTRLA_MODE_USART_INT_CLK | SERCOM_USART_CTRLA_TXPO_PAD2 |
                                    SERCOM_USART_CTRLA_RXPO_PAD3 | SERCOM_USART_CTRLA_FORM_0 // No parity
                                    | SERCOM_USART_CTRLA_DORD;                               // LSB first
    CONSOLE_PORT->USART.CTRLB.reg = SERCOM_USART_CTRLB_TXEN | SERCOM_USART_CTRLB_RXEN |
                                    SERCOM_USART_CTRLB_CHSIZE(0); // 8 bit

    CONSOLE_PORT->USART.BAUD.reg = UART_BAUD(CONSOLE_BAUDRATE, CONSOLE_FREQUENCY);

    gpio_port_set_pin_function(CONSOLE_RX, MUX_CONSOLE_RX);
    gpio_port_set_pin_function(CONSOLE_TX, MUX_CONSOLE_TX);

    CONSOLE_PORT->USART.CTRLA.reg |= SERCOM_USART_CTRLA_ENABLE;

    NVMCTRL->CTRLB.reg = NVMCTRL_CTRLB_RWS(3) | NVMCTRL_CTRLB_MANW;
    NVMCTRL->INTENCLR.reg = NVMCTRL_INTFLAG_READY | NVMCTRL_INTFLAG_ERROR;
}
