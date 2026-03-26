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

#include "samd20j18.h"
#include "config.h"

/* Define for null pointer */
#define NULL_PTR (void *)(0UL)

/* Define for dummy handler weak define */
#define DUMMY_HANDLER __attribute__((weak, alias("Dummy_Handler")))

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
void NonMaskableInt_Handler(void) DUMMY_HANDLER;
void HardFault_Handler(void) DUMMY_HANDLER;
void SVCall_Handler(void) DUMMY_HANDLER;
void PendSV_Handler(void) DUMMY_HANDLER;
void SysTick_Handler(void) DUMMY_HANDLER;

/* Peripherals handlers */
void PM_Handler(void) DUMMY_HANDLER;
void SYSCTRL_Handler(void) DUMMY_HANDLER;
void WDT_Handler(void) DUMMY_HANDLER;
void RTC_Handler(void) DUMMY_HANDLER;
void EIC_Handler(void) DUMMY_HANDLER;
void NVMCTRL_Handler(void) DUMMY_HANDLER;
void EVSYS_Handler(void) DUMMY_HANDLER;
void SERCOM0_Handler(void) DUMMY_HANDLER;
void SERCOM1_Handler(void) DUMMY_HANDLER;
void SERCOM2_Handler(void) DUMMY_HANDLER;
void SERCOM3_Handler(void) DUMMY_HANDLER;
#ifdef ID_SERCOM4
void SERCOM4_Handler(void) DUMMY_HANDLER;
#endif
#ifdef ID_SERCOM5
void SERCOM5_Handler(void) DUMMY_HANDLER;
#endif
void TC0_Handler(void) DUMMY_HANDLER;
void TC1_Handler(void) DUMMY_HANDLER;
void TC2_Handler(void) DUMMY_HANDLER;
void TC3_Handler(void) DUMMY_HANDLER;
void TC4_Handler(void) DUMMY_HANDLER;
void TC5_Handler(void) DUMMY_HANDLER;
#ifdef ID_TC6
void TC6_Handler(void) DUMMY_HANDLER;
#endif
#ifdef ID_TC7
void TC7_Handler(void) DUMMY_HANDLER;
#endif
#ifdef ID_ADC
void ADC_Handler(void) DUMMY_HANDLER;
#endif
#ifdef ID_AC
void AC_Handler(void) DUMMY_HANDLER;
#endif
#ifdef ID_DAC
void DAC_Handler(void) DUMMY_HANDLER;
#endif
#ifdef ID_PTC
void PTC_Handler(void) DUMMY_HANDLER;
#endif

/****************************************************************************/
/*                      - Map of bottom of Flash -                          */
/*                         (Base Address: 0x0)                              */
/*     - user                   - base-offset   - size       - section      */
/*     Real IVT                  0x0             0x100        .vectors      */
/*     Bootloader metadata       0x100           0x100        .blmetadata   */
/*     Botloader FW image        0x200           0x1E00                     */
/****************************************************************************/

