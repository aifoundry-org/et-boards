/**
 * \file
 *
 * \brief gcc starttup file for SAMD20
 *
 * Copyright (c) 2018 Microchip Technology Inc.
 *
 * \asf_license_start
 *
 * \page License
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the Licence at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * \asf_license_stop
 *
 */

#include "samd20.h"

/* Define for null pointer */
#define COM_NULL_PTR (void *)(0UL)

/* Define for dummy handler weak define */
#define COM_DUMMY_HANDLER __attribute__((weak, alias("Dummy_Handler")))

/* Initialize segments */
extern uint32_t _sfixed;
extern uint32_t _efixed;
extern uint32_t _etext;
extern uint32_t _srelocate;
extern uint32_t _erelocate;
extern uint32_t _szero;
extern uint32_t _ezero;
extern uint32_t _sstack;
extern uint32_t _estack;

/** \cond DOXYGEN_SHOULD_SKIP_THIS */
int main(void);
/** \endcond */

void __libc_init_array(void);

/* Default empty handler */
void Dummy_Handler(void);

/* Cortex-M0+ core handlers */
void NonMaskableInt_Handler(void) COM_DUMMY_HANDLER;
void HardFault_Handler(void) COM_DUMMY_HANDLER;
void SVCall_Handler(void) COM_DUMMY_HANDLER;
void PendSV_Handler(void) COM_DUMMY_HANDLER;
void SysTick_Handler(void) COM_DUMMY_HANDLER;

/* Peripherals handlers */
void PM_Handler(void) COM_DUMMY_HANDLER;
void SYSCTRL_Handler(void) COM_DUMMY_HANDLER;
void WDT_Handler(void) COM_DUMMY_HANDLER;
void RTC_Handler(void) COM_DUMMY_HANDLER;
void EIC_Handler(void) COM_DUMMY_HANDLER;
void NVMCTRL_Handler(void) COM_DUMMY_HANDLER;
void EVSYS_Handler(void) COM_DUMMY_HANDLER;
void SERCOM0_Handler(void) COM_DUMMY_HANDLER;
void SERCOM1_Handler(void) COM_DUMMY_HANDLER;
void SERCOM2_Handler(void) COM_DUMMY_HANDLER;
void SERCOM3_Handler(void) COM_DUMMY_HANDLER;
#ifdef ID_SERCOM4
void SERCOM4_Handler(void) COM_DUMMY_HANDLER;
#define SERCOM4_HANDLER (void *)SERCOM4_Handler
#else
#define SERCOM4_HANDLER COM_NULL_PTR
#endif
#ifdef ID_SERCOM5
void SERCOM5_Handler(void) COM_DUMMY_HANDLER;
#define SERCOM5_HANDLER (void *)SERCOM5_Handler
#else
#define SERCOM5_HANDLER COM_NULL_PTR
#endif
void TC0_Handler(void) COM_DUMMY_HANDLER;
void TC1_Handler(void) COM_DUMMY_HANDLER;
void TC2_Handler(void) COM_DUMMY_HANDLER;
void TC3_Handler(void) COM_DUMMY_HANDLER;
void TC4_Handler(void) COM_DUMMY_HANDLER;
void TC5_Handler(void) COM_DUMMY_HANDLER;
#ifdef ID_TC6
void TC6_Handler(void) COM_DUMMY_HANDLER;
#define TC6_HANDLER (void *)TC6_Handler
#else
#define TC6_HANDLER COM_NULL_PTR
#endif
#ifdef ID_TC7
void TC7_Handler(void) COM_DUMMY_HANDLER;
#define TC7_HANDLER (void *)TC7_Handler
#else
#define TC7_HANDLER COM_NULL_PTR
#endif
#ifdef ID_ADC
void ADC_Handler(void) COM_DUMMY_HANDLER;
#define ADC_HANDLER (void *)ADC_Handler
#else
#define ADC_HANDLER COM_NULL_PTR
#endif
#ifdef ID_AC
void AC_Handler(void) COM_DUMMY_HANDLER;
#define AC_HANDLER (void *)AC_Handler
#else
#define AC_HANDLER COM_NULL_PTR
#endif
#ifdef ID_DAC
void DAC_Handler(void) COM_DUMMY_HANDLER;
#define DAC_HANDLER (void *)DAC_Handler
#else
#define DAC_HANDLER COM_NULL_PTR
#endif
#ifdef ID_PTC
void PTC_Handler(void) COM_DUMMY_HANDLER;
#define PTC_HANDLER (void *)PTC_Handler
#else
#define PTC_HANDLER COM_NULL_PTR
#endif

