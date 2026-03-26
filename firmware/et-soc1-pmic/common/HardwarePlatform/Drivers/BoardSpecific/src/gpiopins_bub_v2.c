/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file gpiopins_bub_v2.c
    \brief GPIO pins definition for BUB v2 board
*/
/***********************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include "gpio.h"
#include "gpiopins_bub_v2.h"
#include "samd20.h"
#include "utils.h"

/***********************************************************************
 * GLOBAL Data types:      Defininition of global data types
***********************************************************************/
// clang-format off
enum {
/* OUTPUTS */
    SOC_RESET_N     = PIN_PB10,    /* MCU_RESET_OUT_N */
    PMIC_RDY        = PIN_PB05,
    MCU_PERST_OUT_N = PIN_PB23,    /* SOC_PERST0_N */
    PMIC_INT_N      = PIN_PB11,
    MCU_LED         = PIN_PB02,
    GPIO0_RST       = PIN_PB01,
    GPIO1_RST       = PIN_PB03,
    MEZ_ALERT       = PIN_PB04,

/* TP indeterminate direction */
    MCU_GPIO0       = PIN_PB06,  // TP113
    MCU_GPIO1       = PIN_PB09,  // TP109
    MCU_GPIO2       = PIN_PA10,  // TP108
    MCU_GPIO3       = PIN_PA11,  // TP120
    MCU_GPIO4       = PIN_PB07,  // TP121
    MCU_GPIO5       = PIN_PA00,  // TP106
    MCU_GPIO7       = PIN_PA14,  // TP112
    MCU_GPIO8       = PIN_PA25,  // TP107
    MCU_GPIO9       = PIN_PA24,  // TP122
    MCU_GPIO10      = PIN_PA02,  // TP118
    MCU_GPIO_PAD0   = PIN_PB16,  // TP114
    MCU_GPIO_PAD1   = PIN_PB17,  // TP115
    MCU_GPIO_PAD2   = PIN_PA20,  // TP119
    MCU_GPIO_PAD3   = PIN_PA21,  // TP116
    MEZ_GPIO0       = PIN_PA27,
    MEZ_GPIO1       = PIN_PB08,
    MEZ_GPIO2       = PIN_PB00,
    MEZ_GPIO3       = PIN_PB31,
    MEZ_GPIO4       = PIN_PA09,
    MEZ_GPIO5       = PIN_PB30,

/* INPUTS */
    PERST_IN_N      = PIN_PA01,    /* CE_PERST_BUF_N */
    WDT_RESET_N     = PIN_PA15,    /* WDT_RSTOUT_N */
    TEMP_ALERT_N    = PIN_PA18,    /* TMP_ALERT_N */
    GPIO0_INT_N     = PIN_PA05,
    GPIO1_INT_N     = PIN_PA19,

/* MUST NOT BE USED */
// PA08 cannot be used for EIC
// PA08 is now wired in parallel with PA19
// (PA19 is now used as GPIO1_INT_N)
// PA08 must not be set as an output

/* ANALOG */
    IMON_12V        = PIN_PA03,
    VMON_12V        = PIN_PA04,

/* INPUTS */
/* GPIO EXPANDER 0 */
    PG_MNN          = GPIO0_P0_0, // MNN_PWR_GOOD
    PG_NOC          = GPIO0_P0_1, // NOC_PWR_GOOD
    PG_DDR          = GPIO0_P0_2, // VDD_DDR_PWR_GOOD
    PG_MXN          = GPIO0_P0_3, // VDD_MXN_PWR_GOOD
    PG_QLP          = GPIO0_P0_4, // VDD_QLP_PWR_GOOD
    PG_Q            = GPIO0_P0_5, // VDD_Q_PWR_GOOD
    TPS_ALERT       = GPIO0_P0_6,
/* GPIO EXPANDER 1 */
    PG_LOGIC        = GPIO1_P0_0, // VDD_LOGIC_PWR_GOOD
    PG_PCIE         = GPIO1_P0_1, // VDD_PCIE_IO_PWR_GOOD
    PG_1P8          = GPIO1_P0_2, // VDD_1P8_PWR_GOOD
    PG_SRAM         = GPIO1_P0_3, // VDD_SRAM_PWR_GOOD
    SRAM_ALERT      = GPIO1_P0_4,
    SRAM_FAULT      = GPIO1_P0_5,
    SRAM_EN_SNS     = GPIO1_P0_6, // SRAM_EN_SENSE

/* OUTPUTS */
/* GPIO EXPANDER 0 */
    MNN_ENA         = GPIO0_P1_0, // MNN_ENA
    NOC_ENA         = GPIO0_P1_1, // NOC_ENA
    P_DDR_EN        = GPIO0_P1_2, // VDD_DDR_EN
    MXN_EN          = GPIO0_P1_3, // MXN_EN
    P_QLP_EN        = GPIO0_P1_4, // VDD_QLP_EN
    P_Q_EN          = GPIO0_P1_5, // VDD_Q_EN
    TPS_RESET_N     = GPIO0_P1_6, //
/* GPIO EXPANDER 1 */
    P_LOGIC_EN      = GPIO1_P1_0, // VDD_LOGIC_EN
    P_PCIE_EN       = GPIO1_P1_1, // VDD_1P5_EN
    P1_8V_EN        = GPIO1_P1_2, // VDD_1P8_EN
    P_SRAM_EN       = GPIO1_P1_3, // SRAM_EN_DRIVE
    P3_3V_EN        = GPIO1_P1_4, // P3_3V_EN
    AP1_8V_EN       = GPIO1_P1_5, // VDDA_1P8_EN
};
// clang-format on

