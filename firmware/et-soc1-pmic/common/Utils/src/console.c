/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file console.c
    \brief freeRTOS uart console input
    \details This has the initialization, interrupt handler and blocking get pgl->line
   This keeps a pgl->line of history, and recognizes shell-like manipulations ^p, ^a, ^f, ^b, ^k and ^e
   Sending escape sends a message directly to the task to abort
   You must define an alias for the name of the ConsoleTaskHandle in FreeRTOSconfig.h file
   #define ConsoleTaskHandle CLITaskHandle
   You must define an alias for the size of the pgl->line buffer used
   #define LINE_BUFFER_SIZE 60
   You must define an alias for the number of history buffers you want to keep around
   This must be two or more.
   #define HISTORY_BUFFER_COUNT 2
*/
/***********************************************************************/

#include "features.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include "FreeRTOS.h"
#include "stream_buffer.h"
#include "features.h"
#include "console.h"
#include "globals.h"
#include "board_defs.h"
#include "hooks.h"
#include "CLITask.h"
#include "cli.h"
#include "utils.h"

typedef enum { EM_idle, EM_gotEsc, EM_gotLSqB, EM_awaitTilda, } state_t;

void glInit( getline_t * pgl, char * line, uint8_t bufsize,
            char * historyBuf, uint16_t historyBufSize,
            uint16_t * lineBegIdxBuf, uint16_t lineBegIdxBufSize,
            char const * histCmdName, char const * histCmdAltName )
{
    pgl->line = line;
    pgl->curpos = pgl->curlen = 0;
    pgl->bufsize = bufsize;
    pgl->newLine = false;
    pgl->state = EM_idle;
    pgl->historyBuf = historyBuf;
    pgl->historyBufSize = historyBufSize;
    pgl->historyBufNextIdx = 0;
    pgl->lineBegIdxBuf = lineBegIdxBuf;
    pgl->lineBegIdxBufSize = lineBegIdxBufSize;
    pgl->firstLineBufIdx = 1;
    pgl->thisHistLine = pgl->endLineBufIdx = 1;
    pgl->histCmdName = histCmdName;
    pgl->histCmdAltName = histCmdAltName;
}

const char * StdGetLine( void (*eventHandler)(uint32_t event) )
{
    char ch;
    static char line[80];
    char * p = line;
    bool const echo = 0;
    
    while (1) {
        uint32_t event;
        xTaskNotifyWait(    0x00,           /* Don't clear any notification bits on entry. */
                            ULONG_MAX,      /* Reset the notification value to 0 on exit. */
                            &event,         /* Notified value pass out in ulNotifiedValue. */
                            portMAX_DELAY   /* Block indefinitely. */
                                            );
        if( event & ~CONSOLE_RX_EVENT && eventHandler ) {
            eventHandler( event );
        }
        if( !(event & CONSOLE_RX_EVENT) ) {
            continue;
        }
        while( !xStreamBufferIsEmpty( globals.streamHandle ) ) {
            xStreamBufferReceive( globals.streamHandle, &ch, sizeof(char), 0);
            
            if( p >= endof(line)-1 ) {
                *p = '\0';
                continue;
            }                

            if( ch == '\r' ) {
                *p = '\0';
                if( echo ) puts( "\r\n" );
                return line;
            }
            if( ch=='\b' || ch==0x7F ) {
                if( p>line ) {
                    --p;
                    if( echo ) puts( "\b \b" );
                }
                continue;
            }
            if( p<endof(line) && ' '<= ch && ch<='~') {
                *p++ = ch;
                if( echo ) putchar( ch );
            }
        }
    }
}


