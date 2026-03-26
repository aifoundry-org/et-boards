/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file gpiopins_pcie_v3.c
    \brief GPIO pins definition for PCIE v3 board
*/
/***********************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include "gpio.h"
#include "gpiopins_pcie_v3.h"
#include "samd20.h"
#include "utils.h"

/***********************************************************************
 * GLOBAL Data types:      Defininition of global data types
***********************************************************************/
// clang-format off
enum {
// OUTPUTS
    SOC_RESET_N     = PIN_PB10,    /* MCU_RESET_OUT_N */
    PMIC_RDY        = PIN_PB05,                          
    MCU_PERST_OUT_N = PIN_PB23,    /* SOC_PERST0_N */    
    PMIC_INT_N      = PIN_PB11,
    TPS_RESET_N     = PIN_PB06,
    MCU_LED         = PIN_PB02,

    NOC_ENA         = PIN_PA27,  // VDD_NOC_EN
    MNN_ENA         = PIN_PB03,  // VDD_MNN_EN
    MXN_EN          = PIN_PA18,  // MXN_EN
    P_QLP_EN        = PIN_PB30,  // VDD_QLP_EN
    P_SRAM_EN       = PIN_PB31,  // VDD_SRAM_EN
    P_LOGIC_EN      = PIN_PB00,  // VDD_LOGIC_EN
    P_DDR_EN        = PIN_PB01,  // VDD_DDR_EN
    P_Q_EN          = PIN_PB07,  // VDD_Q_EN
    P_PCIE_EN       = PIN_PA04,  // VDD_1P5_EN
    P1_8V_EN        = PIN_PA25,  // VDD_1P8_EN
    AP1_8V_EN       = PIN_PA28,  // VDDA_1P8_EN
    P3_3V_EN        = PIN_PA24,  // P3_3V_EN

/* TP indeterminate direction */
    MCU_GPIO0       = PIN_PA10,  // TP41
    MCU_GPIO1       = PIN_PA11,  // TP42
    MCU_GPIO2       = PIN_PA08,  // TP43
    MCU_GPIO4       = PIN_PA03,  // TP25
    MCU_GPIO_PAD0   = PIN_PB16,  // TP47
    MCU_GPIO_PAD1   = PIN_PB17,  // TP44
    MCU_GPIO_PAD2   = PIN_PA20,  // TP45
    MCU_GPIO_PAD3   = PIN_PA21,  // TP48

/* INPUTS */
    PERST_IN_N      = PIN_PA01,    /* CE_PERST_BUF_N */
    WDT_RESET_N     = PIN_PA15,    /* WDT_RSTOUT_N */
    TEMP_ALERT_N    = PIN_PA02,    /* TMP_ALERT_N */
    TPS_ALERT       = PIN_PB04,
    SRAM_ALERT      = PIN_PA05,
    SRAM_FAULT      = PIN_PB12,

    PG_BUS0         = PIN_PA00,
    PG_BUS1         = PIN_PA19,
    PG_12V          = PIN_PA09,
    PG_3P3V         = PIN_PB22,

/* ANALOG */
    IMON_12V        = PIN_PB08,
    VMON_12V        = PIN_PB09,
};
// clang-format on

