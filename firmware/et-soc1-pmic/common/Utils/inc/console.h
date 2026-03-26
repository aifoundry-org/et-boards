/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file console.h
    \brief FreeRTOS uart console input
    This has the initialization, interrupt handler and blocking get line.
    This keeps a line of history, and recognizes shell-like manipulations ^p, ^a, ^f, ^b and ^e
*/
/***********************************************************************/

#ifndef __CONSOLE_H__
#define __CONSOLE_H__

#include "globals.h"
#ifndef USE_CONSOLE
#include "SEGGER_RTT.h"
#endif

typedef enum {
    CTRLC=0x03, CR='\r', LF='\n', DEL=0x7F,
    LT=0x84, RT=0x83, UP=0x81, DN=0x82,
    HOME=0x90, END=0xC0, PGUP=0xD0, PGDN=0xE0, INS=0xA0,
    F1=0x91, F2=0x92, F3=0X93, F4=0x94, F5=0x95, F6=0x97,
    F7=0x98, F8=0x99, F9=0xAA, F10=0xA1, F11=0xA3, F12=0xA4
} xtraKeyCode_t;

/* History algorithm:
 *
 * Finding line:
 *
 * if desiredLineNum < firstLineBufIdx --> not available
 * desiredLineNum --> modulo lineBegIdxBufSize --> index into lineBegIdxBuf
 * index into lineBegIdxBuf --> historyBuf --> null terminated string
 *
 * if allocating new space in historyBuf will overwrite entry, increment firstLineBufIdx to eliminate lost entries
 *
 * if new entry is exact copy of existing history, update all lineBegIdx to use new copy
 *
 * Adding line:
 *
 * ++lastLineNum
 * idx --> modulo lineBegIdxBufSize --> index into lineBegIdxBuf
 * strend(lineBegIdxBuf[endLineBufIdx % lineBegIdxBufSize]) --> where to store new line
 *   if end would exceed historyBuf[0historyBufSize start at historyBuf[0]
 * lineBegIdxBuf[endLineBufIdx++ % lineBegIdxBufSize] = store location
 * while( strend(store location) > lineBegIdxBuf[firstLineBufIdx% lineBegIdxBufSize] ) ++firstLineBufIdx
 *
 * if new entry is exact copy of existing history, update all lineBegIdx to use new copy
 *
 */

/*
 * @fn bool getLineFromConsoleQueue(char *line, size_t linelength)
 * @brief reads from console stream and filters carriage return/line feeds/backspace.
 * This is non-blocking and only called when characters are in stream buffer.
 * @note only used with FreeRTOS
 * @param[out] char *line - pointer to char line buffer array length must be LINE_BUFFER_SIZE bytes.
 *
 * @return bool true when line is complete and ready to process, false otherwise
 */

const char * StdGetLine( void (*eventHandler)(uint32_t event) );

bool CliGetLine(getline_t * pgl, void (*eventHandler)(uint32_t event) );

void glInit( getline_t * pgl, char * line, uint8_t bufsize,
            char * historyBuf, uint16_t historyBufSize,
            uint16_t * lineBegIdxBuf, uint16_t lineBegIdxBufSize,
            char const * histCmdName, char const * histCmdAltName );

/*
 * @fn printConsoleHistory()
 * @brief prints the history for the console
 */

void printConsoleHistory( getline_t * pgl );


#endif /* __CONSOLE_H__ */