// non-blocking read from console stream. Does all filtering and
// returns true when line is complete and null terminated.
bool CliGetLine(getline_t * pgl, void (*eventHandler)(uint32_t event) )
{
    char ch;

    if (pgl->newLine) {
        pgl->newLine = false;
        pgl->curpos = pgl->curlen = 0;
        pgl->line[0] = '\0';
    }
#ifdef USE_CONSOLE
    while (1) {
        uint32_t event;
        xTaskNotifyWait(    0x00,           /* Don't clear any notification bits on entry. */
                            ULONG_MAX,      /* Reset the notification value to 0 on exit. */
                            &event,         /* Notified value pass out in ulNotifiedValue. */
                            portMAX_DELAY   /* Block indefinitely. */
                                            );
        if( event & ~CONSOLE_RX_EVENT && eventHandler ) {
            eventHandler( event );
        }        
        if( !(event & CONSOLE_RX_EVENT) ) {
            continue;
        }
        while( !xStreamBufferIsEmpty( globals.streamHandle ) ) {
            xStreamBufferReceive( globals.streamHandle, &ch, sizeof(char), 0);
        
#else
    while ( (ch = SEGGER_RTT_WaitKey()) != 0 ) {
        OSDELAY_MS(1)
        {
#endif
        if( pgl->state != EM_idle ) // have receives ESC - multi byte mode
        {
            switch(pgl->state)
            {
                case EM_gotEsc:
                    if( ch != '[' ) { // error abandon multi byte mode
                        pgl->state = EM_idle;
                        break; //deliver code
                    }
                    pgl->state = EM_gotLSqB;
                    continue; // drop char and continue
                case EM_gotLSqB:
                    if( 'A'<=ch && ch<='D' ) {
                        ch = 0x80 + ch-'@';
                        pgl->state = EM_idle;
                        break; //deliver code
                    }
                    if( '1'<=ch && ch<='6' ) {
                        pgl->eMcode = (ch-'0') <<4;
                        pgl->state = EM_awaitTilda;
                        continue; // drop char and continue
                    }
                    // else error abandon multi byte mode
                    pgl->state = EM_idle;
                    break; //deliver code
                case EM_awaitTilda:
                    if( ch == '~' ) { // '~' terminates
                        ch = pgl->eMcode + 0x80;
                        break; //deliver code
                    }
                    if( pgl->eMcode & ((1<<4)-1) ) { // already seen two chars
                        // error abandon multi byte mode
                        pgl->state = EM_idle;
                        break; //deliver code
                    }
                    if( '0'<=ch && ch<='9' ) {
                        ch -= '0';
                        if( ch == 0 )
                            ch = 0x0A; // differentiate '0' from no character
                        pgl->eMcode += ch;
                        continue; // drop char and continue
                    }
                    // else error abandon multi byte mode
                    pgl->state = EM_idle;
                    break; //deliver code
            } // end switch
        } // end ( pgl->state != EM_idle )

        PRINTSEM_TAKE
        uint8_t nAfter = pgl->curlen -pgl->curpos;
        switch (ch) { // process editing codes and characters
            case '\e':
                pgl->state = EM_gotEsc; // enter multi byte mode
                break;
            case '\b':
                if (pgl->curpos > 0) {
                    --pgl->curpos;
                    --pgl->curlen;
                    memmove( &pgl->line[pgl->curpos], &pgl->line[pgl->curpos+1], nAfter+1 );
                    repeatChar( '\b', 1 );
                    puts(&pgl->line[pgl->curpos]);
                    repeatChar( ' ', 1 );
                    repeatChar( '\b', nAfter +1);
                }
                break;
            case DEL: // delete next character
                if (pgl->curpos < pgl->curlen ) {
                    --pgl->curlen;
                    memmove( &pgl->line[pgl->curpos], &pgl->line[pgl->curpos+1], nAfter );
                    puts( &pgl->line[pgl->curpos]);
                    putchar( ' ' );
                    repeatChar( '\b', nAfter );
                  }
                break;
            case CR:
            case LF:
                // check for History command
                if( strncasecmp( pgl->line, pgl->histCmdName, strlen(pgl->histCmdName) )==0 ||
                        strncasecmp( pgl->line, pgl->histCmdAltName, strlen(pgl->histCmdAltName ) )==0 ) {
                    puts("\n");
                    printConsoleHistory( pgl );
                    pgl->newLine = true;
                    pgl->curlen = 0;
                    PRINTSEM_GIVE
                    return 0;
                }
                // Handle history buffer

                // handle !nnn history replacements
                if( pgl->curlen != 0 ) {
                    do {
                        if( pgl->line[0] != '!' )
                            break;
                        bool ok=0;
                        uint32_t n = atodec( pgl->line+1, &ok );
                        if( !ok )
                            break;
                        if( pgl->firstLineBufIdx>n || n>=pgl->endLineBufIdx )
                            break;
                        char const * hline = &pgl->historyBuf[pgl->lineBegIdxBuf[n]];
                        pgl->curlen = strlen(hline);
                        memcpy( pgl->line, hline, pgl->curlen );
                    } while(0);
                    if( pgl->line[0] == '!' ) {
                        puts( "Bad !nnn\n");
                        pgl->newLine = true;
                        pgl->curlen = 0;
                        PRINTSEM_GIVE
                        return true;
                    }

                    // find where to store the new line in lineBegIdxBuf
                    if( (pgl->endLineBufIdx % pgl->lineBegIdxBufSize) == ((pgl->firstLineBufIdx-1) % pgl->lineBegIdxBufSize) )
                        ++pgl->firstLineBufIdx;         // drop firstLineBufIdx entry if lineBegIdxBuf is full
                    uint16_t nextLineBufIdx = pgl->endLineBufIdx;

                    uint16_t realNextHistBufIdx = pgl->historyBufNextIdx;
                    uint16_t nextHistBufIdx = realNextHistBufIdx;
                    uint16_t realEndNextHistBufIdx = nextHistBufIdx + pgl->curlen+1;
                    uint16_t endNextHistBufIdx = realEndNextHistBufIdx;
                    // if line goes beyond end of historyBuf
                    bool newwrap = realEndNextHistBufIdx >  pgl->historyBufSize;
                    if( newwrap ) {
                        nextHistBufIdx = 0;                          // put it at beginning
                        endNextHistBufIdx = pgl->curlen+1;
                    }

                    // store new line in historyBuf
                    memcpy( &pgl->historyBuf[nextHistBufIdx], pgl->line, pgl->curlen );
                    pgl->historyBuf[endNextHistBufIdx-1] = '\0';
                    pgl->historyBufNextIdx = endNextHistBufIdx;

                    // add entry to lineBegIdxBuf, update pgl->endLineBufIdx
                    pgl->lineBegIdxBuf[pgl->endLineBufIdx++ % pgl->lineBegIdxBufSize] = nextHistBufIdx;

                    // drop lineBegIdx entries from beginning whose historyBuf entry is lost
                    uint16_t nb = nextHistBufIdx  ;         // new beginning
                    uint16_t ne = endNextHistBufIdx;        // new end - new entry never wraps
                    // effective previous end = next beginning or previous end of all entries
                    uint16_t e = realNextHistBufIdx;
                    uint16_t b = 0/*set later*/;          // movable beginning of all entries
                    // |--------|  b==e     -> False (unless empty - first==last)
                    // |b--e    |  b==0             -> nb>=e
                    // |    b--e|  e==0             -> ne<=b
                    // |  b--e  |  else             -> nb>=e || ne<=b
                    while(1) {
                        b = pgl->lineBegIdxBuf[pgl->firstLineBufIdx % pgl->lineBegIdxBufSize];
                        bool ok = 0;
                        do {
                            if( b==e)           { ok = pgl->firstLineBufIdx==pgl->endLineBufIdx-1; break; }
                            if( b==0 )          { ok = nb>=e; break; }
                            if( e==0 )          { ok = ne<=b; break; }
                            /*else*/            { ok = nb>=e || ne<=b; break; }
                        } while(0);
                        if( ok ) // nb--ne not inside b--e considering wrapping
                            break; // that is the required condition
                        pgl->lineBegIdxBuf[pgl->firstLineBufIdx++ % pgl->lineBegIdxBufSize ] = (uint16_t)0xFFFF;  // drop a lineBegIdx from beginning
                    }

                    // look for lineBegIdx entries whose historyBuf entry matches new line
                    // and change to point to new entry
                    for( uint16_t idx=pgl->firstLineBufIdx; idx<nextLineBufIdx;  ++idx ) {
                        uint16_t * pEntry = &pgl->lineBegIdxBuf[idx % pgl->lineBegIdxBufSize];
                        if( strcmp( &pgl->historyBuf[nextHistBufIdx], &pgl->historyBuf[*pEntry] )== 0 )
                            *pEntry = nextHistBufIdx;
                    }
                    pgl->thisHistLine = pgl->endLineBufIdx;
                } // end history section
                checkEndInterrupted();
                puts("\n");
                pgl->curpos = pgl->curlen = 0;
                pgl->newLine = true;
                PRINTSEM_GIVE
                return true;
            case RT: // forward a character
                if (pgl->curpos < pgl->curlen ) {
                    repeatChar(  pgl->line[pgl->curpos++], 1 );
                }
                break;
            case LT: // move cursor back a character
                if (pgl->curpos > 0) {
                    putchar( '\b' );
                    pgl->curpos--;
                }
                break;
            case HOME: // to the beginning of pgl->line
                repeatChar( '\b', pgl->curpos );
                pgl->curpos = 0;
                break;
            case END: // to the end of line
                puts( &pgl->line[pgl->curpos]);
                pgl->curlen = pgl->curlen;
                break;
            case F10: // kill to end of line
                repeatChar( ' ', nAfter );
                repeatChar( '\b', nAfter );
                pgl->line[pgl->curpos] = '\0';
                pgl->curlen = pgl->curpos;
                break;
            case UP: // to previous line
                if( pgl->thisHistLine == pgl->firstLineBufIdx )
                    break;
                repeatChar( '\b', pgl->curpos );
                repeatChar( ' ', pgl->curlen );
                repeatChar( '\b', pgl->curlen );
                {
                    char const * hline = &pgl->historyBuf[pgl->lineBegIdxBuf[--pgl->thisHistLine]];
                    pgl->curlen = pgl->curpos = strlen(hline);
                    memcpy( pgl->line, hline, pgl->curlen+1 );
                    puts( hline );
                }
                break;
            case DN: // to next line
                if( pgl->thisHistLine == pgl->endLineBufIdx )
                    break;
                repeatChar( '\b', pgl->curpos );
                repeatChar( ' ', pgl->curlen );
                repeatChar( '\b', pgl->curlen );
                if( pgl->thisHistLine != pgl->endLineBufIdx-1 )                {
                    char const * hline = &pgl->historyBuf[pgl->lineBegIdxBuf[++pgl->thisHistLine]];
                    pgl->curlen = pgl->curpos = strlen(hline);
                    memcpy( pgl->line, hline, pgl->curlen+1 );
                    puts( hline );
                }
                else {
                    pgl->curlen = pgl->curpos = 0;
                }
                break;
            default:
                if (ch < 0x20 || ch >= 0x7F )
                    break;
                if (pgl->curpos >= pgl->bufsize-1 ) {
                    putchar('\a'); // ring a bell - overflow
                    break;
                }
                if( nAfter==0 ) { // the character is at the end of pgl->line
                    pgl->line[pgl->curpos++] = ch;
                    ++pgl->curlen;
                    pgl->line[pgl->curpos] = '\0';
                    putchar(ch);
                    break;
                }
                // else we are inserting a character
                memmove(&pgl->line[pgl->curpos+1], &pgl->line[pgl->curpos], nAfter+1 );
                pgl->line[pgl->curpos] = ch;
                puts( &pgl->line[pgl->curpos++] );
                ++pgl->curlen;
                repeatChar( '\b', nAfter );
                break;
          } // switch
        PRINTSEM_GIVE
        }        
    } // while stream has characters
    return false;
}


void printConsoleHistory( getline_t * pgl )
{
    for( uint16_t ln=pgl->firstLineBufIdx; ln<pgl->endLineBufIdx; ++ln ) {
        uint16_t lineBegIdx = pgl->lineBegIdxBuf[ ln % pgl->lineBegIdxBufSize ];
        if( lineBegIdx != 0xFFFF )
        printf( "%4d %s\n", ln, &pgl->historyBuf[ lineBegIdx ] );
    }
}


void restoreGetline(void)
{
    getline_t * const pgl = globals.pgl;
    puts( pgl->prompt );
    if( pgl->curlen> 0 ) {
        puts( pgl->line );
        repeatChar( '\b', pgl->curlen - pgl->curpos );
    }
}

void undoGetline(void)
{
    getline_t * const pgl = globals.pgl;
    uint8_t pl = strlen(pgl->prompt);
    repeatChar( '\b', pgl->curpos+pl );
    repeatChar( ' ', pgl->curlen+pl );
    repeatChar( '\b', pgl->curlen+pl );
}
