/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file gpiopins_penguin_revb.c
    \brief GPIO pins definition for Penguin RevB board
*/
/***********************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include "gpio.h"
#include "gpiopins_penguin_revb.h"
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
    MCU_LED         = PIN_PB02,
    GPIO0_RST       = PIN_PB01,
    GPIO1_RST       = PIN_PB01,

/* TP indeterminate direction */
    MCU_GPIO0       = PIN_PA10,  // TP41
    MCU_GPIO1       = PIN_PA11,  // TP42
    MCU_GPIO2       = PIN_PA08,  // TP43
    MCU_GPIO3       = PIN_PA14,  // TP43
    MCU_GPIO4       = PIN_PA03,  // TP25
    MCU_GPIO_PAD2   = PIN_PA20,  // TP45
    MCU_GPIO_PAD3   = PIN_PA21,  // TP48

/* INPUTS */
    PERST_IN_N      = PIN_PA01,    /* CE_PERST_BUF_N */
    WDT_RESET_N     = PIN_PA15,    /* WDT_RSTOUT_N */
    TEMP_ALERT_N    = PIN_PA02,    /* TMP_ALERT_N */

    GPIO0_INT_N     = PIN_PA05,
    GPIO1_INT_N     = PIN_PB12,
    PG_12V          = PIN_PA09,
    PG_3P3V         = PIN_PB22,

