/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file system_clock.h
    \brief Replacement for sketchy clock code.
    This replaces clock.c/clock.h
*/
/***********************************************************************/

#ifndef __SYSTEM_CLOCK_H__
#define __SYSTEM_CLOCK_H__


#ifdef __cplusplus
extern "C" {
#endif

#include <compiler.h>
#include "board_defs.h"

#define DFLL_MAX_COARSE_STEP (0x1F/4)
#define DFLL_MAX_FINE_STEP (0xFF/4)

//??? need to check this value for DFLL_FINE_VALUE
#define DFLL_FINE_VALUE 512

static inline void gclk_write_CLKCTRL(uint16_t data)
{
    GCLK->CLKCTRL.reg = data;
}

static inline void gclk_write_GENCTRL(uint32_t data)
{
    GCLK->GENCTRL.reg = data;
}

static inline void gclk_write_GENDIV(uint32_t data)
{
    GCLK->GENDIV.reg = data;
}

static inline void gclk_wait_for_sync(void)
{
    while (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY) {};
}

static inline void system_clock_osc32k_setup(void)
{
    uint32_t calibration = EXTRACT_NVM_CALIB(NVM_OSC32K_CAL_Pos,NVM_OSC32K_CAL_Len);

    SYSCTRL->OSC32K.reg  = SYSCTRL_OSC32K_EN32K
                         | SYSCTRL_OSC32K_STARTUP_10
                         | SYSCTRL_OSC32K_RUNSTDBY
                         | SYSCTRL_OSC32K_CALIB(calibration);
    SYSCTRL->OSC32K.reg |= SYSCTRL_OSC32K_ENABLE;
}

static inline void system_clock_osc8m_setup(void)
{
    SYSCTRL_OSC8M_Type temp = SYSCTRL->OSC8M;

    /* Use temporary struct to reduce register access */
    temp.bit.PRESC    = 0;
    temp.bit.ONDEMAND = 0;
    temp.bit.RUNSTDBY = 1;

    SYSCTRL->OSC8M = temp;
}

/*
 * system_clock_dfll_setup()
 * this is a maintainable rewrite of the DFLL clock init from Atmel.
 * setting these up is rather simple made complicated by Atmel
 * furthermore the DFLL has one mode of operation that works well
 * and doesn't work well outside of this configuration
 * the only real configuration that would be different is if you wish
 * to use this for battery operation.
 */
static inline void system_clock_dfll_setup(void)
{
    uint32_t coarse;
    SYSCTRL->DFLLCTRL.reg = SYSCTRL_DFLLCTRL_ENABLE;

    while(!(SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_DFLLRDY)) {};
    SYSCTRL->DFLLMUL.reg = SYSCTRL_DFLLMUL_CSTEP(DFLL_MAX_COARSE_STEP) |
                  SYSCTRL_DFLLMUL_FSTEP(DFLL_MAX_FINE_STEP)   |
                  SYSCTRL_DFLLMUL_MUL(DFLL_MULTIPLY_FACTOR);
    coarse = (*((uint32_t *)(NVMCTRL_OTP4)
            + (NVM_DFLL_COARSE_Pos / 32))
        >> (NVM_DFLL_COARSE_Pos % 32))
        & ((1 << NVM_DFLL_COARSE_Len) - 1);
    if (coarse == 0x3f) coarse = 0x1f;

    SYSCTRL->DFLLVAL.reg = SYSCTRL_DFLLVAL_COARSE(coarse) |
                SYSCTRL_DFLLVAL_FINE(DFLL_FINE_VALUE);

    /* Write full configuration to DFLL control register */
    SYSCTRL->DFLLCTRL.reg = 0;
    while(!(SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_DFLLRDY)) {};
    SYSCTRL->DFLLCTRL.reg = SYSCTRL_DFLLCTRL_ENABLE
                          | SYSCTRL_DFLLCTRL_MODE_CLOSED_LOOP
                          | SYSCTRL_DFLLCTRL_QLDIS_QUICKLOCK_ENABLED
                          | SYSCTRL_DFLLCTRL_STABLE_TRACK_AFTER_LOCK
                          | SYSCTRL_DFLLCTRL_LLAW_KEEP_LOCK_AFTER_WAKE
                          | SYSCTRL_DFLLCTRL_CCDIS_CHILLCYCLE_ENABLED;

    while (!((SYSCTRL->PCLKSR.reg
        & (SYSCTRL_PCLKSR_DFLLRDY | SYSCTRL_PCLKSR_DFLLLCKF | SYSCTRL_PCLKSR_DFLLLCKC))
        == (SYSCTRL_PCLKSR_DFLLRDY | SYSCTRL_PCLKSR_DFLLLCKF | SYSCTRL_PCLKSR_DFLLLCKC))) {};
}
#endif // __SYSTEM_CLOCK_H__