/***********************************************************************
 * GLOBAL variables:  Defininition of global variables and constants
***********************************************************************/
// clang-format off
gpioPinIdMap_t const gpioPinIdMap_bub_v2[] = {
    {.id = SOC_RST_OUT,            .pin = SOC_RESET_N     },
    {.id = PMIC_RDY_OUT,           .pin = PMIC_RDY        },
    {.id = SOC_PERST_OUT,          .pin = MCU_PERST_OUT_N },
    {.id = PMIC_INT_OUT,           .pin = PMIC_INT_N      },
    {.id = LED_OUT,                .pin = MCU_LED         }, 
    {.id = IOXP_0_RST_OUT,         .pin = GPIO0_RST       },
    {.id = IOXP_1_RST_OUT,         .pin = GPIO1_RST       },
    {.id = MEZ_CONN_ALERT_OUT,     .pin = MEZ_ALERT       },
    {.id = MCU_GPIO0_OUT,          .pin = MCU_GPIO0       },
    {.id = MCU_GPIO1_OUT,          .pin = MCU_GPIO1       },
    {.id = MCU_GPIO2_OUT,          .pin = MCU_GPIO2       },
    {.id = MCU_GPIO3_OUT,          .pin = MCU_GPIO3       },
    {.id = MCU_GPIO4_OUT,          .pin = MCU_GPIO4       },
    {.id = MCU_GPIO5_OUT,          .pin = MCU_GPIO5       },
    {.id = MCU_GPIO7_OUT,          .pin = MCU_GPIO7       },
    {.id = MCU_GPIO8_OUT,          .pin = MCU_GPIO8       },
    {.id = MCU_GPIO9_OUT,          .pin = MCU_GPIO9       },
    {.id = MCU_GPIO10_OUT,         .pin = MCU_GPIO10      },
    {.id = MCU_GPIO_PAD0_OUT,      .pin = MCU_GPIO_PAD0   },
    {.id = MCU_GPIO_PAD1_OUT,      .pin = MCU_GPIO_PAD1   },
    {.id = MCU_GPIO_PAD2_OUT,      .pin = MCU_GPIO_PAD2   },
    {.id = MCU_GPIO_PAD3_OUT,      .pin = MCU_GPIO_PAD3   },
    {.id = MEZ_GPIO0_OUT,          .pin = MEZ_GPIO0       },
    {.id = MEZ_GPIO1_OUT,          .pin = MEZ_GPIO1       },
    {.id = MEZ_GPIO2_OUT,          .pin = MEZ_GPIO2       },
    {.id = MEZ_GPIO3_OUT,          .pin = MEZ_GPIO3       },
    {.id = MEZ_GPIO4_OUT,          .pin = MEZ_GPIO4       },
    {.id = MEZ_GPIO5_OUT,          .pin = MEZ_GPIO5       },
    {.id = PERST_IN,               .pin = PERST_IN_N      },
    {.id = WDT_RST_IN,             .pin = WDT_RESET_N     },
    {.id = TEMP_ALERT_IN,          .pin = TEMP_ALERT_N    },
    {.id = IOXP_0_IN,              .pin = GPIO0_INT_N     },
    {.id = IOXP_1_IN,              .pin = GPIO1_INT_N     },
    {.id = VMON_12V_AIN,           .pin = VMON_12V        },
    {.id = IMON_12V_AIN,           .pin = IMON_12V        },
    {.id = POWER_GOOD_MNN_IN,      .pin = PG_MNN          },  
    {.id = POWER_GOOD_NOC_IN,      .pin = PG_NOC          },  
    {.id = POWER_GOOD_DDR_IN,      .pin = PG_DDR          },      
    {.id = POWER_GOOD_MXN_IN,      .pin = PG_MXN          },      
    {.id = POWER_GOOD_QLP_IN,      .pin = PG_QLP          },      
    {.id = POWER_GOOD_Q_IN,        .pin = PG_Q            },      
    {.id = MNN_NOC_ALERT_IN,       .pin = TPS_ALERT       },
    {.id = POWER_GOOD_LOGIC_IN,    .pin = PG_LOGIC        },      
    {.id = POWER_GOOD_PCIE_IN,     .pin = PG_PCIE         },         
    {.id = POWER_GOOD_1P8_IN,      .pin = PG_1P8          },      
    {.id = POWER_GOOD_SRAM_IN,     .pin = PG_SRAM         },      
    {.id = SRAM_ALERT_IN,          .pin = SRAM_ALERT      },
    {.id = SRAM_FAULT_IN,          .pin = SRAM_FAULT      },      
    {.id = SRAM_EN_SENSE_IN,       .pin = SRAM_EN_SNS     },    
    {.id = MNN_EN_OUT,             .pin = MNN_ENA         },
    {.id = NOC_EN_OUT,             .pin = NOC_ENA         },
    {.id = DDR_EN_OUT,             .pin = P_DDR_EN        },
    {.id = MXN_EN_OUT,             .pin = MXN_EN,         },
    {.id = QLP_EN_OUT,             .pin = P_QLP_EN        },
    {.id = Q_EN_OUT,               .pin = P_Q_EN          },
    {.id = MNN_NOC_PWR_RST_OUT,    .pin = TPS_RESET_N     },
    {.id = LOGIC_EN_OUT,           .pin = P_LOGIC_EN      },
    {.id = PCIE_EN_OUT,            .pin = P_PCIE_EN       },
    {.id = VDD_1P8V_EN_OUT,        .pin = P1_8V_EN        },
    {.id = SRAM_EN_OUT,            .pin = P_SRAM_EN       },
    {.id = VDD_3P3V_EN_OUT,        .pin = P3_3V_EN        },
    {.id = VDDA_1P8V_EN_OUT,       .pin = AP1_8V_EN       },
};
//  clang-format on