/***********************************************************************
 * GLOBAL variables:  Defininition of global variables and constants
***********************************************************************/
// clang-format off
gpioPinIdMap_t const gpioPinIdMap_pcie_v3[] = {
    {.id = SOC_RST_OUT,            .pin = SOC_RESET_N     },
    {.id = PMIC_RDY_OUT,           .pin = PMIC_RDY        },
    {.id = SOC_PERST_OUT,          .pin = MCU_PERST_OUT_N },
    {.id = PMIC_INT_OUT,           .pin = PMIC_INT_N      },
    {.id = MNN_NOC_PWR_RST_OUT,    .pin = TPS_RESET_N     },
    {.id = NOC_EN_OUT,             .pin = NOC_ENA         },
    {.id = MNN_EN_OUT,             .pin = MNN_ENA         },
    {.id = MXN_EN_OUT,             .pin = MXN_EN,         },
    {.id = QLP_EN_OUT,             .pin = P_QLP_EN        },
    {.id = SRAM_EN_OUT,            .pin = P_SRAM_EN       },
    {.id = LOGIC_EN_OUT,           .pin = P_LOGIC_EN      },
    {.id = DDR_EN_OUT,             .pin = P_DDR_EN        },
    {.id = Q_EN_OUT,               .pin = P_Q_EN          },
    {.id = PCIE_EN_OUT,            .pin = P_PCIE_EN       },
    {.id = VDD_1P8V_EN_OUT,        .pin = P1_8V_EN        },
    {.id = VDDA_1P8V_EN_OUT,       .pin = AP1_8V_EN       },
    {.id = VDD_3P3V_EN_OUT,        .pin = P3_3V_EN        },
    {.id = MCU_GPIO0_OUT,          .pin = MCU_GPIO0       },
    {.id = MCU_GPIO1_OUT,          .pin = MCU_GPIO1       },
    {.id = MCU_GPIO2_OUT,          .pin = MCU_GPIO2       },
    {.id = MCU_GPIO4_OUT,          .pin = MCU_GPIO4       },
    {.id = MCU_GPIO_PAD0_OUT,      .pin = MCU_GPIO_PAD0   },
    {.id = MCU_GPIO_PAD1_OUT,      .pin = MCU_GPIO_PAD1   },
    {.id = MCU_GPIO_PAD2_OUT,      .pin = MCU_GPIO_PAD2   },
    {.id = MCU_GPIO_PAD3_OUT,      .pin = MCU_GPIO_PAD3   },
    {.id = PERST_IN,               .pin = PERST_IN_N      },
    {.id = WDT_RST_IN,             .pin = WDT_RESET_N     },
    {.id = TEMP_ALERT_IN,          .pin = TEMP_ALERT_N    },
    {.id = MNN_NOC_ALERT_IN,       .pin = TPS_ALERT       },
    {.id = POWER_GOOD_12V_IN,      .pin = PG_12V          },
    {.id = POWER_GOOD_3P3V_IN,     .pin = PG_3P3V         },
    {.id = POWER_GOOD_BUS0_IN,     .pin = PG_BUS0         },
    {.id = POWER_GOOD_BUS1_IN,     .pin = PG_BUS1         },
    {.id = SRAM_ALERT_IN,          .pin = SRAM_ALERT      },
    {.id = SRAM_FAULT_IN,          .pin = SRAM_FAULT      },
    {.id = VMON_12V_AIN,           .pin = VMON_12V        },
    {.id = IMON_12V_AIN,           .pin = IMON_12V        },
};
//  clang-format on

