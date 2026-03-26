/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file boardinfo.h
    \brief Board related information.
*/
/***********************************************************************/

#ifndef __BOARDINFO_H__
#define __BOARDINFO_H__

#define CONSOLE_PORT        SERCOM0
#define CONSOLE_Handler     SERCOM0_Handler
#define CONSOLE_IRQn        SERCOM0_IRQn
#define CONSOLE_GCLK        GCLK_CLKCTRL_GEN_GCLK4
#define CONSOLE_GCLK_ID     GCLK_CLKCTRL_ID_SERCOM0_CORE
#define PM_APBCMASK_CONSOLE PM_APBCMASK_SERCOM0
#define CONSOLE_BAUDRATE    115200
#define CONSOLE_FREQUENCY   GCLK4_FREQUENCY
#define CONSOLE_TXPO        SERCOM_USART_CTRLA_TXPO_PAD2
#define CONSOLE_RXPO        SERCOM_USART_CTRLA_RXPO_PAD3

/* Pin Defines */
#define CONSOLE_RX PIN_PA07 /* SERCOM0 */
#define CONSOLE_TX PIN_PA06

/* Peripheral Pin Mux */

#define MUX_CONSOLE_RX MUX_PA07D_SERCOM0_PAD3 /* SERCOM 0 USART */
#define MUX_CONSOLE_TX MUX_PA06D_SERCOM0_PAD2

#endif // __BOARDINFO_H__