// clang-format off
/* An initialization macro for exception table, this is a common place to initialize
   exception table which is then used in initializing exception vectors and a copy of ivt placed in flash image*/
#define EXCEPTION_VECTOR_TABLE \
 {  /* start of loaded code, location START_OFFSET+0x40, size 0xE0 */ \
    /* Configure Initial Stack Pointer, using linker-generated symbols */ \
    .pvStack = (void *)(&_estack), \
    .pfnReset_Handler = (void *)Reset_Handler, \
    .pfnNonMaskableInt_Handler = (void *)NonMaskableInt_Handler, \
    .pfnHardFault_Handler = (void *)HardFault_Handler, \
    .pvReservedM12 = COM_NULL_PTR, /* Reserved */ \
    .pvReservedM11 = COM_NULL_PTR, /* Reserved */ \
    .pvReservedM10 = COM_NULL_PTR, /* Reserved */ \
    .pvReservedM9 = COM_NULL_PTR,  /* Reserved */ \
    .pvReservedM8 = COM_NULL_PTR,  /* Reserved */ \
    .pvReservedM7 = COM_NULL_PTR,  /* Reserved */ \
    .pvReservedM6 = COM_NULL_PTR,  /* Reserved */ \
    .pfnSVCall_Handler = (void *)SVCall_Handler,    \
    .pvReservedM4 = COM_NULL_PTR, /* Reserved */ \
    .pvReservedM3 = COM_NULL_PTR, /* Reserved */ \
    .pfnPendSV_Handler = (void *)PendSV_Handler, \
    .pfnSysTick_Handler = (void *)SysTick_Handler, \
    /* Configurable interrupts */ \
    .pfnPM_Handler = (void *)PM_Handler,           /*  0 Power Manager */ \
    .pfnSYSCTRL_Handler = (void *)SYSCTRL_Handler, /*  1 System Control */ \
    .pfnWDT_Handler = (void *)WDT_Handler,         /*  2 Watchdog Timer */ \
    .pfnRTC_Handler = (void *)RTC_Handler,         /*  3 Real-Time Counter */ \
    .pfnEIC_Handler = (void *)EIC_Handler,         /*  4 External Interrupt Controller */ \
    .pfnNVMCTRL_Handler = (void *)NVMCTRL_Handler, /*  5 Non-Volatile Memory Controller */ \
    .pfnEVSYS_Handler = (void *)EVSYS_Handler,     /*  6 Event System Interface */ \
    .pfnSERCOM0_Handler = (void *)SERCOM0_Handler, /*  7 Serial Communication Interface 0 */ \
    .pfnSERCOM1_Handler = (void *)SERCOM1_Handler, /*  8 Serial Communication Interface 1 */ \
    .pfnSERCOM2_Handler = (void *)SERCOM2_Handler, /*  9 Serial Communication Interface 2 */ \
    .pfnSERCOM3_Handler = (void *)SERCOM3_Handler, /* 10 Serial Communication Interface 3 */ \
    .pfnSERCOM4_Handler = SERCOM4_HANDLER,   /* 11 Serial Communication Interface 4 */ \
    .pfnSERCOM5_Handler = SERCOM5_HANDLER,   /* 12 Serial Communication Interface 5 */ \
    .pfnTC0_Handler = TC0_Handler, /* 13 Basic Timer Counter 0 */ \
    .pfnTC1_Handler = TC1_Handler, /* 14 Basic Timer Counter 1 */ \
    .pfnTC2_Handler = TC2_Handler, /* 15 Basic Timer Counter 2 */ \
    .pfnTC3_Handler = TC3_Handler, /* 16 Basic Timer Counter 3 */ \
    .pfnTC4_Handler = TC4_Handler, /* 17 Basic Timer Counter 4 */ \
    .pfnTC5_Handler = TC5_Handler, /* 18 Basic Timer Counter 5 */ \
    .pfnTC6_Handler = TC6_HANDLER,  /* 19 Basic Timer Counter 6 */ \
    .pfnTC7_Handler = TC7_HANDLER,  /* 20 Basic Timer Counter 7 */ \
    .pfnADC_Handler = ADC_HANDLER,  /* 21 Analog Digital Converter */ \
    .pfnAC_Handler = AC_HANDLER,   /* 22 Analog Comparators */ \
    .pfnDAC_Handler = DAC_HANDLER,  /* 23 Digital Analog Converter */ \
    .pfnPTC_Handler = PTC_HANDLER  /* 24 Peripheral Touch Controller */ \
}
// clang-format on