// clang-format off
gpioConfig_t gpioConfig_pcie_v3[] = {
    {.pin = SOC_RESET_N,     .type = GPIO_OUTPUT,    .state = ACTIVE_HIGH, .pull = GPIO_PULL_OFF, .drive = NORMAL_DRIVE },
    {.pin = PMIC_RDY,        .type = GPIO_OUTPUT,    .state = ACTIVE_HIGH, .pull = GPIO_PULL_OFF, .drive = NORMAL_DRIVE },
    {.pin = MCU_PERST_OUT_N, .type = GPIO_OUTPUT,    .state = ACTIVE_HIGH, .pull = GPIO_PULL_OFF, .drive = NORMAL_DRIVE },
    {.pin = PMIC_INT_N,      .type = GPIO_OUTPUT,    .state = ACTIVE_HIGH, .pull = GPIO_PULL_OFF, .drive = NORMAL_DRIVE },
    {.pin = TPS_RESET_N,     .type = GPIO_OUTPUT,    .state = ACTIVE_HIGH, .pull = GPIO_PULL_OFF, .drive = NORMAL_DRIVE },
    {.pin = NOC_ENA,         .type = GPIO_OUTPUT,    .state = ACTIVE_HIGH, .pull = GPIO_PULL_OFF, .drive = NORMAL_DRIVE },
    {.pin = MNN_ENA,         .type = GPIO_OUTPUT,    .state = ACTIVE_HIGH, .pull = GPIO_PULL_OFF, .drive = NORMAL_DRIVE },
    {.pin = MXN_EN,          .type = GPIO_OUTPUT,    .state = ACTIVE_HIGH, .pull = GPIO_PULL_OFF, .drive = NORMAL_DRIVE },
    {.pin = P_QLP_EN,        .type = GPIO_OUTPUT,    .state = ACTIVE_HIGH, .pull = GPIO_PULL_OFF, .drive = NORMAL_DRIVE },
    {.pin = P_SRAM_EN,       .type = GPIO_OUTPUT,    .state = ACTIVE_HIGH, .pull = GPIO_PULL_OFF, .drive = NORMAL_DRIVE },
    {.pin = P_LOGIC_EN,      .type = GPIO_OUTPUT,    .state = ACTIVE_HIGH, .pull = GPIO_PULL_OFF, .drive = NORMAL_DRIVE },
    {.pin = P_DDR_EN,        .type = GPIO_OUTPUT,    .state = ACTIVE_HIGH, .pull = GPIO_PULL_OFF, .drive = NORMAL_DRIVE },
    {.pin = P_Q_EN,          .type = GPIO_OUTPUT,    .state = ACTIVE_HIGH, .pull = GPIO_PULL_OFF, .drive = NORMAL_DRIVE },
    {.pin = P_PCIE_EN,       .type = GPIO_OUTPUT,    .state = ACTIVE_HIGH, .pull = GPIO_PULL_OFF, .drive = NORMAL_DRIVE },
    {.pin = P1_8V_EN,        .type = GPIO_OUTPUT,    .state = ACTIVE_HIGH, .pull = GPIO_PULL_OFF, .drive = NORMAL_DRIVE },
    {.pin = AP1_8V_EN,       .type = GPIO_OUTPUT,    .state = ACTIVE_HIGH, .pull = GPIO_PULL_OFF, .drive = NORMAL_DRIVE },
    {.pin = P3_3V_EN,        .type = GPIO_OUTPUT,    .state = ACTIVE_HIGH, .pull = GPIO_PULL_OFF, .drive = NORMAL_DRIVE },
    {.pin = MCU_GPIO0,       .type = GPIO_OUTPUT,    .state = ACTIVE_HIGH, .pull = GPIO_PULL_OFF, .drive = NORMAL_DRIVE },
    {.pin = MCU_GPIO1,       .type = GPIO_OUTPUT,    .state = ACTIVE_HIGH, .pull = GPIO_PULL_OFF, .drive = NORMAL_DRIVE },
    {.pin = MCU_GPIO2,       .type = GPIO_OD_OUTPUT, .state = ACTIVE_LOW,  .pull = GPIO_PULL_OFF, .drive = HIGH_DRIVE   },
    {.pin = MCU_GPIO4,       .type = GPIO_OUTPUT,    .state = ACTIVE_HIGH, .pull = GPIO_PULL_OFF, .drive = NORMAL_DRIVE },
    {.pin = MCU_GPIO_PAD0,   .type = GPIO_OUTPUT,    .state = ACTIVE_HIGH, .pull = GPIO_PULL_OFF, .drive = NORMAL_DRIVE },
    {.pin = MCU_GPIO_PAD1,   .type = GPIO_OUTPUT,    .state = ACTIVE_HIGH, .pull = GPIO_PULL_OFF, .drive = NORMAL_DRIVE },
    {.pin = MCU_GPIO_PAD2,   .type = GPIO_OUTPUT,    .state = ACTIVE_HIGH, .pull = GPIO_PULL_OFF, .drive = NORMAL_DRIVE },
    {.pin = MCU_GPIO_PAD3,   .type = GPIO_OUTPUT,    .state = ACTIVE_HIGH, .pull = GPIO_PULL_OFF, .drive = NORMAL_DRIVE },
    {.pin = PERST_IN_N,      .type = EIC_INPUT,                            .pull = GPIO_PULL_OFF, .sense = EIC_CONFIG_SENSE_BOTH,  .eicHandler = NULL },
    {.pin = WDT_RESET_N,     .type = EIC_INPUT,                            .pull = GPIO_PULL_OFF, .sense = EIC_CONFIG_SENSE_FALL,  .eicHandler = NULL },
    {.pin = TEMP_ALERT_N,    .type = EIC_INPUT,                            .pull = GPIO_PULL_OFF, .sense = EIC_CONFIG_SENSE_FALL,  .eicHandler = NULL },
    {.pin = TPS_ALERT,       .type = EIC_INPUT,                            .pull = GPIO_PULL_OFF, .sense = EIC_CONFIG_SENSE_FALL,  .eicHandler = NULL },
    {.pin = PG_12V,          .type = EIC_INPUT,                            .pull = GPIO_PULL_OFF, .sense = EIC_CONFIG_SENSE_FALL,  .eicHandler = NULL },
    {.pin = PG_3P3V,         .type = EIC_INPUT,                            .pull = GPIO_PULL_OFF, .sense = EIC_CONFIG_SENSE_FALL,  .eicHandler = NULL },
    {.pin = PG_BUS0,         .type = EIC_INPUT,                            .pull = GPIO_PULL_OFF, .sense = EIC_CONFIG_SENSE_FALL,  .eicHandler = NULL },
    {.pin = PG_BUS1,         .type = EIC_INPUT,                            .pull = GPIO_PULL_OFF, .sense = EIC_CONFIG_SENSE_FALL,  .eicHandler = NULL },
    {.pin = SRAM_ALERT,      .type = EIC_INPUT,                            .pull = GPIO_PULL_OFF, .sense = EIC_CONFIG_SENSE_FALL,  .eicHandler = NULL },
    {.pin = SRAM_FAULT,      .type = GPIO_INPUT,     .state = ACTIVE_HIGH, .pull = GPIO_PULL_OFF},
    {.pin = VMON_12V,        .type = ANALOG_INPUT,                                                                                 .adcHandler = NULL },
    {.pin = IMON_12V,        .type = ANALOG_INPUT,                                                                                 .adcHandler = NULL },
};
//  clang-format on