/* Exception Table */
__attribute__((section(".vectors"))) const DeviceVectors exception_table = {

    /* Configure Initial Stack Pointer, using linker-generated symbols */
    .pvStack = (void *)(&_estack),

    .pfnReset_Handler = (void *)Reset_Handler,                   /* Reset handler */
    .pfnNonMaskableInt_Handler = (void *)NonMaskableInt_Handler, /* NMI handler */
    .pfnHardFault_Handler = (void *)HardFault_Handler,
    .pvReservedM12 = NULL_PTR,                     /* Reserved slot*/
    .pvReservedM11 = NULL_PTR,                     /* Reserved slot*/
    .pvReservedM10 = NULL_PTR,                     /* Reserved slot */
    .pvReservedM9 = NULL_PTR,                      /* Reserved slot */
    .pvReservedM8 = NULL_PTR,                      /* Reserved slot */
    .pvReservedM7 = NULL_PTR,                      /* Reserved slot */
    .pvReservedM6 = NULL_PTR,                      /* Reserved slot */
    .pfnSVCall_Handler = (void *)SVCall_Handler,   /* Service call handler */
    .pvReservedM4 = NULL_PTR,                      /* Reserved slot */
    .pvReservedM3 = NULL_PTR,                      /* Reserved slot */
    .pfnPendSV_Handler = (void *)PendSV_Handler,   /* Pending service call handler */
    .pfnSysTick_Handler = (void *)SysTick_Handler, /* System tick handler */

    /* Configurable interrupts */
    .pfnPM_Handler = (void *)PM_Handler,           /*  0-Power Manager */
    .pfnSYSCTRL_Handler = (void *)SYSCTRL_Handler, /*  1-System Control */
    .pfnWDT_Handler = (void *)WDT_Handler,         /*  2-Watchdog Timer */
    .pfnRTC_Handler = (void *)RTC_Handler,         /*  3-Real-Time Counter */
    .pfnEIC_Handler = (void *)EIC_Handler,         /*  4-External Interrupt Controller */
    .pfnNVMCTRL_Handler = (void *)NVMCTRL_Handler, /*  5-Non-Volatile Memory Controller */
    .pfnEVSYS_Handler = (void *)EVSYS_Handler,     /*  6-Event System Interface */
    .pfnSERCOM0_Handler = (void *)SERCOM0_Handler, /*  7-Serial Communication Interface 0 */
    .pfnSERCOM1_Handler = (void *)SERCOM1_Handler, /*  8-Serial Communication Interface 1 */
    .pfnSERCOM2_Handler = (void *)SERCOM2_Handler, /*  9-Serial Communication Interface 2 */
    .pfnSERCOM3_Handler = (void *)SERCOM3_Handler, /* 10-Serial Communication Interface 3 */
#ifdef ID_SERCOM4
    .pfnSERCOM4_Handler = (void *)SERCOM4_Handler, /* 11-Serial Communication Interface 4 */
#else
    .pvReserved11 = NULL_PTR, /*-11 Reserved slot */
#endif
#ifdef ID_SERCOM5
    .pfnSERCOM5_Handler = (void *)SERCOM5_Handler, /*12-Serial Communication Interface 5 */
#else
    .pvReserved12 = NULL_PTR, /* 12-Reserved slot */
#endif
    .pfnTC0_Handler = (void *)TC0_Handler, /* 13-Basic Timer Counter 0 */
    .pfnTC1_Handler = (void *)TC1_Handler, /* 14-Basic Timer Counter 1 */
    .pfnTC2_Handler = (void *)TC2_Handler, /* 15-Basic Timer Counter 2 */
    .pfnTC3_Handler = (void *)TC3_Handler, /* 16-Basic Timer Counter 3 */
    .pfnTC4_Handler = (void *)TC4_Handler, /* 17-Basic Timer Counter 4 */
    .pfnTC5_Handler = (void *)TC5_Handler, /* 18-Basic Timer Counter 5 */
#ifdef ID_TC6
    .pfnTC6_Handler = (void *)TC6_Handler, /* 19-Basic Timer Counter 6 */
#else
    .pvReserved19 = NULL_PTR, /* 19-Reserved slot */
#endif
#ifdef ID_TC7
    .pfnTC7_Handler = (void *)TC7_Handler, /* 20-Basic Timer Counter 7 */
#else
    .pvReserved20 = NULL_PTR, /* 20-Reserved slot */
#endif
#ifdef ID_ADC
    .pfnADC_Handler = (void *)ADC_Handler, /* 21-Analog Digital Converter */
#else
    .pvReserved21 = NULL_PTR, /* 21-Reserved slot */
#endif
#ifdef ID_AC
    .pfnAC_Handler = (void *)AC_Handler, /* 22-Analog Comparators */
#else
    .pvReserved22 = NULL_PTR, /* 22-Reserved slot */
#endif
#ifdef ID_DAC
    .pfnDAC_Handler = (void *)DAC_Handler, /* 23-Digital Analog Converter */
#else
    .pvReserved23 = NULL_PTR, /* 23-Reserved slot */
#endif
#ifdef ID_PTC
    .pfnPTC_Handler = (void *)PTC_Handler /* 24-Peripheral Touch Controller */
#else
    .pvReserved24 = NULL_PTR  /* 24-Reserved slot */
#endif
};

#include "board_defs.h"

__attribute__((section(".vectors.2")))
const uint32_t exception_table_padding[(IVT_SIZE - sizeof(DeviceVectors)) / sizeof(uint32_t)];

/**
 * \brief This is the code that gets called on processor reset.
 * To initialize the device, and call the main() routine.
 */

void system_init1(void); // in board_defs.h

extern uint32_t startupFlag;

void Reset_Handler(uint32_t r0)
{
    asm volatile("    push {r0} ");
//#define INITDBGHOLD
#ifdef INITDBGHOLD
    asm volatile("       movs    r1, #0  \n\t"
                 "x:     beq x   \n\t" /* clear APSR Z bit from debugger to continue */
    );
#endif

    uint32_t *pSrc, *pDest;

    system_init1();

    /* Initialize the relocate segment */
    pSrc = &_etext;
    pDest = &_srelocate;

    if (pSrc != pDest)
    {
        for (; pDest < &_erelocate;)
        {
            *pDest++ = *pSrc++;
        }
    }

    /* Clear the zero segment */
    for (pDest = &_szero; pDest < &_ezero;)
    {
        *pDest++ = 0;
    }

    /* Set the vector table base address */
    SCB->VTOR = ((uint32_t)&_sfixed & SCB_VTOR_TBLOFF_Msk);

    /* Overwriting the default value of the NVMCTRL.CTRLB.MANW bit (errata reference 13134) */
    NVMCTRL->CTRLB.bit.MANW = 1;

    /* Initialize the C library */
    __libc_init_array();

    uint32_t *p = &startupFlag;
    asm volatile("    pop {r0}       \t\n"
                 "    str r0, [%0]"
                 :
                 : "r"(p));

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
    {
    }
}