// clang-format off
gpioConfig_t gpioConfig_bub_v2[] = {
/* TP indeterminate direction */
    { .pin = MCU_GPIO0,       .type = GPIO_OUTPUT, .state = ACTIVE_HIGH, .pull = GPIO_PULL_OFF, .drive = NORMAL_DRIVE },
    { .pin = MCU_GPIO1,       .type = GPIO_OUTPUT, .state = ACTIVE_HIGH, .pull = GPIO_PULL_OFF, .drive = NORMAL_DRIVE },
    { .pin = MCU_GPIO2,       .type = GPIO_OUTPUT, .state = ACTIVE_HIGH, .pull = GPIO_PULL_OFF, .drive = NORMAL_DRIVE },
    { .pin = MCU_GPIO3,       .type = GPIO_OUTPUT, .state = ACTIVE_HIGH, .pull = GPIO_PULL_OFF, .drive = NORMAL_DRIVE },
    { .pin = MCU_GPIO4,       .type = GPIO_OUTPUT, .state = ACTIVE_HIGH, .pull = GPIO_PULL_OFF, .drive = NORMAL_DRIVE },
    { .pin = MCU_GPIO5,       .type = GPIO_OUTPUT, .state = ACTIVE_HIGH, .pull = GPIO_PULL_OFF, .drive = NORMAL_DRIVE },
// MCU_GPIO6 (PA19) wired to GPIO1_INT_N, therefore it cannot be used as an MCU_GPIO
    { .pin = MCU_GPIO7,       .type = GPIO_OUTPUT, .state = ACTIVE_HIGH, .pull = GPIO_PULL_OFF, .drive = NORMAL_DRIVE },
    { .pin = MCU_GPIO8,       .type = GPIO_OUTPUT, .state = ACTIVE_HIGH, .pull = GPIO_PULL_OFF, .drive = NORMAL_DRIVE },
    { .pin = MCU_GPIO9,       .type = GPIO_OUTPUT, .state = ACTIVE_HIGH, .pull = GPIO_PULL_OFF, .drive = NORMAL_DRIVE },
    { .pin = MCU_GPIO10,      .type = GPIO_OUTPUT, .state = ACTIVE_HIGH, .pull = GPIO_PULL_OFF, .drive = NORMAL_DRIVE },
    { .pin = MEZ_GPIO1,       .type = GPIO_INPUT,                        .pull = GPIO_PULL_OFF},
    { .pin = MEZ_GPIO2,       .type = GPIO_INPUT,                        .pull = GPIO_PULL_OFF},
    { .pin = MEZ_GPIO3,       .type = GPIO_INPUT,                        .pull = GPIO_PULL_OFF},
    { .pin = MEZ_GPIO4,       .type = GPIO_INPUT,                        .pull = GPIO_PULL_OFF},
    { .pin = MEZ_GPIO5,       .type = GPIO_INPUT,                        .pull = GPIO_PULL_OFF},

    /* OUTPUTS */
    { .pin = SOC_RESET_N,     .type = GPIO_OUTPUT, .state = ACTIVE_HIGH, .pull = GPIO_PULL_OFF, .drive = NORMAL_DRIVE },
    { .pin = PMIC_RDY,        .type = GPIO_OUTPUT, .state = ACTIVE_HIGH, .pull = GPIO_PULL_OFF, .drive = NORMAL_DRIVE },
    { .pin = MCU_PERST_OUT_N, .type = GPIO_OUTPUT, .state = ACTIVE_HIGH, .pull = GPIO_PULL_OFF, .drive = NORMAL_DRIVE },
    { .pin = PMIC_INT_N,      .type = GPIO_OUTPUT, .state = ACTIVE_HIGH, .pull = GPIO_PULL_OFF, .drive = NORMAL_DRIVE },
    { .pin = MCU_LED,         .type = GPIO_OUTPUT, .state = ACTIVE_HIGH, .pull = GPIO_PULL_OFF, .drive = NORMAL_DRIVE },
    { .pin = GPIO0_RST,       .type = GPIO_OUTPUT, .state = ACTIVE_HIGH, .pull = GPIO_PULL_OFF, .drive = NORMAL_DRIVE },
    { .pin = GPIO1_RST,       .type = GPIO_OUTPUT, .state = ACTIVE_HIGH, .pull = GPIO_PULL_OFF, .drive = NORMAL_DRIVE },
    { .pin = MEZ_ALERT,       .type = GPIO_OUTPUT, .state = ACTIVE_HIGH, .pull = GPIO_PULL_OFF, .drive = NORMAL_DRIVE },

    /* INPUTS */
    { .pin = PERST_IN_N,      .type = EIC_INPUT,                         .pull = GPIO_PULL_OFF, .sense = EIC_CONFIG_SENSE_BOTH,  .eicHandler = NULL },
    { .pin = WDT_RESET_N,     .type = EIC_INPUT,                         .pull = GPIO_PULL_OFF, .sense = EIC_CONFIG_SENSE_FALL,  .eicHandler = NULL },
    { .pin = TEMP_ALERT_N,    .type = EIC_INPUT,                         .pull = GPIO_PULL_OFF, .sense = EIC_CONFIG_SENSE_FALL,  .eicHandler = NULL },
    { .pin = GPIO0_INT_N,     .type = EIC_INPUT,                         .pull = GPIO_PULL_OFF, .sense = EIC_CONFIG_SENSE_FALL,  .eicHandler = NULL },
    { .pin = GPIO1_INT_N,     .type = EIC_INPUT,                         .pull = GPIO_PULL_OFF, .sense = EIC_CONFIG_SENSE_FALL,  .eicHandler = NULL },
    { .pin = VMON_12V,        .type = ANALOG_INPUT,                                                                              .adcHandler = NULL },
    { .pin = IMON_12V,        .type = ANALOG_INPUT,                                                                              .adcHandler = NULL },
};
// clang-format on

