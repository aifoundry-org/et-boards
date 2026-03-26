/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file power_manager_event.h
    \brief 
*/
/***********************************************************************/
#ifndef _POWER_MANAGER_EVENT_H_
#define _POWER_MANAGER_EVENT_H_

#include <stdint.h>
#include <stdbool.h>
#include "iostructs.h"
#include "globals.h"
#include "gpio.h"
#include "features.h"

/***********************************************************************
 * GLOBAL Macros:        Defininition of global macros
***********************************************************************/
#define POWER_GENERAL_EVENT           0x00
#define POWER_MONITORING_EVENT        0x10
#define POWER_ALERT_EVENT             0x20
#define POWER_FAILURE_EVENT           0x40
#define POWER_IOXPANDER_FAILURE_EVENT 0x80

/***********************************************************************
 * GLOBAL data types:  Defininition of global data types
***********************************************************************/
/** Power manager event type. Events are divided into categories 
 * (general events, monitoring events, alerts, failures, io expander events) which are handled differently. */

// clang-format off
typedef enum
{
    POWER_UNKNOWN_EVENT            = POWER_GENERAL_EVENT + 0,     /**< Unknown event. */
    
    POWER_GENERAL_PERST_IN         = POWER_GENERAL_EVENT + 1,     /**< EIC interrupt event. */
    POWER_GENERAL_WDT              = POWER_GENERAL_EVENT + 2,     /**< EIC interrupt event. */

    POWER_MONITORING_VOLTAGE       = POWER_MONITORING_EVENT + 0,  /**< ADC interrupt event. */
    POWER_MONITORING_POWER         = POWER_MONITORING_EVENT + 1,  /**< ADC interrupt event. */
    POWER_MONITORING_TEMPERATURE   = POWER_MONITORING_EVENT + 2,  /**< todo. */

    POWER_ALERT_TEMPERATURE        = POWER_ALERT_EVENT + 0,       /**< EIC interrupt event from temperature sensor MAX6660 */
    POWER_ALERT_MNN_NOC            = POWER_ALERT_EVENT + 1,       /**< EIC interrupt event from NOC/MNN regulator TPS53681. */
    POWER_ALERT_SRAM               = POWER_ALERT_EVENT + 2,       /**< EIC interrupt event from SRAM regulator LTM4680. */
    POWER_ALERT_SRAM_FAULT         = POWER_ALERT_EVENT + 3,       /**< EIC interrupt event from SRAM regulator LTM4680. Available on BUB and included in PG_BUS0/1 on PCIE3.*/
    POWER_ALERT_SRAM_EN_SNS        = POWER_ALERT_EVENT + 4,       /**< EIC interrupt event from SRAM regulator LTM4680. Available on BUB and included in PG_BUS0/1 on PCIE3. */

    POWER_FAILURE_PG_12V           = POWER_FAILURE_EVENT + 0,     /**< EIC interrupt event from regulator FS1406. Available only on PCIE3. */
    POWER_FAILURE_PG_3P3V          = POWER_FAILURE_EVENT + 1,     /**< EIC interrupt event from regulator FS1406. Available only on PCIE3. */
    POWER_FAILURE_PG_BUS0          = POWER_FAILURE_EVENT + 2,     /**< EIC interrupt event from TPS53681, FS1406, and SRAM regulators. Available on PCIE3, included power good signals are on IO expander on BUB. */
    POWER_FAILURE_PG_BUS1          = POWER_FAILURE_EVENT + 3,     /**< EIC interrupt event from TPS53681, FS1406, and SRAM regulators. Available on PCIE3, included power good signals are on IO expander on BUB. */
    POWER_FAILURE_PG_MNN           = POWER_FAILURE_EVENT + 4,     /**< EIC interrupt event from regulator TPS53681. Available on BUB, included in PG_BUS0/1 on PCIE3. */
    POWER_FAILURE_PG_NOC           = POWER_FAILURE_EVENT + 5,     /**< EIC interrupt event from regulator TPS53681. Available on BUB, included in PG_BUS0/1 on PCIE3. */
    POWER_FAILURE_PG_DDR           = POWER_FAILURE_EVENT + 6,     /**< EIC interrupt event from regulator FS1406. Available on BUB, included in PG_BUS0/1 on PCIE3. */
    POWER_FAILURE_PG_MXN           = POWER_FAILURE_EVENT + 7,     /**< EIC interrupt event from regulator FS1406. Available on BUB, included in PG_BUS0/1 on PCIE3. */
    POWER_FAILURE_PG_QLP           = POWER_FAILURE_EVENT + 8,     /**< EIC interrupt event from regulator FS1406. Available on BUB, included in PG_BUS0/1 on PCIE3. */
    POWER_FAILURE_PG_Q             = POWER_FAILURE_EVENT + 9,     /**< EIC interrupt event from regulator FS1406. Available on BUB, included in PG_BUS0/1 on PCIE3. */
    POWER_FAILURE_PG_LOGIC         = POWER_FAILURE_EVENT + 10,    /**< EIC interrupt event from regulator FS1406. Available on BUB, included in PG_BUS0/1 on PCIE3. */
    POWER_FAILURE_PG_PCIE          = POWER_FAILURE_EVENT + 11,    /**< EIC interrupt event from regulator FS1406. Available on BUB, included in PG_BUS0/1 on PCIE3. */
    POWER_FAILURE_PG_1P8           = POWER_FAILURE_EVENT + 12,    /**< EIC interrupt event from regulator FS1406. Available on BUB, included in PG_BUS0/1 on PCIE3. */
    POWER_FAILURE_PG_SRAM          = POWER_FAILURE_EVENT + 13,    /**< EIC interrupt event from SRAM regulator LTM4680. Available on BUB, included in PG_BUS0/1 on PCIE3. */

    POWER_FAILURE_IOXPANDER_0      = POWER_IOXPANDER_FAILURE_EVENT + 0,  /**< EIC interrupt event from IO expander. Available on BUB. */
    POWER_FAILURE_IOXPANDER_1      = POWER_IOXPANDER_FAILURE_EVENT + 1,  /**< EIC interrupt event from IO expander. Available on BUB. */

    //TODO: will be removed when Logging task is implemented
    FT_intrPrint                   = 0x100,
    FT_oflo                        = FT_intrPrint + 1,

} powerManagerEventType_t;
// clang-format on

/** Structure holding info about the power management related events */
typedef struct {
    powerManagerEventType_t eventType; /**< Event type to be handled.  */
    uint32_t data;                     /**< Data relevant to power management monitoring event. */
} powerManagerEvent_t;

/***********************************************************************
 * GLOBAL Functions:   Declaration of global functions
***********************************************************************/
void perstInInterruptCallback(void);
void wdtResetInterruptCallback(void);
void powerGood12VInterruptCallback(void);
void powerGood3P3VInterruptCallback(void);
void powerGoodBus0interruptCallback(void);
void powerGoodBus1InterruptCallback(void);
void alertTempInterruptCallback(void);
void alertMNNorNOCInterruptCallback(void);
void alertSRAMInterruptCallback(void);
void ioxpander0InterruptCallback(void);
void ioxpander1InterruptCallback(void);
void adcIMON12VInterruptCallback(uint16_t result);
void adcVMON12VInterruptCallback(uint16_t result);

void powerManagerSendEvent(powerManagerEventType_t eventType, uint32_t data);

powerManagerEventType_t getEventTypeForPinId(gpioId_t pinId);
uint8_t getPinIdForEventType(powerManagerEventType_t eventType);

#endif //_POWER_MANAGER_EVENT_H_