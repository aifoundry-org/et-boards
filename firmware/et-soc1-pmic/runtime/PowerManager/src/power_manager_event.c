/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file power_manager_event.c
    \brief 
*/
/***********************************************************************/
#include <stdint.h>
#include <stdbool.h>

#include "power_manager_event.h"
#include "IoTask.h"
#include "FreeRTOS.h"
#include "iostructs.h"
#include "globals.h"
#include "gpio.h"
#include "adc.h"
#include "utils.h"
#include "etsoc_cmd_handler_task.h"
#include "error_codes.h"

/***********************************************************************
 * Macros:        Defininition of local macros
***********************************************************************/
#define AVERG_TIME (1000000UL) // 1 second in units of uSec

// If over power or over volage event occured, sending the next event is suspended for several ADC measurements
#define OVER_POWER_EVENT_SUSPENSION_PERIOD   100
#define OVER_VOLTAGE_EVENT_SUSPENSION_PERIOD 100

/***********************************************************************
 * Data types:      Defininition of local data types
***********************************************************************/
typedef struct {
    gpioId_t pinId;
    powerManagerEventType_t eventType;
} pinToEventTypeTable_t;

/***********************************************************************
 * Variables:  Defininition of local variables and constants
***********************************************************************/
// clang-format off
pinToEventTypeTable_t const pinToEventTypeTable[] = {
    {.pinId = PERST_IN,            .eventType = POWER_GENERAL_PERST_IN },
    {.pinId = WDT_RST_IN,          .eventType = POWER_GENERAL_WDT },
    {.pinId = VMON_12V_AIN,        .eventType = POWER_MONITORING_VOLTAGE },
    {.pinId = IMON_12V_AIN,        .eventType = POWER_MONITORING_POWER },
    {.pinId = TEMP_ALERT_IN,       .eventType = POWER_ALERT_TEMPERATURE },
    {.pinId = MNN_NOC_ALERT_IN,    .eventType = POWER_ALERT_MNN_NOC },
    {.pinId = SRAM_ALERT_IN,       .eventType = POWER_ALERT_SRAM },
    {.pinId = SRAM_FAULT_IN,       .eventType = POWER_ALERT_SRAM_FAULT },
    {.pinId = SRAM_EN_SENSE_IN,    .eventType = POWER_ALERT_SRAM_EN_SNS },
    {.pinId = POWER_GOOD_12V_IN,   .eventType = POWER_FAILURE_PG_12V },
    {.pinId = POWER_GOOD_3P3V_IN,  .eventType = POWER_FAILURE_PG_3P3V },
    {.pinId = POWER_GOOD_BUS0_IN,  .eventType = POWER_FAILURE_PG_BUS0 },
    {.pinId = POWER_GOOD_BUS1_IN,  .eventType = POWER_FAILURE_PG_BUS0 },
    {.pinId = POWER_GOOD_MNN_IN,   .eventType = POWER_FAILURE_PG_MNN },
    {.pinId = POWER_GOOD_NOC_IN,   .eventType = POWER_FAILURE_PG_NOC },
    {.pinId = POWER_GOOD_DDR_IN,   .eventType = POWER_FAILURE_PG_DDR },
    {.pinId = POWER_GOOD_MXN_IN,   .eventType = POWER_FAILURE_PG_MXN },
    {.pinId = POWER_GOOD_QLP_IN,   .eventType = POWER_FAILURE_PG_QLP },
    {.pinId = POWER_GOOD_Q_IN,     .eventType = POWER_FAILURE_PG_Q },
    {.pinId = POWER_GOOD_LOGIC_IN, .eventType = POWER_FAILURE_PG_LOGIC },
    {.pinId = POWER_GOOD_PCIE_IN,  .eventType = POWER_FAILURE_PG_PCIE },
    {.pinId = POWER_GOOD_1P8_IN,   .eventType = POWER_FAILURE_PG_1P8 },
    {.pinId = POWER_GOOD_SRAM_IN,  .eventType = POWER_FAILURE_PG_SRAM },    
    {.pinId = IOXP_0_IN,           .eventType = POWER_FAILURE_IOXPANDER_0 },
    {.pinId = IOXP_1_IN,           .eventType = POWER_FAILURE_IOXPANDER_1 },
};
// clang-format on

/***********************************************************************
 * GLOBAL Functions:   Defininition of global functions
***********************************************************************/
void perstInInterruptCallback(void) // interrupt context
{
    printfFromInterruptNoFlush("perstInInterruptCallback: pin --> %d", gpio_get_in_pin_id(PERST_IN), 0);
    bool data = gpio_get_in_pin_id(PERST_IN);
    powerManagerSendEvent(POWER_GENERAL_PERST_IN, (uint32_t)data);
}

