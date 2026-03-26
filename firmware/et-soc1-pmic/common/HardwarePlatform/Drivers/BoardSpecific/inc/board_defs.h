/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file board_defs.h
    \brief Local board definition for Bring up Board (BUB).
    For use with SAMD20J17A libraries asf3, CMSIS and FreeRTOS.
*/
/***********************************************************************/

#ifndef __BOARD_DEFS_H__
#define __BOARD_DEFS_H__

#include <stdint.h>
#include <stdbool.h>
#include "samd20.h"

#define V12FAILmv 11000

#define CONSOLE_ABORT_EVENT    (1 << 0)
#define CONSOLE_RX_EVENT       (1 << 1)
#define CONSOLE_OVERFLOW_EVENT (1 << 2)
#define ADC_EVENT              (1 << 3)
#define NMI_EVENT              (1 << 4)
#define EIC_EVENT              (0xFFFF0000)

#define EXTRACT_NVM_CALIB(pos, len) \
    ((*((uint32_t *)(NVMCTRL_OTP4) + ((pos) / 32)) >> ((pos) % 32)) & ((1 << (len)) - 1))

#define GCLK3_FREQUENCY      32768UL
#define DFLL_MULTIPLY_FACTOR 1464
#define GCLK4_FREQUENCY      12000000
#define GCLK6_FREQUENCY      2000000

#define I2CM1_IRQn        SERCOM3_IRQn
#define I2CM1_Handler     SERCOM3_Handler
#define I2CM1_SERCOM      SERCOM3
#define I2CM1_GCLK        GCLK_CLKCTRL_GEN_GCLK4
#define I2CM1_GCLK_ID     GCLK_CLKCTRL_ID_SERCOM3_CORE
#define PM_APBCMASK_I2CM1 PM_APBCMASK_SERCOM3

#define I2CM_BAUDRATE  400000UL
#define I2CM_FREQUENCY GCLK4_FREQUENCY

#define I2CM2_IRQn        SERCOM2_IRQn
#define I2CM2_Handler     SERCOM2_Handler
#define I2CM2_SERCOM      SERCOM2
#define I2CM2_GCLK        GCLK_CLKCTRL_GEN_GCLK4
#define I2CM2_GCLK_ID     GCLK_CLKCTRL_ID_SERCOM2_CORE
#define PM_APBCMASK_I2CM2 PM_APBCMASK_SERCOM2

#define SP_SLAVE_ADDRESS 0x42
#define I2CS_IRQn        SERCOM1_IRQn
#define I2CS_Handler     SERCOM1_Handler
#define I2CS_SERCOM      SERCOM1
#define I2CS_GCLK        GCLK_CLKCTRL_GEN_GCLK4
#define I2CS_GCLK_ID     GCLK_CLKCTRL_ID_SERCOM1_CORE
#define PM_APBCMASK_I2CS PM_APBCMASK_SERCOM1

#define CONSOLE_PORT        SERCOM0
#define CONSOLE_Handler     SERCOM0_Handler
#define CONSOLE_IRQn        SERCOM0_IRQn
#define CONSOLE_GCLK        GCLK_CLKCTRL_GEN_GCLK4
#define CONSOLE_GCLK_ID     GCLK_CLKCTRL_ID_SERCOM0_CORE
#define PM_APBCMASK_CONSOLE PM_APBCMASK_SERCOM0
#define CONSOLE_BAUDRATE    115200
#define CONSOLE_FREQUENCY   GCLK4_FREQUENCY
#define CONSOLE_TXPO        SERCOM_USART_CTRLA_TXPO_PAD2
#define CONSOLE_RXPO        SERCOM_USART_CTRLA_RXPO_PAD3

// ADC Sample rate is period 16us 0.5second interval
#define ADC_GCLK      GCLK_CLKCTRL_GEN_GCLK6
#define ADC_FREQUENCY GCLK6_FREQUENCY
#define ADC_RATE      31250

#if defined(__SAMD20J17__) || defined(__ATSAMD20J17__)
#define NVM_PAGE_COUNT 2048
#elif defined(__SAMD20J18__) || defined(__ATSAMD20J18__)
#define NVM_PAGE_COUNT      FLASH_NB_OF_PAGES
#define PAGE_COUNT_PER_ROW  4
#define WORD_COUNT_PER_PAGE (FLASH_PAGE_SIZE / sizeof(uint32_t))
#define WORD_COUNT_PER_ROW  (FLASH_PAGE_SIZE * PAGE_COUNT_PER_ROW / sizeof(uint32_t))
#define FLASH_ROW_SIZE      (FLASH_PAGE_SIZE * PAGE_COUNT_PER_ROW)
#endif

