/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file uart.c
    \brief Basic blocking UART driver for SAMD51
*/
/***********************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "features.h"
#include "samd20.h"
#include "board_defs.h"
#include "globals.h"
#include "utils.h"
#include "uart.h"
#include "console.h"
#undef putchar


#ifdef USE_CONSOLE

#define STREAM_BUFFER_SIZE 16

int putchar(int c)
{
    while((CONSOLE_PORT->USART.INTFLAG.reg & SERCOM_USART_INTFLAG_DRE) == 0);
    CONSOLE_PORT->USART.DATA.reg = (uint8_t)c;
    return 1;
}

#else

int putchar(int c)
{
    return SEGGER_RTT_PutCharSkipNoLock( 0, (char)c );
}

#endif



#ifdef USE_CONSOLE

#define CONSOLEOUTBUFSIZE 512
typedef struct
{
    uint16_t inIdx;
    uint16_t outIdx;
    bool intrBusy;
    uint8_t buf[CONSOLEOUTBUFSIZE];
} consoleOut_t;

consoleOut_t volatile consoleOut;


int ConsoleInit(void)
{
    consoleOut.intrBusy = 0;
    consoleOut.inIdx = consoleOut.outIdx = 0;

    // create stream buffer for received characters
    static uint8_t streamStorage[STREAM_BUFFER_SIZE+1];
        static StaticStreamBuffer_t streamBuffer;
        globals.streamHandle = xStreamBufferCreateStatic(
                    STREAM_BUFFER_SIZE,
                    1, // trigger level
                    streamStorage,
                    &streamBuffer);

    NVIC_DisableIRQ(CONSOLE_IRQn);
    NVIC_ClearPendingIRQ(CONSOLE_IRQn);
    NVIC_EnableIRQ(CONSOLE_IRQn);

    CONSOLE_PORT->USART.INTFLAG.reg = SERCOM_USART_INTFLAG_RXC | SERCOM_USART_INTFLAG_DRE;
    CONSOLE_PORT->USART.INTENSET.reg = SERCOM_USART_INTENSET_RXC;
    memset( &globals.cli, 0, sizeof(cliGlobals_t) );
    globals.pgl->prompt = "";
    globals.putchByIntr = 1;

    return STATUS_SUCCESS;
}



void CONSOLE_Handler(void) /* ISR for Console SERCOM UART Receive Complete  */
{
    char ch;
    size_t bytesSent;
    BaseType_t higherPriorityTaskWoken = pdFALSE;
    SERCOM_USART_INTFLAG_Type flag = CONSOLE_PORT->USART.INTFLAG;

    if( flag.bit.DRE ) {
        if( consoleOut.inIdx != consoleOut.outIdx ) {
            consoleOut.intrBusy = 1;
//printfFromInterruptNoFlush( "intr %c - %d", consoleOut.buf[consoleOut.outIdx], consoleOut.outIdx );
            CONSOLE_PORT->USART.DATA.reg = consoleOut.buf[consoleOut.outIdx++];
            if( consoleOut.outIdx == countof(consoleOut.buf) ) consoleOut.outIdx = 0;
        } else {
            CONSOLE_PORT->USART.INTENCLR.reg = SERCOM_USART_INTENSET_DRE;
            CONSOLE_PORT->USART.INTFLAG.reg = SERCOM_USART_INTFLAG_DRE;
//printfFromInterruptNoFlush( "intr MT %d", consoleOut.outIdx, 0 );
            consoleOut.intrBusy = 0;
        }
    }
    if( flag.bit.RXC ) {
        // read the character, this also resets the interrupt flag
        ch = (char) CONSOLE_PORT->USART.DATA.reg;
//printfFromInterruptNoFlush("(%c %02X)", ch,ch);
        if (ch == CTRLC) {
            xTaskNotifyFromISR( globals.CLITaskHandle,
                                CONSOLE_ABORT_EVENT,
                                eSetBits,
                                &higherPriorityTaskWoken );
        } else {
            bytesSent =  xStreamBufferSendFromISR(  globals.streamHandle,
                                                    &ch,
                                                    1, // one byte sent
                                                    &higherPriorityTaskWoken);
            xTaskNotifyFromISR(
                                globals.CLITaskHandle,
                                (bytesSent > 0) ? CONSOLE_RX_EVENT : CONSOLE_OVERFLOW_EVENT,
                                eSetBits,
                                &higherPriorityTaskWoken );
        }
        portYIELD_FROM_ISR(higherPriorityTaskWoken);
    }
}
#else  /* USE_CONSOLE */


