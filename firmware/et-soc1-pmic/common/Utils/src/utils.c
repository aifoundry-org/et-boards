/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file utils.c
    \brief C file for utils
*/

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <compiler.h>
#include <string.h>
#include "globals.h"
#include "CLITask.h"
#include "IoTask.h"

#include "utils.h"

allStacksInfo_t allStacksInfo;
globals_t globals;

void repeatChar( char c, uint8_t n)
{
    uint8_t i;
    for( i=0; i<n; ++i )
        putchar(c);
}

int puts( char const * s )
{
    while(*s)
        putchar(*s++);
   return STATUS_SUCCESS;
}

static uint64_t volatile tc2OfloCount;
static uint16_t volatile oldTc2count;

void TC2_Handler(void) // interrupt handler
{
    TC2->COUNT16.INTFLAG.reg = TC_INTFLAG_MASK;     // Clear overflow interrupt
}

static void checkTc2Count(void);
void checkTc2Count(void)
{
    bool inverted;
    static bool oldInverted = 0;

    taskENTER_CRITICAL();
    uint16_t count = TC2->COUNT16.COUNT.reg;
    inverted = (oldTc2count & ~count) >>15;
    if( inverted && !oldInverted) ++tc2OfloCount;     // really wrapped
    oldInverted = inverted;
    oldTc2count = count;
    taskEXIT_CRITICAL();
}

uint64_t getTimer2(void)
{
    checkTc2Count();
    return (tc2OfloCount<<16) | oldTc2count;
}

uint64_t getTimerUS(void)
{
    return getTimer2()/MHZ;
}

void TC3_Handler(void) // interrupt handler 100 kHz
{
    checkTc2Count();
    TC3->COUNT16.INTFLAG.reg = TC_INTFLAG_MASK;     // Clear overflow interrupt
}

void hwDelayUS( uint32_t uS )
{
    int64_t volatile t = getTimer2();
    while( (int64_t)(getTimer2() -t) < (MHZ * (int64_t)uS) )
        continue;
}

void hwDelay( uint32_t cycles )
{
    int64_t volatile t = getTimer2();
    while( (int64_t)(getTimer2() -t) < cycles )
        continue;
}

static uint32_t volatile t4Remaining;

void hwOsDelayUS( uint32_t uS )
{
    uint32_t cycles = uS;
    if( cycles==0 ) return;
    T4PROTECTSEM_TAKE
    uint16_t count = cycles<=(1<<16) ? cycles-1 : 0xFFFF;
    t4Remaining = cycles - (count+1);
    TC4->COUNT16.CC[0].reg = count;
    while(TC4->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY) {};
    TC4->COUNT16.INTFLAG.reg = TC_INTFLAG_MASK;
    TC4->COUNT16.INTENSET.reg = TC_INTENSET_OVF;
    while(TC4->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY) {};
    TC4->COUNT16.CTRLBSET.reg = TC_CTRLBSET_CMD_RETRIGGER | TC_CTRLBSET_ONESHOT; // start timer
    xSemaphoreTake( globals.t4ReleaseSemaphore, portMAX_DELAY  );
    T4PROTECTSEM_GIVE
}