/* watchdog initial value (3.2s * 1MHz)/64 */
#define DEFAULT_WDT 50000
/* Watchdog Timer/Counter */
#define WDT_TC      TC0
#define CLKCTRL_WDT GCLK_CLKCTRL_ID_TC0_TC1
#define WDT_IRQn    TC0_IRQn
#define WDT_Handler TC0_Handler

#define EXTRACT_NVM_CALIB(pos, len) \
    ((*((uint32_t *)(NVMCTRL_OTP4) + ((pos) / 32)) >> ((pos) % 32)) & ((1 << (len)) - 1))
/**
 * @brief NVM CALIBRATION Offset Position and Size
 */
#define NVM_OSC32K_CAL_Pos       38
#define NVM_OSC32K_CAL_Len       7
#define NVM_DFLL_COARSE_Pos      58
#define NVM_DFLL_COARSE_Len      6
#define NVM_DFLL_FINE_Pos        64
#define NVM_DFLL_FINE_Len        10
#define NVM_ADC_LINEARITY_LO_Pos 27
#define NVM_ADC_LINEARITY_LO_Len 5
#define NVM_ADC_LINEARITY_HI_Pos 32
#define NVM_ADC_LINEARITY_HI_Len 3
#define NVM_ADC_BIAS_Pos         35
#define NVM_ADC_BIAS_Len         2
/*
 * Atmel forgot these, stronzo
 */
#define EVENT_GEN_TC0_MC0    0x1D
#define EVENT_USER_ADC_START 0x08

/* Pin Defines */
#define I2C0_SDA PIN_PA16 /* SERCOM1  */
#define I2C0_SCL PIN_PA17

#define PSBUS0_SDA PIN_PA22 /* SERCOM3 */
#define PSBUS0_SCL PIN_PA23

#define PSBUS1_SDA PIN_PA12 /* SERCOM2 */
#define PSBUS1_SCL PIN_PA13

#define CONSOLE_RX PIN_PA07 /* SERCOM0 */
#define CONSOLE_TX PIN_PA06

/* Peripheral Pin Mux */
#define MUX_PSBUS0_SCL MUX_PA23C_SERCOM3_PAD1 /* SERCOM 3 I2C Master */
#define MUX_PSBUS0_SDA MUX_PA22C_SERCOM3_PAD0

#define MUX_PSBUS1_SCL MUX_PA13C_SERCOM2_PAD1 /* SERCOM 2 I2C Master */
#define MUX_PSBUS1_SDA MUX_PA12C_SERCOM2_PAD0

#define MUX_I2C0_SCL MUX_PA17C_SERCOM1_PAD1 /* SERCOM 1 I2C Slave */
#define MUX_I2C0_SDA MUX_PA16C_SERCOM1_PAD0

#define MUX_CONSOLE_RX MUX_PA07D_SERCOM0_PAD3 /* SERCOM 0 USART */
#define MUX_CONSOLE_TX MUX_PA06D_SERCOM0_PAD2

/* GPO Control 0x02 */
#define GPO0 (1 << 0)
#define GPO1 (1 << 1)
#define GPO2 (1 << 2)
#define GPO3 (1 << 3)
#define GPO4 (1 << 4)
#define GPO5 (1 << 5)
#define GPO6 (1 << 6)
#define GPO7 (1 << 7)
/* Interrupt Controller Configuration 0x03  and Interrupt Cause */
#define OV_TEMP       (1 << 0)
#define OV_POWER      (1 << 1)
#define PWR_FAIL      (1 << 2)
#define MINION_DROOP  (1 << 3)
#define MESSAGE_ERROR (1 << 5)
#define REG_COM_FAIL  (1 << 6)
/* Reset Control */
#define WDT_EN   (1 << 4)
#define PERST_EN (1 << 5)
/* Reset Causation */
#define PWR_ON_RESET   (1 << 0)
#define SOFTWARE_RESET (1 << 2)
#define WDT_RESET      (1 << 3)
#define MYSTERY_RESET  (1 << 4)
/* watchdog configuration */
// watchdog time in msec, 0 is 200ms with steps of 200ms up to 3200ms
#define WDT_TIME_TO_COUNT(x) (((((x)&0xF) << 0) + 1) * 3125)

// wdt_assert timeout assert reset if 1, perst if 0
#define WDT_ASSERT (1 << 4)
// wdt_enable set to enable watchdog counter
#define WDT_ENABLE (1 << 5)
/* Reset Command */
#define FORCE_PERST       (1 << 0)
#define FORCE_RESET       (1 << 1)
#define FORCE_POWER_CYCLE (1 << 2)
#define FORCE_SHUTDOWN    (1 << 3)
/* Watchdog Reset */
#define WDT_POKE (1 << 0)

#endif // __BOARD_DEFS_H__
