/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file cli.c
    \brief C file for command line interface
*/
/***********************************************************************/

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include "board_defs.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "globals.h"
#include "CLITask.h"
#include "cli.h"
#include "console.h"
#include "utils.h"

// returns start of the current token
// updates the char pointer str with the end of the current String
// returns NULL if done
char *nextToken(char **str)
{
    char *p = *str;
    char *q;
    if(p == NULL) return NULL;
    while((*p == ' ') || (*p == '\t')) p++; // eat whitespace
    if(*p == '\0') return NULL;
    q = p; // p points to start of token
    while((*q != ' ') && (*q != '\t') && (*q != '\r') && (*q != '\n') && (*q != '\0')) q++; // find end of token
    if (*q == '\0') {
        *str = NULL;
    } else {
        *q++ = '\0'; // null terminate current token
        *str = q; // update our next pointer
    }
    return p;
}

// checks if hex or decimal number, hex is preceded by 0x or 0X
// returns -2 if  no number
// return -1 if invalid number
// return 0 if valid and converts number to signed 32 bit int
// if good converts number to signed 32 bit int
int parseNumber(char *s, int32_t *value)
{
    char *ep;
    int32_t v;

    if ((s == NULL) || (*s == '\0')) return UTILS_CLI_ERROR_INVALID_ARGUMENT;
    errno = 0;
    v = strtol(s, &ep, 0);
    if (s == ep || ((v == LONG_MIN || v == LONG_MAX) && errno == ERANGE)) return UTILS_CLI_ERROR_INVALID_VALUE;
    *value = v;
    return STATUS_SUCCESS;
}

// linear search small array for exact match to string s not case sensitive
// return -1 if not found otherwise returns position starting at zero
// end list with empty string, null entries are skipped
//

int searchList(char *s, const char **list)
{
    int i = 0;
    if (*s == '\0') return -1;
    while ((list[i] == NULL) || ((list[i] != NULL) && (*list[i] != '\0'))) {
        if ((list[i] != NULL) && (strcasecmp(list[i], s) == 0)) return i;
        i++;
    }
    return -1;
}

// searchCommand
int searchCommand(char *cmdstr, const commandList_t *list, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        if (strcasecmp(list[i].name, cmdstr) == 0) {
            return list[i].id;
        }
        if ((list[i].altname != NULL) && (strcasecmp(list[i].altname, cmdstr) == 0)) {
            return list[i].id;
        }
    }
    return UTILS_CLI_ERROR_INVALID_CMD;
}

// printHelp
void printHelp(const commandList_t *list, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        if (list[i].name == NULL)
            continue;
        printf("%-17s", list[i].name);
        if (list[i].altname != NULL) {
            printf("| %-8s", list[i].altname);
        } else {
            printf("%10s", " " );
        }
        printf("-> %s\n", list[i].help);
    }
}

// dump byte memory starting at mem for count
void dump_mem(uint8_t *mem, uint16_t count)
{
    int len = ((count + 15)/16) * 16;
    unsigned char ch;
    int i;
    int j;
    for (i = 0; i < len; i++) {
        if (i < count) {
            printf("%02X ", (unsigned char) mem[i]);
        } else {
            printf("   ");
        }
        if((i % 16) == 15) {
            printf(" ");
            for(j = i-15; j <= i; j++) {
                if (j < count) {
                    ch = mem[j];
                    if ((ch < 32) || (ch > 127)) ch = '.';
                    printf("%c", ch);
                } else {
                    printf(" ");
                }
            }
#ifdef FEATURE_UART_CRLF
            printf("\r");
#endif
            printf("\n");
        }
    }
}



void _assert( char const * file, int line, char const * msg)
{
    xSemaphoreTake( globals.printSemaphore, pdMS_TO_TICKS(200)  );
    if(msg)
        printf( "Assert fail: %s in %s:%d\n", msg, file, line );
    else
        printf( "Assert fail in %s:%d\n", file, line );
    for(;;);

}

void i2cPErrorOL( i2cmErr_t rv, char const * msg )
{
    if( rv != I2CM_OK ) interruptingPrintfOneLine( "%s: I2C error %s\n", msg, i2cm_get_error_message(rv) );
}

void i2cPError( i2cmErr_t rv, char const * msg )
{
    if( rv != I2CM_OK ) interruptingPrintf( "%s: I2C error %s\n", msg, i2cm_get_error_message(rv) );
}

void i2cPError2OL( i2cmErr_t rv, char const * fmt, char const * msg, uint32_t v )
{
    if( rv == I2CM_OK ) return;
    interruptingPrintBegin();
    interruptingPrintf( fmt, msg, v );
    interruptingPrintf( " : I2C error %s\n", i2cm_get_error_message(rv) );
    interruptingPrintEnd();
}

char toupper1( char c )
{
    if( c>=0x60 && c<0x7B )
    c &= 0xDF;
    return c;
}

void toupperstr( char * p )
{
    for( ; p && *p; ++p)
    *p = toupper1(*p);
}

uint32_t atohex( char const * p, bool * pok )
{
    char * endptr;
    uint32_t ret;
    if( !p ) {
        if(pok) *pok = 0;
        return 0;
    }
    ret = strtoul( p, &endptr, 16 );
    if(pok) *pok = *endptr == '\0';
    return ret;
}

uint32_t atodec( char const * p, bool * pok )
{
    char * endptr;
    uint32_t ret;
    if( !p ) {
        if(pok) *pok = 0;
        return 0;
    }
    ret = strtoul( p, &endptr, 10 );
    if(pok) *pok = *endptr == '\0';
    return ret;
}


