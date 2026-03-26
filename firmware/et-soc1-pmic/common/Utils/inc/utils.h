/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file utils.h
    \brief Utils.
*/

#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdbool.h>
#include "error_codes.h"

#undef printf
#undef putchar
#undef sprintf
#undef snprintf

 __attribute__ ((format (printf, 1, 2)))
int printf_(const char *format, ...);

 __attribute__ ((format (printf, 1, 2)))
void printfProt(const char *format, ...);

 __attribute__ ((format (printf, 1, 2)))
void printfProt(const char *format, ...);

void restartPrintfFromInterrupt(void);
void printfFromInterrupt(const char * format, uint32_t v0, uint32_t v1 );
void printfFromInterruptNoFlush(const char * format, uint32_t v0, uint32_t v1 );
void FlushPrintfFromInterrupt(void);

#define TRACE_FI( msg ) {printfFromInterruptNoFlush( "TRC "msg": %s line %d", (uint32_t)__FUNCTION__, __LINE__);}

void clearPrintfFromInterrupt(void);
void checkPrintfFromInterrupt(void);

#define printf printf_
#define puts puts_
#define putchar putchar_

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define ARRAY_SIZE(x) (sizeof((x)) / sizeof((x)[0]))

#define CPU_FREQUENCY 48000000
#define MHZ (CPU_FREQUENCY/1000000)

#define INT64_SHR32(x) ( (x) >>32 )
#define INT64_SHL32(x) ( ((int64_t)x) <<32 )


#define MA_PER_A (1000UL)
#define MV_PER_V (1000UL)
#define MV_PER_50MV (50UL)
#define MV_PER_CENTIVOLT (10UL)
#define MW_PER_W (1000UL)

extern bool pfiShowTime;

int putchar(int ch );
int putcharRaw(int ch );

#define countof(x) ( sizeof(x)/sizeof(*x) )
#define endof(x) ( &x[sizeof(x)/sizeof(*x)] )

void assert1( bool cond, char const * _msg, uint32_t _v1, uint32_t _v2 );

void _assert( char const * file, int line, char const * msg);
#define assertMsg(v,msg) ( (v) ? (void)0 : _assert(__FILE__, __LINE__, msg ) )

#define MS_TO_CYCLE(x) ( x * (CPU_FREQUENCY/1000) )
#define US_TO_CYCLE(x) ( x * (CPU_FREQUENCY/1000000) )
#define NS_TO_CYCLE(x) ( (x * (CPU_FREQUENCY/1000000) )/1000 )

#define OSDELAY_MS(n) vTaskDelay(pdMS_TO_TICKS(n));
#define OSHWDELAY_MS(n) hwOsDelayUS( (uint32_t)(n*1000) );

uint64_t getTimer2(void);
uint64_t getTimerUS(void);
void hwDelay( uint32_t cycles );
void hwDelayUS( uint32_t uS );
void hwOsDelayUS( uint32_t uS );
void recordTime( char const * name );

void repeatChar( char c, uint8_t n);
int  printf (const char *__restrict, ...) _ATTRIBUTE ((__format__ (__printf__, 1, 2)));
int  printArgs( char **out, void (*afterNlHook)(char), const char *format, va_list args );
int  puts (const char *);

void dummy(void);
void dummy1(uint32_t n);

#define disable_irq()   __disable_irq()
#define enable_irq()    __enable_irq()
void initStack(void);
void cmdStack(void);

void reboot(void);

#endif /* ASSERTMSG_H */
