/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file system_init.c
    \brief C file for SAMD20 initialization for Bring Up Board (BUB)
*/
/***********************************************************************/

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "stream_buffer.h"
#include "task.h"
#include "semphr.h"
#include <compiler.h>
#include "board_defs.h"
#include "common_defs.h"
#include "gpio.h"
#include "i2cm.h"
#include "i2cm2.h"
#include "i2cs.h"
#include "nvm.h"
#include "adc.h"
#include "system_clock.h"
#include "i2cs.h"
#include "error_codes.h"
#include "hw_encoding.h"
#include "IoTask.h"
#include "etsoc_cmd_handler_task.h"

/***********************************************************************
* Macros:             Defininition of local macros
***********************************************************************/
#define UART_BAUD(rate, freq) (65536 - ((1024 * (rate)) / ((freq) / 1024)))

// I2C Baud rate calculations
#define TRISE 300
#define I2CM_BAUDTOTAL(rate, freq) \
    (((freq) - (rate * 10) - (TRISE * ((rate) / 1000) * ((freq) / 1000)) / 1000) / (rate))
#define I2CM_BAUDLOW(rate, freq) ((2 * I2CM_BAUDTOTAL(rate, freq)) / 3)
#define I2CM_BAUD(rate, freq)    (I2CM_BAUDTOTAL(rate, freq) - I2CM_BAUDLOW(rate, freq))

/***********************************************************************
* GLOBAL Functions:   Defininition of global functions
***********************************************************************/
/*
 * void system_init()
 * call system_init() prior to anything else in main()
 * this configures all peripherals with the exception of NVIC
 * NVIC enables are located in separate init functions usually located
 * with the service routine. These inits must be called from main after system_init()
 */
void system_init1(void)
{
    SYSCTRL->INTFLAG.reg = SYSCTRL_INTFLAG_BOD33RDY | SYSCTRL_INTFLAG_BOD33DET | SYSCTRL_INTFLAG_DFLLRDY;

    // Start 32KHz oscillator for DFLL Reference
    system_clock_osc32k_setup();

    // flash wait states at 3.3V, 1 for 48MHz Clock, 0 for 8MHz or less
    // flash wait states at 1.8V, 3 for 48MHz Clock, 0 for 8MHz or less
#ifdef SAMD20DEVBOARD
    NVMCTRL->CTRLB.bit.RWS = 1;
#else
    NVMCTRL->CTRLB.bit.RWS = 3;
#endif

    /* Initialize GCLK */
    // GCLK 0 48MHz DFLL to CPU
    // GCLK 3 OSC32 (32.768KHz) to DFLL Reference
    gclk_write_GENDIV(GCLK_GENDIV_ID_GCLK3 | GCLK_GENDIV_DIV(1));
    gclk_write_GENCTRL(GCLK_GENCTRL_ID_GCLK3 | GCLK_GENCTRL_GENEN | GCLK_GENCTRL_SRC_OSC32K);
    gclk_wait_for_sync();
    gclk_write_CLKCTRL(GCLK_CLKCTRL_GEN_GCLK3 | GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_ID_DFLL48M);

    system_clock_dfll_setup();
    gclk_write_GENDIV(GCLK_GENDIV_ID_GCLK0 | GCLK_GENDIV_DIV(1));
    gclk_write_GENCTRL(GCLK_GENCTRL_ID_GCLK0 | GCLK_GENCTRL_GENEN | GCLK_GENCTRL_SRC_DFLL48M);
    gclk_wait_for_sync();

    // GCLK 4 (12MHz)
    gclk_write_GENDIV(GCLK_GENDIV_ID_GCLK4 | GCLK_GENDIV_DIV(4));
    gclk_write_GENCTRL(GCLK_GENCTRL_ID_GCLK4 | GCLK_GENCTRL_GENEN | GCLK_GENCTRL_OE | GCLK_GENCTRL_SRC_DFLL48M);
    gclk_wait_for_sync();

    // GCLK6 (1MHz)
    gclk_write_GENDIV(GCLK_GENDIV_ID_GCLK6 | GCLK_GENDIV_DIV(48));
    gclk_write_GENCTRL(GCLK_GENCTRL_ID_GCLK6 | GCLK_GENCTRL_GENEN | GCLK_GENCTRL_SRC_DFLL48M);
}