int ConsoleInit(void)
{
    SEGGER_RTT_Init();
    SEGGER_RTT_ConfigUpBuffer(0, NULL, NULL, 0, SEGGER_RTT_MODE_NO_BLOCK_SKIP);
    memset( &globals.cli, 0, sizeof(cliGlobals_t) );
    globals.pgl->prompt = "";
    
    return STATUS_SUCCESS;
}

#endif /* USE_CONSOLE */

int putchar_(int ch )
{
#ifdef FEATURE_UART_CRLF
    if( ch == '\n') {
        putcharRaw( '\r');
    }
#endif
    putcharRaw( ch );
    return 1;
}


uint16_t maxFillCnt = 0;
int putcharRaw(int ch )
{
//    threadLocal_t * pThreadLocal = pvTaskGetThreadLocalStoragePointer( NULL, 0 );
//    if( pThreadLocal->threadId == TH_CLI) printfFromInterruptNoFlush("[%c %02X]", ch,ch);
    #ifdef USE_CONSOLE
    if( !globals.putchByIntr )
    {
        while(!CONSOLE_PORT->USART.INTFLAG.bit.DRE);
        CONSOLE_PORT->USART.DATA.reg = (uint8_t) ch;
        return 1;
    }
    if( CONSOLE_PORT->USART.INTFLAG.bit.DRE && !consoleOut.intrBusy ) {
//printfFromInterruptNoFlush( "write direct %c - %d", (uint8_t)ch, consoleOut.inIdx );
        CONSOLE_PORT->USART.DATA.reg = (uint8_t) ch;
        CONSOLE_PORT->USART.INTENSET.reg = SERCOM_USART_INTENSET_DRE;
    } else {
        uint16_t t = consoleOut.outIdx;
        uint16_t fillcnt = consoleOut.inIdx -t;
        if( (int16_t)fillcnt < 0 ) fillcnt += countof(consoleOut.buf);
        if( fillcnt > maxFillCnt ) maxFillCnt = fillcnt;
        if( fillcnt >= countof(consoleOut.buf) -1 ) {
            while( consoleOut.inIdx != consoleOut.outIdx )  // wait until completely empty
               taskYIELD();  // only yields to higher priority tasks - make sure I2C can run
        }

//printfFromInterruptNoFlush( "queue %c - %d", ch, consoleOut.inIdx );
        consoleOut.buf[consoleOut.inIdx++] = (uint8_t) ch;
              if( consoleOut.inIdx == countof(consoleOut.buf) ) consoleOut.inIdx = 0;

        if( CONSOLE_PORT->USART.INTFLAG.bit.DRE && !consoleOut.intrBusy ) {
//printfFromInterruptNoFlush( "unqueue %c - %d", consoleOut.buf[consoleOut.outIdx], consoleOut.outIdx );
            uint8_t cho = consoleOut.buf[consoleOut.outIdx++];
            if( consoleOut.outIdx == countof(consoleOut.buf) ) consoleOut.outIdx = 0;
            CONSOLE_PORT->USART.DATA.reg = cho;
            CONSOLE_PORT->USART.INTENSET.reg = SERCOM_USART_INTENSET_DRE;
        }
    }
#else
    SEGGER_RTT_PutCharSkipNoLock( 0, ch );
#endif
    return 1;
}