/* ANALOG */
    IMON_12V        = PIN_PB08,
    VMON_12V        = PIN_PB09,

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
gpioPinIdMap_t const gpioPinIdMap_penguin_revb[] = {
    {.id = SOC_RST_OUT,            .pin = SOC_RESET_N     },
    {.id = PMIC_RDY_OUT,           .pin = PMIC_RDY        },
    {.id = SOC_PERST_OUT,          .pin = MCU_PERST_OUT_N },
    {.id = PMIC_INT_OUT,           .pin = PMIC_INT_N      },
    {.id = LED_OUT,                .pin = MCU_LED         },
    {.id = IOXP_0_RST_OUT,         .pin = GPIO0_RST       },
    {.id = IOXP_1_RST_OUT,         .pin = GPIO0_RST       },
    {.id = MCU_GPIO0_OUT,          .pin = MCU_GPIO0       },
    {.id = MCU_GPIO1_OUT,          .pin = MCU_GPIO1       },
    {.id = MCU_GPIO2_OUT,          .pin = MCU_GPIO2       },
    {.id = MCU_GPIO3_OUT,          .pin = MCU_GPIO3       },
    {.id = MCU_GPIO4_OUT,          .pin = MCU_GPIO4       },
    {.id = MCU_GPIO_PAD2_OUT,      .pin = MCU_GPIO_PAD2   },
    {.id = MCU_GPIO_PAD3_OUT,      .pin = MCU_GPIO_PAD3   },
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
    {.id = POWER_GOOD_12V_IN,      .pin = PG_12V          },
    {.id = POWER_GOOD_3P3V_IN,     .pin = PG_3P3V         },
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
gpioConfig_t gpioConfig_penguin_revb[] = {
    {.pin = MCU_LED,         .type = GPIO_OUTPUT,    .state = ACTIVE_HIGH, .pull = GPIO_PULL_OFF, .drive = NORMAL_DRIVE},
    {.pin = GPIO0_RST,       .type = GPIO_OUTPUT,    .state = ACTIVE_HIGH, .pull = GPIO_PULL_OFF, .drive = NORMAL_DRIVE},
    {.pin = GPIO1_RST,       .type = GPIO_OUTPUT,    .state = ACTIVE_HIGH, .pull = GPIO_PULL_OFF, .drive = NORMAL_DRIVE},
    {.pin = SOC_RESET_N,     .type = GPIO_OUTPUT,    .state = ACTIVE_HIGH, .pull = GPIO_PULL_OFF, .drive = NORMAL_DRIVE },
    {.pin = PMIC_RDY,        .type = GPIO_OUTPUT,    .state = ACTIVE_HIGH, .pull = GPIO_PULL_OFF, .drive = NORMAL_DRIVE },
    {.pin = MCU_PERST_OUT_N, .type = GPIO_OUTPUT,    .state = ACTIVE_HIGH, .pull = GPIO_PULL_OFF, .drive = NORMAL_DRIVE },
    {.pin = PMIC_INT_N,      .type = GPIO_OUTPUT,    .state = ACTIVE_HIGH, .pull = GPIO_PULL_OFF, .drive = NORMAL_DRIVE },
    {.pin = MCU_GPIO0,       .type = GPIO_OUTPUT,    .state = ACTIVE_HIGH, .pull = GPIO_PULL_OFF, .drive = NORMAL_DRIVE },
    {.pin = MCU_GPIO1,       .type = GPIO_OUTPUT,    .state = ACTIVE_HIGH, .pull = GPIO_PULL_OFF, .drive = NORMAL_DRIVE },
    {.pin = MCU_GPIO2,       .type = GPIO_OD_OUTPUT, .state = ACTIVE_LOW,  .pull = GPIO_PULL_OFF, .drive = HIGH_DRIVE   },
    {.pin = MCU_GPIO3,       .type = GPIO_OUTPUT,    .state = ACTIVE_HIGH, .pull = GPIO_PULL_OFF, .drive = NORMAL_DRIVE },
    {.pin = MCU_GPIO4,       .type = GPIO_OUTPUT,    .state = ACTIVE_HIGH, .pull = GPIO_PULL_OFF, .drive = NORMAL_DRIVE },
    {.pin = MCU_GPIO_PAD2,   .type = GPIO_OUTPUT,    .state = ACTIVE_HIGH, .pull = GPIO_PULL_OFF, .drive = NORMAL_DRIVE },
    {.pin = MCU_GPIO_PAD3,   .type = GPIO_OUTPUT,    .state = ACTIVE_HIGH, .pull = GPIO_PULL_OFF, .drive = NORMAL_DRIVE },

    {.pin = PERST_IN_N,      .type = EIC_INPUT,                            .pull = GPIO_PULL_OFF, .sense = EIC_CONFIG_SENSE_BOTH, .eicHandler = NULL },
    {.pin = WDT_RESET_N,     .type = EIC_INPUT,                            .pull = GPIO_PULL_OFF, .sense = EIC_CONFIG_SENSE_FALL, .eicHandler = NULL },
    {.pin = TEMP_ALERT_N,    .type = EIC_INPUT,                            .pull = GPIO_PULL_OFF, .sense = EIC_CONFIG_SENSE_FALL, .eicHandler = NULL },
    {.pin = GPIO0_INT_N,     .type = EIC_INPUT,                            .pull = GPIO_PULL_OFF, .sense = EIC_CONFIG_SENSE_FALL, .eicHandler = NULL },
    {.pin = GPIO1_INT_N,     .type = EIC_INPUT,                            .pull = GPIO_PULL_OFF, .sense = EIC_CONFIG_SENSE_FALL, .eicHandler = NULL },
    {.pin = PG_12V,          .type = EIC_INPUT,                            .pull = GPIO_PULL_OFF, .sense = EIC_CONFIG_SENSE_FALL, .eicHandler = NULL },
    {.pin = PG_3P3V,         .type = EIC_INPUT,                            .pull = GPIO_PULL_OFF, .sense = EIC_CONFIG_SENSE_FALL, .eicHandler = NULL },
    {.pin = VMON_12V,        .type = ANALOG_INPUT,                                                                                .adcHandler = NULL },
    {.pin = IMON_12V,        .type = ANALOG_INPUT,                                                                                .adcHandler = NULL },
};
//  clang-format on

/***********************************************************************
 * GLOBAL Functions:   Defininition of global functions
***********************************************************************/
const gpioConfig_t * getGpioConfig_penguin_revb(void)
{
    return gpioConfig_penguin_revb;
}

const gpioPinIdMap_t * getGpioPinIdMap_penguin_revb(void)
{
    return gpioPinIdMap_penguin_revb;
}

const size_t getGpioConfigSize_penguin_revb(void)
{
    return (ARRAY_SIZE(gpioConfig_penguin_revb));
}

const size_t getGpioPinIdMapSize_penguin_revb(void)
{
    return (ARRAY_SIZE(gpioPinIdMap_penguin_revb));
}

void setEicHandler_penguin_revb(uint8_t pin, EIC_handler_callback handler)
{
    for(unsigned int i = 0; i < ARRAY_SIZE(gpioConfig_penguin_revb); i++)
    {
        if(gpioConfig_penguin_revb[i].pin == pin)
        {
            gpioConfig_penguin_revb[i].eicHandler = handler;
            return;
        }
    }
}

void setAdcHandler_penguin_revb(uint8_t pin, ADC_handler_callback handler)
{
    for(unsigned int i = 0; i < ARRAY_SIZE(gpioConfig_penguin_revb); i++)
    {
        if(gpioConfig_penguin_revb[i].pin == pin)
        {
            gpioConfig_penguin_revb[i].adcHandler = handler;
            return;
        }
    }
}

uint8_t pgPinFromEnPinId_penguin_revb(gpioId_t enablePinId)
{
    uint8_t pin = gpio_get_pin_from_id(enablePinId);
    if (!((MNN_ENA <= pin && pin <= P_Q_EN) || (P_LOGIC_EN <= pin && pin <= P_SRAM_EN)))
    {
        return 0xFF;
    }
    return pin - EXPANDER_P1;
}
