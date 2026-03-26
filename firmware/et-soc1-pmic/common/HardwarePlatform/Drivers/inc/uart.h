/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/
/*! \file uart.h
    \brief SAM UART
*/
/***********************************************************************/

#ifndef __UART_H__
#define __UART_H__


// Define FEATURE_UART_CRLF if using CR/LF (/r/n) for end of line
// otherwise /n line feed only
// #define FEATURE_UART_CRLF
// Define FEATURE_UART_ECHO if incoming characters are echoed
// #define FEATURE_UART_ECHO

/**
 * @fn ConsoleInit()
 * @brief creates a static stream for console receive characters and enables interrupts.
 * @note only used with FreeRTOS > 10.1
 */
int ConsoleInit(void);

int putchar_( int ch );

#endif /* __USART_H__ */