void wdtResetInterruptCallback(void)
{
    if (globals.commonData.powerState.hwUp)
    {
        powerManagerSendEvent(POWER_GENERAL_WDT, 0); //for failure or alert event type data field is not used
    }
}

void alertTempInterruptCallback(void)
{
    powerManagerSendEvent(POWER_ALERT_TEMPERATURE, 0); //for failure or alert event type data field is not used
}

void alertMNNorNOCInterruptCallback(void)
{
    powerManagerSendEvent(POWER_ALERT_MNN_NOC, 0); //for failure or alert event type data field is not used
}

void alertSRAMInterruptCallback(void)
{
    powerManagerSendEvent(POWER_ALERT_SRAM, 0); //for failure or alert event type data field is not used
}

void powerGood12VInterruptCallback(void)
{
    if (globals.blockPTFail)
    {
        return;
    }
    powerManagerSendEvent(POWER_FAILURE_PG_12V, 0); //for failure or alert event type data field is not used
}

void powerGood3P3VInterruptCallback(void)
{
    if (globals.blockPTFail)
    {
        return;
    }
    powerManagerSendEvent(POWER_FAILURE_PG_3P3V, 0); //for failure or alert event type data field is not used
}

void powerGoodBus0interruptCallback(void)
{
    if (globals.blockPTFail)
    {
        return;
    }
    powerManagerSendEvent(POWER_FAILURE_PG_BUS0, 0); //for failure or alert event type data field is not used
}

void powerGoodBus1InterruptCallback(void)
{
    if (globals.blockPTFail)
    {
        return;
    }
    powerManagerSendEvent(POWER_FAILURE_PG_BUS0, 0); //for failure or alert event type data field is not used
}

void ioxpander0InterruptCallback(void)
{
    powerManagerSendEvent(POWER_FAILURE_IOXPANDER_0, 0); //for failure or alert event type data field is not used
}

void ioxpander1InterruptCallback(void)
{
    powerManagerSendEvent(POWER_FAILURE_IOXPANDER_1, 0); //for failure or alert event type data field is not used
}

// The ADC interrupts come every 6 to 10 ms

// New constants give a voltage 100.2% of previous constants
void adcVMON12VInterruptCallback(uint16_t result) // ADC interrupt handler context
{
    static uint32_t overVoltageSuccessiveEventCounter = 0;
    analogData_t *pad = &globals.commonData.analogData;
    pad->vmon12vMV = convert_ADC_to_voltage(result);
    pad->initV = true;
    uint16_t data =
        (uint8_t)((pad->vmon12vMV + MV_PER_50MV / 2) / MV_PER_50MV); // variable used by SOC I2C handler is 8 bits
    if (setRegisterValue(VOLTAGE_IN, data) != STATUS_SUCCESS)
    {
        printfFromInterruptNoFlush("adcVMON12VInterruptCallback: unable to set register value ", 0, 0);
    }

    if (isInterruptEnabled(INTR_cmd_pwr_fail))
    {
        if (pad->vmon12vMV < LOW_VOLTAGE_ALARM_SET_POINT_mV)
        {
            // Suspend consecutive over voltege events, don't send interrupts to SP on every ADC measurement to allow enought time for a proper reaction
            if (overVoltageSuccessiveEventCounter == 0)
            {
                powerManagerSendEvent(POWER_MONITORING_VOLTAGE, pad->vmon12vMV);
            }
            // reset counter if suspension period passed
            if (overVoltageSuccessiveEventCounter < OVER_VOLTAGE_EVENT_SUSPENSION_PERIOD)
            {
                overVoltageSuccessiveEventCounter++;
            }
            else
            {
                overVoltageSuccessiveEventCounter = 0;
            }
        }
        else
        {
            // reset counter if over voltage event didn't occur
            overVoltageSuccessiveEventCounter = 0;
        }
    }
}