#include "board_defs.h"
#include "config.h"

/***************************************************************************/
/*                      - Map of bottom of Flash -                         */
/*               (Base Address: 0x2000 or 0x0 for standalone image)        */
/*                                                                         */
/*     - user                   - base-offset   - size     - section       */
/*     Slot reserved             0x0             0x0 OR                    */
/*                                               0x1E800   .emptyslot      */
/*     IVT                       0x0 OR                                    */
/*                               0x1E800         0x100     .vectors        */
/*     App metadata              0x100 OR                                  */
/*                               0x1E900         0x100     .appmetadata    */
/*     App FW Image              0x200 OR                                  */
/*                               0x1EA00         0x1E600                   */
/***************************************************************************/

/* Exception Table placed in vectors section */
__attribute__((section(".vectors"))) const DeviceVectors exception_table = EXCEPTION_VECTOR_TABLE;

/* padding for Exception table space in vectors section*/
__attribute__((section(".vectors.2")))
const uint32_t exception_table_padding[(IVT_SIZE - sizeof(DeviceVectors)) / sizeof(uint32_t)];

__attribute__((section(".ramvectors"))) DeviceVectors exception_table_ram_copy;

/**
 * \brief This is the code that gets called on processor reset.
 * To initialize the device, and call the main() routine.
 */
#include <string.h>
#include "common_defs.h"

void system_init1(void); // in common_defs.h

void Reset_Handler(void)
{
    uint32_t *pSrc, *pDest;

#ifdef INITDBGHOLD
    asm volatile("       movs    r0, #0  \n\t"
                 "x:     beq x           \n\t" /* clear APSR Z bit from debugger to continue */
    );
#endif

    system_init1();

    /* Skip copying to exception_table_ram_copy (at start of RAM section(".ramvectors"),
       exception table is already present at vectors section which is copied manually below */
    pSrc = ((uint32_t *)&_etext) + sizeof(exception_table) / sizeof(uint32_t);
    pDest = ((uint32_t *)&_srelocate) + sizeof(exception_table) / sizeof(uint32_t);

    /* Data section */
    /* Initialize the relocate segment */
    if (pSrc != pDest)
    {
        while (pDest < &_erelocate)
        {
            *pDest++ = *pSrc++;
        }
    }

    /* BSS section */
    /* Clear the zero segment */
    for (pDest = &_szero; pDest < &_ezero;)
    {
        *pDest++ = 0;
    }

    /* Copy the exeption table to RAM from flash image */
    memcpy((void *)&exception_table_ram_copy, (const uint32_t *)(const void *)&exception_table,
        sizeof(exception_table_ram_copy));
    SCB->VTOR = (uint32_t)&exception_table_ram_copy;

    /* Overwriting the default value of the NVMCTRL.CTRLB.MANW bit (errata reference 13134) */
    NVMCTRL->CTRLB.bit.MANW = 1;

    /* Initialize the C library */
    __libc_init_array();

    // set main stack area to known values for detecting usage
    memset(&_sstack, 0xA5, (uint32_t)__get_MSP() - 0x24 - (uint32_t)&_sstack);

    /* Branch to main function */
    main();

    /* Infinite loop */
    while (1)
        ;
}

/**
 * \brief Default interrupt handler for unused IRQs.
 */
void Dummy_Handler(void)
{
    while (1)
        ;
}