void system_init2(void)
{
    gclk_wait_for_sync();
    // Slow clock for all SERCOMs - not really needed
    gclk_write_CLKCTRL(GCLK_CLKCTRL_GEN_GCLK6 | GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_ID_SERCOMX_SLOW);
    // EIC
    gclk_write_CLKCTRL(GCLK_CLKCTRL_GEN_GCLK4 // 12MHz
                       | GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_ID_EIC);
    //DAC
    gclk_write_CLKCTRL(GCLK_CLKCTRL_GEN_GCLK4 // 12MHz
                       | GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_ID_DAC);

    // AC
    gclk_write_CLKCTRL(GCLK_CLKCTRL_GEN_GCLK4 // 12MHz
                       | GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_ID_AC_ANA);
    gclk_write_CLKCTRL(GCLK_CLKCTRL_GEN_GCLK6 // 1MHz
                       | GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_ID_AC_DIG);

    // Enable power to modules
    PM->APBAMASK.reg |= PM_APBAMASK_EIC;
    PM->APBBMASK.reg |= PM_APBBMASK_NVMCTRL;
    PM->APBCMASK.reg |= 0

#ifdef USE_I2CM1
                        | PM_APBCMASK_I2CM1 // APBCMASK != APBAMASK
#endif
#ifdef USE_I2CM2
                        | PM_APBCMASK_I2CM2
#endif
#ifdef USE_CONSOLE
                        | PM_APBCMASK_CONSOLE
#endif
#ifdef USE_I2CS
                        | PM_APBCMASK_I2CS
#endif
                        | PM_APBCMASK_ADC | PM_APBCMASK_TC0 | PM_APBCMASK_TC1
#ifdef USE_TC2
                        | PM_APBCMASK_TC2
#endif
#ifdef USE_TC3
                        | PM_APBCMASK_TC3
#endif
#ifdef USE_TC4
                        | PM_APBCMASK_TC4
#endif
#ifdef USE_DAC
                        | PM_APBCMASK_DAC
#endif
#ifdef USE_AC
                        | PM_APBCMASK_AC
#endif
        ; // terminate PM->APBCMASK.reg |=
          /*
gclk_write_CLKCTRL(GCLK_CLKCTRL_GEN_GCLK6
                | GCLK_CLKCTRL_CLKEN
                | GCLK_CLKCTRL_ID_EVSYS_CHANNEL_1); */

    /* Initialize GPIO and EIC initial state  */
    Hw_Encoding_Read_And_Store_Hw_Version_Info(); //NOTE: must be called before gpio init
    powerManagerInitialize();                     //NOTE: must be called before gpio init
    gpio_init();
    etsocCmdHandlerInitialize();

#ifdef USE_CONSOLE
    /* Console UART */
    // 12MHz to Console UART
    gclk_write_CLKCTRL(CONSOLE_GCLK | GCLK_CLKCTRL_CLKEN | CONSOLE_GCLK_ID);
    // reset Console UART and wait for it to complete, enable is cleared
    CONSOLE_PORT->USART.CTRLA.reg = SERCOM_USART_CTRLA_SWRST;
    while (CONSOLE_PORT->USART.CTRLA.reg & SERCOM_USART_CTRLA_SWRST)
        ;
    CONSOLE_PORT->USART.CTRLA.reg = SERCOM_USART_CTRLA_MODE_USART_INT_CLK | SERCOM_USART_CTRLA_TXPO_PAD2 |
                                    SERCOM_USART_CTRLA_RXPO_PAD3 | SERCOM_USART_CTRLA_FORM_0 // No parity
                                    | SERCOM_USART_CTRLA_DORD;                               // LSB first
    CONSOLE_PORT->USART.CTRLB.reg = SERCOM_USART_CTRLB_TXEN | SERCOM_USART_CTRLB_RXEN |
                                    SERCOM_USART_CTRLB_CHSIZE(0); // 8 bit

    CONSOLE_PORT->USART.BAUD.reg = UART_BAUD(CONSOLE_BAUDRATE, CONSOLE_FREQUENCY);

    gpio_port_set_pin_function(CONSOLE_RX, MUX_CONSOLE_RX);
    gpio_port_set_pin_function(CONSOLE_TX, MUX_CONSOLE_TX);

    CONSOLE_PORT->USART.CTRLA.reg |= SERCOM_USART_CTRLA_ENABLE;
#endif
#ifdef USE_I2CM1
    /*  I2C Master PWR_I2C to PMIC devices */
    // 12MHz to PWR_I2C
    gclk_write_CLKCTRL(I2CM1_GCLK | GCLK_CLKCTRL_CLKEN | I2CM1_GCLK_ID);
    // Reset I2C and wait for it to complete, enable is cleared
    I2CM1_SERCOM->I2CM.CTRLA.reg = SERCOM_I2CM_CTRLA_SWRST;
    while (I2CM1_SERCOM->I2CM.CTRLA.reg & SERCOM_I2CM_CTRLA_SWRST)
        ;
    i2cm_wait_for_sync(I2CM1_SERCOM);

    // pinout = 0, not using 4 wire, check LOWTOUT, INACTOUT later
    I2CM1_SERCOM->I2CM.CTRLA.reg = SERCOM_I2CM_CTRLA_MODE_I2C_MASTER |
                                   SERCOM_I2CM_CTRLA_SDAHOLD(2); // 300-600ns hold time
    // QCEN not used, cmd and ackact used later
    I2CM1_SERCOM->I2CM.CTRLB.reg = SERCOM_I2CM_CTRLB_SMEN;
    I2CM1_SERCOM->I2CM.BAUD.reg = SERCOM_I2CM_BAUD_BAUDLOW(I2CM_BAUDLOW(I2CM_BAUDRATE, I2CM_FREQUENCY)) |
                                  SERCOM_I2CM_BAUD_BAUD(I2CM_BAUD(I2CM_BAUDRATE, I2CM_FREQUENCY));

    gpio_port_set_pin_function(PSBUS0_SDA, MUX_PSBUS0_SDA);
    gpio_port_set_pin_function(PSBUS0_SCL, MUX_PSBUS0_SCL);

    I2CM1_SERCOM->I2CM.CTRLA.reg |= SERCOM_I2CM_CTRLA_ENABLE;
    i2cm_wait_for_sync(I2CM1_SERCOM);

    I2CM1_SERCOM->I2CM.INTFLAG.reg = SERCOM_I2CM_INTFLAG_MASK;
    I2CM1_SERCOM->I2CM.INTENCLR.reg = SERCOM_I2CM_INTENCLR_MASK;
#endif
#ifdef USE_I2CM2
    /*  I2C Master PWR_I2C to PMIC devices */
    // 12MHz to PWR_I2C
    gclk_write_CLKCTRL(I2CM2_GCLK | GCLK_CLKCTRL_CLKEN | I2CM2_GCLK_ID);
    // Reset I2C and wait for it to complete, enable is cleared
    I2CM2_SERCOM->I2CM.CTRLA.reg = SERCOM_I2CM_CTRLA_SWRST;
    while (I2CM2_SERCOM->I2CM.CTRLA.reg & SERCOM_I2CM_CTRLA_SWRST)
        ;
    i2cm_wait_for_sync(I2CM2_SERCOM);

    // pinout = 0, not using 4 wire, check LOWTOUT, INACTOUT later
    I2CM2_SERCOM->I2CM.CTRLA.reg = SERCOM_I2CM_CTRLA_MODE_I2C_MASTER |
                                   SERCOM_I2CM_CTRLA_SDAHOLD(2); // 300-600ns hold time
    // QCEN not used, cmd and ackact used later
    I2CM2_SERCOM->I2CM.CTRLB.reg = SERCOM_I2CM_CTRLB_SMEN;
    I2CM2_SERCOM->I2CM.BAUD.reg = SERCOM_I2CM_BAUD_BAUDLOW(I2CM_BAUDLOW(I2CM_BAUDRATE, I2CM_FREQUENCY)) |
                                  SERCOM_I2CM_BAUD_BAUD(I2CM_BAUD(I2CM_BAUDRATE, I2CM_FREQUENCY));

    gpio_port_set_pin_function(PSBUS1_SDA, MUX_PSBUS1_SDA);
    gpio_port_set_pin_function(PSBUS1_SCL, MUX_PSBUS1_SCL);

    I2CM2_SERCOM->I2CM.CTRLA.reg |= SERCOM_I2CM_CTRLA_ENABLE;
    i2cm_wait_for_sync(I2CM2_SERCOM);

    I2CM2_SERCOM->I2CM.INTFLAG.reg = SERCOM_I2CM_INTFLAG_MASK;
    I2CM2_SERCOM->I2CM.INTENCLR.reg = SERCOM_I2CM_INTENCLR_MASK;
#endif
#ifdef USE_I2CS
    I2CS_Init();
#endif
#ifdef USE_ADC
    /* ADC  */
    // 1MHz to ADC
    gclk_write_CLKCTRL(ADC_GCLK | GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_ID_ADC);
    // enable the voltage reference - temperature sensor is also enabled
    SYSCTRL->VREF.reg |= SYSCTRL_VREF_BGOUTEN | SYSCTRL_VREF_TSEN;
    // Reset ADC and wait for it to complete, must be disabled before reset
    ADC->CTRLA.reg &= ~ADC_CTRLA_ENABLE;
    while (ADC->STATUS.reg & ADC_STATUS_SYNCBUSY)
        ;
    // select 1V reference
    ADC->REFCTRL.reg = ADC_REFCTRL_REFCOMP | ADC_REFCTRL_REFSEL_INT1V;
    // 250KHz sample rate 12bit resolution right adjusted single shot mode
    ADC->CTRLB.reg = ADC_CTRLB_PRESCALER_DIV4 | ADC_CTRLB_RESSEL_16BIT;
    // Gain 1, IMON channel selected
    ADC->CALIB.reg =
        ADC_CALIB_LINEARITY_CAL((EXTRACT_NVM_CALIB(NVM_ADC_LINEARITY_LO_Pos, NVM_ADC_LINEARITY_LO_Len)) |
                                ((EXTRACT_NVM_CALIB(NVM_ADC_LINEARITY_HI_Pos, NVM_ADC_LINEARITY_HI_Len)) << 5)) |
        ADC_CALIB_BIAS_CAL(EXTRACT_NVM_CALIB(NVM_ADC_BIAS_Pos, NVM_ADC_BIAS_Len));
    ADC->AVGCTRL.reg = ADC_AVGCTRL_SAMPLENUM_64 | ADC_AVGCTRL_ADJRES(4);
#ifdef USE_CALIBRATION
    if (calibration->gain != 0xFFFFFFFF)
    {
        ADC->GAINCORR.reg = ADC_GAINCORR_GAINCORR(calibration->gain);
        ADC->OFFSETCORR.reg = ADC_OFFSETCORR_OFFSETCORR(calibration->offset);
        ADC->CTRLB.reg |= ADC_CTRLB_CORREN;
        while (ADC->STATUS.reg & ADC_STATUS_SYNCBUSY)
            ;
    }
#endif
    ADC->CTRLA.reg |= ADC_CTRLA_ENABLE;
    while (ADC->STATUS.reg & ADC_STATUS_SYNCBUSY)
        ;
    ADC->INTENCLR.reg = ADC_INTENCLR_MASK;
    ADC->INTFLAG.reg = ADC_INTFLAG_OVERRUN | ADC_INTFLAG_RESRDY;
    ADC->INTENSET.reg = ADC_INTENSET_OVERRUN | ADC_INTENSET_RESRDY;
    ADC->INPUTCTRL.bit.MUXNEG = 0x18;
    NVIC_DisableIRQ(ADC_IRQn);
    // start the ADC
    ADC->INPUTCTRL.bit.MUXPOS |= gpio_initial_Ain(); //other bit fields are 0
    ADC->SWTRIG.bit.START |= 1;                      //other bit fields are 0
#endif
#if defined(TC0) || defined(TC1) || defined(USE_I2CM1)
    gclk_write_CLKCTRL(GCLK_CLKCTRL_GEN_GCLK6 // 1 MHz
                       | GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_ID_TC0_TC1);
    while (GCLK->STATUS.bit.SYNCBUSY)
        ; // Wait for synchronization
#endif
#if defined(TC4) || defined(TC5) || defined(USE_I2CM2)
    gclk_write_CLKCTRL(GCLK_CLKCTRL_GEN_GCLK6 // 1 MHz
                       | GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_ID_TC4_TC5);
    while (GCLK->STATUS.bit.SYNCBUSY)
        ; // Wait for synchronization
#endif

#ifdef USE_TC0
    /* TC0 Timer - watchdog timer */
    // 1MHz to TC0 with 64usec prescale
    // Reset WDT = TCn see board_defs.h - it was recommended to clear enable first
    WDT_TC->COUNT16.CTRLA.reg &= ~TC_CTRLA_ENABLE;
    while (WDT_TC->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY)
        ;
    WDT_TC->COUNT16.CTRLA.reg = TC_CTRLA_SWRST;
    while (WDT_TC->COUNT16.CTRLA.reg & TC_CTRLA_SWRST)
        ;
    // 16 bit counter, reset on match, count at 1MHz/64 or 64usec period or 15.625KHz
    // counter is preset to 3.2s interval and stopped with interrupts disabled.
    WDT_TC->COUNT16.CTRLA.reg = TC_CTRLA_MODE_COUNT16 | TC_CTRLA_WAVEGEN_MFRQ | TC_CTRLA_PRESCALER_DIV64 |
                                TC_CTRLA_PRESCSYNC_GCLK;
    while (WDT_TC->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY)
        ;
    WDT_TC->COUNT16.COUNT.reg = 0;
    while (WDT_TC->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY)
        ;
    WDT_TC->COUNT16.CC[0].reg = DEFAULT_WDT;
    while (WDT_TC->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY)
        ;
    WDT_TC->COUNT16.INTFLAG.reg = TC_INTFLAG_MASK;
    WDT_TC->COUNT16.CTRLA.reg |= TC_CTRLA_ENABLE;
    while (WDT_TC->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY)
        ;
    WDT_TC->COUNT16.CTRLBSET.reg = TC_CTRLBSET_CMD_STOP;
    while (WDT_TC->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY)
        ;
    NVIC_DisableIRQ(WDT_IRQn);
#endif
#undef ENABLE
#if defined(USE_TC2) || defined(USE_TC3)
    gclk_write_CLKCTRL(GCLK_GENDIV_ID_GCLK0 // 48 MHz
                       | GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_ID_TC2_TC3);
    while (GCLK->STATUS.bit.SYNCBUSY)
        ; // Wait for synchronization
#endif
#ifdef USE_TC2
    TC2->COUNT16.CTRLA.reg = TC_CTRLA_MODE_COUNT16 | TC_CTRLA_WAVEGEN_NFRQ // MAX count 35536 --> 732 Hz
                             | TC_CTRLA_PRESCALER_DIV1 | TC_CTRLA_PRESCSYNC_GCLK;

    TC2->COUNT16.CTRLA.bit.ENABLE = 1; // Enable TC2
    while (TC2->COUNT16.STATUS.bit.SYNCBUSY)
        ; // Wait for synchronization

    TC2->COUNT16.READREQ.reg = TC_READREQ_RCONT |     // Enable a continuous read request
                               TC_READREQ_ADDR(0x10); // Offset of the 32-bit COUNT register
    while (TC2->COUNT16.STATUS.bit.SYNCBUSY)
        ;                                        // Wait for (read) synchronization
    TC2->COUNT16.INTENSET.reg = TC_INTENSET_OVF; // Enable overflow interrupt
    TC2->COUNT16.INTFLAG.reg = TC_INTFLAG_MASK;
    while (TC2->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY)
        ;

    NVIC_DisableIRQ(TC2_IRQn);
#endif
#ifdef USE_TC3
    TC3->COUNT16.CTRLA.reg &= ~TC_CTRLA_ENABLE;
    while (TC3->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY)
        ;
    TC3->COUNT16.CTRLA.reg = TC_CTRLA_SWRST;
    while (TC3->COUNT16.CTRLA.reg & TC_CTRLA_SWRST)
        ;
    TC3->COUNT16.CTRLA.reg = TC_CTRLA_MODE_COUNT16 | TC_CTRLA_WAVEGEN_MFRQ | TC_CTRLA_PRESCALER_DIV1 |
                             TC_CTRLA_PRESCSYNC_GCLK;
    while (TC3->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY)
        ;
    TC3->COUNT16.INTENSET.reg = TC_INTENSET_OVF; // Enable overflow interrupt
    TC3->COUNT16.INTFLAG.reg = TC_INTFLAG_MASK;
    TC3->COUNT16.COUNT.reg = 0;
    while (TC3->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY)
        ;
    TC3->COUNT16.CC[0].reg = CPU_FREQUENCY / 2000; // 2 kHz
    while (TC3->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY)
        ;
    TC3->COUNT16.CTRLA.reg |= TC_CTRLA_ENABLE;
    while (TC3->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY)
        ;
    NVIC_DisableIRQ(TC3_IRQn);
#endif

#ifdef USE_TC4
    TC4->COUNT16.CTRLA.reg = TC_CTRLA_MODE_COUNT16 | TC_CTRLA_WAVEGEN_MFRQ | TC_CTRLA_PRESCALER_DIV1 |
                             TC_CTRLA_PRESCSYNC_PRESC;

    TC4->COUNT16.CTRLA.bit.ENABLE = 1; // Enable TC4
    while (TC4->COUNT16.STATUS.bit.SYNCBUSY)
        ; // Wait for synchronization

    TC4->COUNT16.CTRLBSET.reg = TC_CTRLBSET_CMD_STOP | TC_CTRLBSET_ONESHOT;
    while (TC4->COUNT16.STATUS.bit.SYNCBUSY)
        ;                                        // Wait for (read) synchronization
    TC4->COUNT16.INTENSET.reg = TC_INTENSET_OVF; // Enable overflow interrupt
    TC4->COUNT16.INTFLAG.reg = TC_INTFLAG_MASK;

    NVIC_DisableIRQ(TC4_IRQn);
#endif

#ifdef USE_NVM
    NVMCTRL->CTRLB.reg = NVMCTRL_CTRLB_RWS(3) | NVMCTRL_CTRLB_MANW;
    NVMCTRL->INTENCLR.reg = NVMCTRL_INTENCLR_READY | NVMCTRL_INTENCLR_ERROR;
    NVIC_DisableIRQ(NVMCTRL_IRQn);
#endif

#ifdef USE_DAC
    while (DAC->STATUS.bit.SYNCBUSY)
        ;
    DAC->CTRLA.reg = 0;
    while (DAC->STATUS.bit.SYNCBUSY)
        ;
    DAC->CTRLB.reg = DAC_CTRLB_IOEN;
    while (DAC->STATUS.bit.SYNCBUSY)
        ;
    DAC->DATA.reg = (uint32_t)(.079729 * V12FAILmv * (1 << 10) + 500) / 1000;
    while (DAC->STATUS.bit.SYNCBUSY)
        ;
    DAC->CTRLA.bit.ENABLE = 1;
#endif

#ifdef USE_AC
    while (AC->STATUSB.bit.SYNCBUSY)
        ;
    AC->CTRLA.reg = 0;
    while (AC->STATUSB.bit.SYNCBUSY)
        ;
    AC->CTRLB.reg = 0;
    while (AC->STATUSB.bit.SYNCBUSY)
        ;
    AC->COMPCTRL[0].reg = AC_COMPCTRL_FLEN(AC_COMPCTRL_FLEN_MAJ5_Val) | AC_COMPCTRL_HYST | AC_COMPCTRL_MUXPOS_PIN0 |
                          AC_COMPCTRL_INTSEL_TOGGLE | AC_COMPCTRL_SPEED_HIGH | AC_COMPCTRL_ENABLE;
    while (AC->STATUSB.bit.SYNCBUSY)
        ;
    AC->CTRLA.reg = AC_CTRLA_LPMUX | AC_CTRLA_ENABLE;
#endif
}