/***********************************************************************
 * GLOBAL Functions:   Defininition of global functions
***********************************************************************/
const gpioConfig_t *getGpioConfig_bub_v2(void)
{
    return gpioConfig_bub_v2;
}

const gpioPinIdMap_t *getGpioPinIdMap_bub_v2(void)
{
    return gpioPinIdMap_bub_v2;
}

const size_t getGpioConfigSize_bub_v2(void)
{
    return (ARRAY_SIZE(gpioConfig_bub_v2));
}

const size_t getGpioPinIdMapSize_bub_v2(void)
{
    return (ARRAY_SIZE(gpioPinIdMap_bub_v2));
}

void setEicHandler_bub_v2(uint8_t pin, EIC_handler_callback handler)
{
    for (unsigned int i = 0; i < ARRAY_SIZE(gpioConfig_bub_v2); i++)
    {
        if (gpioConfig_bub_v2[i].pin == pin)
        {
            gpioConfig_bub_v2[i].eicHandler = handler;
            return;
        }
    }
}

void setAdcHandler_bub_v2(uint8_t pin, ADC_handler_callback handler)
{
    for (unsigned int i = 0; i < ARRAY_SIZE(gpioConfig_bub_v2); i++)
    {
        if (gpioConfig_bub_v2[i].pin == pin)
        {
            gpioConfig_bub_v2[i].adcHandler = handler;
            return;
        }
    }
}

uint8_t pgPinFromEnPinId_bub_v2(gpioId_t enablePinId)
{
    uint8_t pin = gpio_get_pin_from_id(enablePinId);
    if (!((MNN_ENA <= pin && pin <= P_Q_EN) || (P_LOGIC_EN <= pin && pin <= P_SRAM_EN)))
    {
        return 0xFF;
    }
    return pin - EXPANDER_P1;
}