void adcIMON12VInterruptCallback(uint16_t result) // ADC interrupt handler context
{
    static uint32_t overPowerSuccessiveEventCounter = 0;
    uint16_t data;
    analogData_t *pad = &globals.commonData.analogData;
    volatile uint32_t numerator = (result << SHIFT_FOR_INCREASED_PRECISION) * ADC_FULL_SCALE_MV;
    uint32_t denominator = (uint32_t)(IMON_DENOMINATOR * (1 << SHIFT_FOR_INCREASED_PRECISION));
    pad->imon12vMA = (numerator + denominator / 2) / denominator;

    // Set current power and check for power alarm
    pad->instmW = (pad->imon12vMA * pad->vmon12vMV + MW_PER_W / 2) / MW_PER_W;
    data = (uint16_t)(pad->instmW / 10UL); // variable used by SOC I2C handler is 16 bits
    if (setRegisterValue(POWER_IN, data) != STATUS_SUCCESS)
    {
        printfFromInterruptNoFlush("adcIMON12VInterruptCallback: unable to set register value, reg: POWER_IN ", 0, 0);
    }

    // Set average power
    uint32_t t = (uint32_t)getTimer2();
    int32_t dt = (t - pad->oldT) / MHZ; // uSec
    pad->oldT = t;
    int64_t dd = (dt * (INT64_SHL32(pad->instmW) - pad->avemW)) / AVERG_TIME;
    pad->avemW = pad->initV ? pad->avemW + dd : INT64_SHL32(pad->instmW);
    pad->initV = false;
    data = (uint16_t)((uint32_t)(INT64_SHR32(pad->avemW) + MV_PER_CENTIVOLT / 2) /
                      MV_PER_CENTIVOLT); // variable used by SOC I2C handler is 16 bits

    if (setRegisterValue(AVERAGE_POWER, data) != STATUS_SUCCESS)
    {
        printfFromInterruptNoFlush(
            "adcIMON12VInterruptCallback: unable to set register value, reg: AVERAGE_POWER ", 0, 0);
    }

    // Check the over power event
    if (isInterruptEnabled(INTR_cmd_ov_pwr) && globals.commonData.powerState.hwUp)
    {
        uint32_t powerAlarmSetPoint_mW = 0xFFFF;
        int status = getPowerAlarmSetPoint(&powerAlarmSetPoint_mW);
        if (status != STATUS_SUCCESS)
        {
            printfFromInterruptNoFlush("adcIMON12VInterruptCallback: unable to get power alarm set point ", 0, 0);
        }
        if ((data * MV_PER_CENTIVOLT) > powerAlarmSetPoint_mW)
        {
            // Suspend consecutive over power events, don't send interrupts to SP on every ADC measurement to allow enought time for a proper reaction
            if (overPowerSuccessiveEventCounter == 0)
            {
                powerManagerSendEvent(POWER_MONITORING_POWER, (data * MV_PER_CENTIVOLT));
            }

            // reset counter if suspension period passed
            if (overPowerSuccessiveEventCounter < OVER_POWER_EVENT_SUSPENSION_PERIOD)
            {
                overPowerSuccessiveEventCounter++;
            }
            else
            {
                overPowerSuccessiveEventCounter = 0;
            }
        }
        else
        {
            // reset counter if over power event didn't occur
            overPowerSuccessiveEventCounter = 0;
        }
    }
}

/** \brief Power management event handler.
 * 
 * Called fom EIC or ADC interrupts
 * when event is raised on the dedicated pin.
 * Relevant event information are sent to the queue
 * which is received and handled by the PowerManagement task.
 *
 * \param[in] eventType   Type of raised event
 * \param[in] data        Relevant data received with monitoring event (voltage, power, average power etc).
 */
void powerManagerSendEvent(powerManagerEventType_t eventType, uint32_t data)
{
    powerManagerEvent_t powerManagementEvent = { .eventType = eventType, .data = data };

    if (eventType == POWER_UNKNOWN_EVENT)
    {
        printfFromInterrupt("Unknown event type %d", eventType, 0);
        return;
    }
    if (uxQueueMessagesWaitingFromISR(globals.powerManagementTaskQueueHandle) == POWER_MANAGEMENT_TASK_QUEUE_LENGTH)
    {
        printfFromInterrupt("PowerManagement task queue full, event can't be handled", 0, 0);
        return;
    }

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendToBackFromISR(globals.powerManagementTaskQueueHandle, &powerManagementEvent, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

//todo: becomes local when IoTask is cleaned up
powerManagerEventType_t getEventTypeForPinId(gpioId_t pinId)
{
    powerManagerEventType_t eventType = POWER_UNKNOWN_EVENT;
    for (uint32_t i = 0; i < ARRAY_SIZE(pinToEventTypeTable); i++)
    {
        if (pinToEventTypeTable[i].pinId == pinId)
        {
            eventType = pinToEventTypeTable[i].eventType;
            break;
        }
    }
    return eventType;
}

//todo: to be removed when IoTask is cleaned up
gpioId_t getPinIdForEventType(powerManagerEventType_t eventType)
{
    gpioId_t pinId = 0xFF;
    for (uint32_t i = 0; i < ARRAY_SIZE(pinToEventTypeTable); i++)
    {
        if (pinToEventTypeTable[i].eventType == eventType)
        {
            pinId = pinToEventTypeTable[i].pinId;
            break;
        }
    }
    return pinId;
}