/***********************************************************************
 * GLOBAL Functions:   Defininition of global functions
***********************************************************************/
const gpioConfig_t * getGpioConfig_pcie_v3(void)
{
    return gpioConfig_pcie_v3;
}

const gpioPinIdMap_t * getGpioPinIdMap_pcie_v3(void)
{
    return gpioPinIdMap_pcie_v3;
}

const size_t getGpioConfigSize_pcie_v3(void)
{
    return (ARRAY_SIZE(gpioConfig_pcie_v3));
}

const size_t getGpioPinIdMapSize_pcie_v3(void)
{
    return (ARRAY_SIZE(gpioPinIdMap_pcie_v3));
}

void setEicHandler_pcie_v3(uint8_t pin, EIC_handler_callback handler)
{
    for(unsigned int i = 0; i < ARRAY_SIZE(gpioConfig_pcie_v3); i++)
    {
        if(gpioConfig_pcie_v3[i].pin == pin)
        {
            gpioConfig_pcie_v3[i].eicHandler = handler;
            return;
        }
    }
}

void setAdcHandler_pcie_v3(uint8_t pin, ADC_handler_callback handler)
{
    for(unsigned int i = 0; i < ARRAY_SIZE(gpioConfig_pcie_v3); i++)
    {
        if(gpioConfig_pcie_v3[i].pin == pin)
        {
            gpioConfig_pcie_v3[i].adcHandler = handler;
            return;
        }
    }
}