void TC4_Handler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    TC4->COUNT16.INTFLAG.reg = TC_INTFLAG_MASK;
    if( t4Remaining>0 ) {
       if( t4Remaining<=(1<<16) ) { // last time
            TC4->COUNT16.CC[0].reg = t4Remaining-1;
            t4Remaining = 0;
       } else { // not last time
            TC4->COUNT16.CC[0].reg = 0xFFFF;
            t4Remaining -= 0x10000;
       }
        while(TC4->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY) {};
        TC4->COUNT16.CTRLBSET.reg = TC_CTRLBSET_CMD_RETRIGGER | TC_CTRLBSET_ONESHOT; // start timer
    } else {
        TC4->COUNT16.CTRLBSET.reg = TC_CTRLBSET_CMD_STOP | TC_CTRLBSET_ONESHOT;
        xSemaphoreGiveFromISR( globals.t4ReleaseSemaphore, &xHigherPriorityTaskWoken );
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void recordTime( char const * name ) // may be called in interrupt context
{
   printfFromInterruptNoFlush( "%-25s", (uint32_t)name, 0 );
}

#define MHZ (CPU_FREQUENCY/1000000)
static void printTime( uint64_t cycles );
void printTime( uint64_t cycles )
{
    uint64_t volatile uS = cycles/MHZ;
    uint32_t volatile hNs = ((cycles - uS*MHZ)*100)/MHZ; // hundredths of microseconds
    uint64_t mS = uS/1000;
    uS -= 1000*mS;          // microseconds
    uint64_t s = mS/1000;   // seconds
    mS -= s*1000;            // milliseconds
    interruptingPrintf( "%lu.%03d,%03d,%02d", (uint32_t)s, (int)mS, (int)uS, (int)hNs );
}


typedef struct
{
    uint64_t time;
    uint32_t count;
    char const * format;
    union {
        uint32_t v;
        void * p;
    } arg[2];
} intrPrintf_t;


typedef struct {
    uint16_t inIdx;
    uint16_t outIdx;
    intrPrintf_t buf[256];
} intrPrintfInfo_t;
static volatile intrPrintfInfo_t intrPrintfInfo = {0,0};

void restartPrintfFromInterrupt(void)
{
    uint16_t prevOut = intrPrintfInfo.outIdx;
    do {
        intrPrintfInfo.outIdx = prevOut;
        prevOut = intrPrintfInfo.outIdx -1;
        if( (int16_t)prevOut <0 ) {
            prevOut += countof(intrPrintfInfo.buf);
        }            
    } while( intrPrintfInfo.buf[prevOut].format  &&  prevOut != intrPrintfInfo.inIdx );
    enqueueCommandFasttask(  FT_intrPrint, NULL );
}


void printfFromInterrupt(const char * format, uint32_t v0, uint32_t v1 )
{
    printfFromInterruptNoFlush( format, v0, v1 );
    FlushPrintfFromInterrupt();
}


void FlushPrintfFromInterrupt(void)
{
    enqueueCommandFasttask(  FT_intrPrint, NULL );
}

void printfFromInterruptNoFlush(const char * format, uint32_t v0, uint32_t v1 )
{
    static uint32_t count = 1;
    taskENTER_CRITICAL();
    do {
        int32_t t = (int32_t)(intrPrintfInfo.inIdx -intrPrintfInfo.outIdx);
        if( t == countof(intrPrintfInfo.buf)  || t == -1 ) break;
    
        intrPrintf_t volatile * p = &intrPrintfInfo.buf[intrPrintfInfo.inIdx];
        p->time = getTimer2();
        p->count = count++;
        p->format = format;
        p->arg[0].v = v0;
        p->arg[1].v = v1;
        if( ++intrPrintfInfo.inIdx == countof(intrPrintfInfo.buf) ) {
            intrPrintfInfo.inIdx = 0;
        }

    } while(0);    
    taskEXIT_CRITICAL();
}

void clearPrintfFromInterrupt(void)
{
    intrPrintfInfo.outIdx = intrPrintfInfo.inIdx = 0;
}

bool pfiShowTime = 1;
void checkPrintfFromInterrupt(void)
{
    static uint64_t prev = 0;

    if( intrPrintfInfo.outIdx != intrPrintfInfo.inIdx ) {
        OSDELAY_MS(3)
        interruptingPrintBegin();
        while(  intrPrintfInfo.outIdx != intrPrintfInfo.inIdx ) {
            intrPrintf_t volatile * p = &intrPrintfInfo.buf[intrPrintfInfo.outIdx];
            interruptingPrintf( "(%03d) ", p->count );
            interruptingPrintf( p->format, p->arg[0], p->arg[1] );
            if(pfiShowTime) {
                interruptingPrintf( "         " );
                printTime( p->time );
                interruptingPrintf( " sec. (" );
                printTime( p->time -prev );
                interruptingPrintf( ")" );
                prev = p->time;
            }
            interruptingPrintNL();
            if( ++intrPrintfInfo.outIdx == countof(intrPrintfInfo.buf) ) {
                intrPrintfInfo.outIdx = 0;
            }
        }
        interruptingPrintEnd();
    }
}


void assert1( bool cond, char const * _msg, uint32_t _v1, uint32_t _v2 )
{
    if( cond ) return;

    char const * volatile msg = _msg; // for debugger
    uint32_t volatile v1 = _v1;;
    uint32_t volatile v2 = _v2;
    (void)msg;
    (void)v1;
    (void)v2;
    for(;;);
}


static int volatile xxxx;
void dummy(void) { ++xxxx; }
void dummy1(uint32_t n) { (void)n; dummy(); }



void reboot(void)
{
    __disable_irq();
    NVIC_SystemReset();
}