int systemEnableHwInterrupts(void)
{
    uint32_t volatile dfltPrio = NVIC_GetPriority(CONSOLE_IRQn); // dfltPrio == 0 = highest urgency
    NVIC_SetPriority(CONSOLE_IRQn, dfltPrio + 3); // lowest urgency - prevent console from blocking other ISRs

#ifdef USE_I2CS
    NVIC_SetPriority(I2CS_IRQn, dfltPrio + 2); // 3rd highest urgency - allow TC3 to preempt
    NVIC_ClearPendingIRQ(I2CS_IRQn);
    NVIC_EnableIRQ(I2CS_IRQn);
#endif
#ifdef USE_ADC
    NVIC_SetPriority(ADC_IRQn, dfltPrio + 2); // 3rd highest urgency - allow TC3 to preempt
    NVIC_ClearPendingIRQ(ADC_IRQn);
    NVIC_EnableIRQ(ADC_IRQn);
#endif
#ifdef USE_TC0
    NVIC_SetPriority(WDT_IRQn, dfltPrio + 2); // 3rd highest urgency - allow TC3 to preempt
    NVIC_ClearPendingIRQ(WDT_IRQn);
    NVIC_EnableIRQ(WDT_IRQn);
#endif
#ifdef USE_TC2
#endif
#ifdef USE_TC3
    NVIC_SetPriority(TC3_IRQn, dfltPrio + 0); // highest urgency - preempt all other ISRs
    NVIC_ClearPendingIRQ(TC3_IRQn);
    NVIC_EnableIRQ(TC3_IRQn);
#endif
#ifdef USE_TC4
    NVIC_SetPriority(TC4_IRQn, dfltPrio + 2); // 3rd highest urgency - allow TC3 to preempt
    NVIC_ClearPendingIRQ(TC4_IRQn);
    NVIC_EnableIRQ(TC4_IRQn);
#endif
#ifdef USE_EIC_INTERRUPT
    NVIC_SetPriority(EIC_IRQn, dfltPrio + 2); // 3rd highest urgency - allow TC3 to preempt
    NVIC_ClearPendingIRQ(EIC_IRQn);
    NVIC_EnableIRQ(EIC_IRQn);
#endif
#ifdef USE_NVM
    NVIC_SetPriority(NVMCTRL_IRQn, dfltPrio + 2); // 3rd highest urgency - allow TC3 to preempt
    nvmInit();
#endif
#ifdef USE_DAC
#endif
#ifdef USE_AC
    NVIC_SetPriority(AC_IRQn, dfltPrio + 2); // 2nd highest urgency - allow TC3 to preempt
    NVIC_ClearPendingIRQ(AC_IRQn);
    NVIC_EnableIRQ(AC_IRQn);
#endif
    return STATUS_SUCCESS;
}

void systemDisableHwInterrupts(void)
{
#ifdef USE_I2CS
    NVIC_DisableIRQ(I2CS_IRQn);
#endif
#ifdef USE_ADC
    NVIC_DisableIRQ(ADC_IRQn);
#endif
#ifdef USE_TC0
    NVIC_DisableIRQ(WDT_IRQn);
#endif
#ifdef USE_TC2
#endif
#ifdef USE_TC3
    NVIC_DisableIRQ(TC3_IRQn);
#endif
#ifdef USE_TC4
    NVIC_DisableIRQ(TC4_IRQn);
#endif
#ifdef USE_EIC_INTERRUPT
    NVIC_DisableIRQ(EIC_IRQn);
#endif
#ifdef USE_DAC
#endif
#ifdef USE_AC
    NVIC_DisableIRQ(AC_IRQn);
#endif
}
