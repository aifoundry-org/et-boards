/***********************************************************************
 *
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *
 ************************************************************************/

/***********************************************************************/
/*! \file IoTask.c
    \brief C file for IO Task related functions
*/
/***********************************************************************/

#include <compiler.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "FreeRTOS.h"
#include "board_defs.h"
#include "hooks.h"

#include "CLITask.h"
#include "IoTask.h"
#include "boardchipinfo.h"
#include "chipio.h"
#include "chips.h"
#include "cli.h"
#include "commitinfo.h"
#include "common_defs.h"
#include "etsoc_cmd_handler_task.h"
#include "globals.h"
#include "gpio.h"
#include "hw_encoding.h"
#include "i2cm.h"
#include "image_metadata.h"
#include "ioxpander.h"
#include "power_manager_event.h"
#include "scripts.h"
#include "uart.h"
#include "utils.h"
#include "version.h"
#include "vreg.h"
#include "features.h"

/***********************************************************************
 * Macros:        Defininition of local macros
***********************************************************************/
#define MONITOR_FREQ  100          // Hz - maximum 1000
#define N_AVE         MONITOR_FREQ // per 1 sec
#define E_Dev_SRM     0
#define E_Dev_MNN_NOC 1
#define E_DevPage_0   0
#define E_DevPage_1   1

#define NUM_VOUT_ERROR_TYPES 7
#define NUM_VIN_ERROR_TYPES  6
#define NUM_CUR_ERROR_TYPES  5
#define NUM_TEMP_ERROR_TYPES 1
#define NUM_CML_ERROR_TYPES  8
#define NUM_MFR_ERROR_TYPES  8
#define MFR_ERROR_GROUP      2

// Function to read 8-bit value from I2C
int static i2c_read_8bit(uint8_t addr, uint8_t reg, uint8_t *val)
{
    i2cmErr_t rv;
    rv = i2cm_read8(addr, reg, val);
    if (rv != I2CM_OK)
    {
        interruptingPrintfOneLine("I2C read8 error %s", i2cm_get_error_message(rv));
        return I2C_MASTER_ERROR_REG_READ;
    }
    return rv;
}

// Function to read 16-bit value from I2C
int static i2c_read_16bit(uint8_t addr, uint8_t reg, uint16_t *val)
{
    i2cmErr_t rv;
    rv = i2cm_read16(addr, reg, val);
    if (rv != I2CM_OK)
    {
        interruptingPrintfOneLine("I2C read16 error %s", i2cm_get_error_message(rv));
        return I2C_MASTER_ERROR_REG_READ;
    }
    return rv;
}

int static i2c_read_blk(uint8_t addr, uint8_t reg, uint64_t *val, uint8_t *nbytes, uint8_t size)
{
    i2cmErr_t rv;
    rv = i2cm_read_blk_word(addr, reg, val, size);
    if (rv != I2CM_OK)
    {
        *nbytes = (uint8_t)(0xF0 | rv);
        interruptingPrintfOneLine("I2C readblk error %s", i2cm_get_error_message(rv));
        return I2C_MASTER_ERROR_REG_READ;
    }
    return rv;
}

// Function to write 8-bit data to I2C
int static i2c_write(uint8_t addr, uint8_t reg, uint8_t data)
{
    i2cmErr_t rv;
    rv = i2cm_write8(addr, reg, data);
    if (rv != I2CM_OK)
    {
        interruptingPrintfOneLine("I2C error %s", i2cm_get_error_message(rv));
    }
    return rv;
}

#define VV_VALUE(ireg, ichan) \
    ((ireg == 1) ? ((ichan == 0) ? 0x3000 : 0xA00) : ((ireg == 2) ? ((ichan == 0) ? 0x1400 : 0x800) : 0x100))

/***********************************************************************
 * Data types:      Defininition of local data types
 ***********************************************************************/
typedef struct {
    uint8_t eID;
    char const *name;
    char const *id;
} t_pmcDevs;

typedef enum {
    RF_none = 0,
    RF_fault = 1,
    RF_overtemp = 2 // some regulators can report either fault or overtemp
} regulatorFail_t;

typedef struct {
    powerManagerEventType_t eventType;
    regulatorFail_t (*eventHandler)(
        powerManagerEventType_t eventType, uint32_t data, char const *name, powerState_t *pps);
    char const *name;
} powerManagementEventInfo_t;

/***********************************************************************
 * Functions:   Declaration of local function used as func pointers
 ***********************************************************************/
static regulatorFail_t handlePerstEvent(
    powerManagerEventType_t eventType, uint32_t data, char const *name, powerState_t *pps);
static regulatorFail_t handleWatchdogTimerEvent(
    powerManagerEventType_t eventType, uint32_t data, char const *name, powerState_t *pps);

static regulatorFail_t handleVoltageMonitoringEvent(
    powerManagerEventType_t eventType, uint32_t data, char const *name, powerState_t *pps);
static regulatorFail_t handlePowerMonitoringEvent(
    powerManagerEventType_t eventType, uint32_t data, char const *name, powerState_t *pps);
static regulatorFail_t handleTemperatureMonitoringEvent(
    powerManagerEventType_t eventType, uint32_t data, char const *name, powerState_t *pps);

static regulatorFail_t handleTemperatureAlert(
    powerManagerEventType_t eventType, uint32_t data, char const *name, powerState_t *pps);
static regulatorFail_t handleMNNOrNOCRegulatorAlert(
    powerManagerEventType_t eventType, uint32_t data, char const *name, powerState_t *pps);
static regulatorFail_t handleSRAMRegulatorAlert(
    powerManagerEventType_t eventType, uint32_t data, char const *name, powerState_t *pps);

static regulatorFail_t handle12VPowerGoodFailure(
    powerManagerEventType_t eventType, uint32_t data, char const *name, powerState_t *pps);
static regulatorFail_t handle3P3VPowerGoodFailure(
    powerManagerEventType_t eventType, uint32_t data, char const *name, powerState_t *pps);
static regulatorFail_t handle1P8VPowerGoodFailure(
    powerManagerEventType_t eventType, uint32_t data, char const *name, powerState_t *pps);
static regulatorFail_t handleBUS0PowerGoodFailure(
    powerManagerEventType_t eventType, uint32_t data, char const *name, powerState_t *pps);
static regulatorFail_t handleBUS1PowerGoodFailure(
    powerManagerEventType_t eventType, uint32_t data, char const *name, powerState_t *pps);
static regulatorFail_t handleMNNPowerGoodFailure(
    powerManagerEventType_t eventType, uint32_t data, char const *name, powerState_t *pps);
static regulatorFail_t handleNOCPowerGoodFailure(
    powerManagerEventType_t eventType, uint32_t data, char const *name, powerState_t *pps);
static regulatorFail_t handleSRAMPowerGoodFailure(
    powerManagerEventType_t eventType, uint32_t data, char const *name, powerState_t *pps);
static regulatorFail_t handlePowerGoodFailure(
    powerManagerEventType_t eventType, uint32_t data, char const *name, powerState_t *pps);

/***********************************************************************
 * GLOBAL variables:  Defininition of global variables and constants
***********************************************************************/
uint32_t perstDownCount = 0;
uint8_t testPmbStats = 0;
static const gpioId_t onOffGpioOrder[] = { VDD_1P8V_EN_OUT, VDDA_1P8V_EN_OUT, MNN_NOC_PWR_RST_OUT, NOC_EN_OUT,
    MNN_EN_OUT, SRAM_EN_OUT, LOGIC_EN_OUT, QLP_EN_OUT, MXN_EN_OUT, DDR_EN_OUT, Q_EN_OUT, PCIE_EN_OUT };

// clang-format off
t_pmcDevs pmcDevs[2][2] = {
    { { eID_LTM4680p0,    "SRMp0", "SRM" }, { eID_LTM4680p1, "SRMp1", "SRM"  } },
    { { eID_TPS53681_MNN, "MNN", "MNN"   }, { eID_TPS53681_NOC, "NOC", "NOC" } }
};
// clang-format on

pmbStats_t pmbStats; //todo SW-16056: avoid using global variables

// clang-format off
static const powerManagementEventInfo_t powerManagementEventInfo[] = 
{
    { .eventType = POWER_GENERAL_PERST_IN,       .eventHandler = &handlePerstEvent,                 .name = "PERST_IN_N" }, 
    { .eventType = POWER_GENERAL_WDT,            .eventHandler = &handleWatchdogTimerEvent,         .name = "WDT_RESET_N" },

    { .eventType = POWER_MONITORING_VOLTAGE,     .eventHandler = &handleVoltageMonitoringEvent,     .name = "VMON_12V" },
    { .eventType = POWER_MONITORING_POWER,       .eventHandler = &handlePowerMonitoringEvent,       .name = "IMON_12V" },
    { .eventType = POWER_MONITORING_TEMPERATURE, .eventHandler = &handleTemperatureMonitoringEvent, .name = "TEMPERATURE" },

    { .eventType = POWER_ALERT_TEMPERATURE,      .eventHandler = &handleTemperatureAlert,         .name = "TEMP_ALERT_N" },
    { .eventType = POWER_ALERT_MNN_NOC,          .eventHandler = &handleMNNOrNOCRegulatorAlert,   .name = "TPS_ALERT" },
    { .eventType = POWER_ALERT_SRAM,             .eventHandler = &handleSRAMRegulatorAlert,       .name = "SRAM_ALERT" },
    { .eventType = POWER_ALERT_SRAM_FAULT,       .eventHandler = &handleSRAMRegulatorAlert,       .name = "SRAM_FAULT" },
    { .eventType = POWER_ALERT_SRAM_EN_SNS,      .eventHandler = &handleSRAMRegulatorAlert,       .name = "SRAM_EN_SNS" },

    { .eventType = POWER_FAILURE_PG_12V,         .eventHandler = &handle12VPowerGoodFailure,      .name = "PG_12V" },
    { .eventType = POWER_FAILURE_PG_3P3V,        .eventHandler = &handle3P3VPowerGoodFailure,     .name = "PG_3P3V" },
    { .eventType = POWER_FAILURE_PG_BUS0,        .eventHandler = &handleBUS0PowerGoodFailure,     .name = "PG_BUS0" },
    { .eventType = POWER_FAILURE_PG_BUS1,        .eventHandler = &handleBUS1PowerGoodFailure,     .name = "PG_BUS1" },
    { .eventType = POWER_FAILURE_PG_MNN,         .eventHandler = &handleMNNPowerGoodFailure,      .name = "PG_MNN" },
    { .eventType = POWER_FAILURE_PG_NOC,         .eventHandler = &handleNOCPowerGoodFailure,      .name = "PG_NOC" },
    { .eventType = POWER_FAILURE_PG_DDR,         .eventHandler = &handlePowerGoodFailure,         .name = "PG_DDR" },
    { .eventType = POWER_FAILURE_PG_MXN,         .eventHandler = &handlePowerGoodFailure,         .name = "PG_MXN" },
    { .eventType = POWER_FAILURE_PG_QLP,         .eventHandler = &handlePowerGoodFailure,         .name = "PG_QLP" },
    { .eventType = POWER_FAILURE_PG_Q,           .eventHandler = &handlePowerGoodFailure,         .name = "PG_Q" },
    { .eventType = POWER_FAILURE_PG_LOGIC,       .eventHandler = &handlePowerGoodFailure,         .name = "PG_LOGIC" },
    { .eventType = POWER_FAILURE_PG_PCIE,        .eventHandler = &handlePowerGoodFailure,         .name = "PG_PCIE" },
    { .eventType = POWER_FAILURE_PG_1P8,         .eventHandler = &handle1P8VPowerGoodFailure,     .name = "PG_1P8" },
    { .eventType = POWER_FAILURE_PG_SRAM,        .eventHandler = &handleSRAMPowerGoodFailure,     .name = "PG_SRAM" },
    { .eventType = POWER_FAILURE_IOXPANDER_0,    .eventHandler = NULL,                            .name = "GPIO0_INT_N" }, //handled separately
    { .eventType = POWER_FAILURE_IOXPANDER_1,    .eventHandler = NULL,                            .name = "GPIO1_INT_N" }, //handled separately
};
// clang-format on

vout_t vout[eIdx_MaxCount] = {
    { &voutInfoTbl[eIdx_QLP], 0, 0 },
    { &voutInfoTbl[eIdx_SRM], 0, 0 },
    { &voutInfoTbl[eIdx_DDR], 0, 0 },
    { &voutInfoTbl[eIdx_DDQ], 0, 0 },
    { &voutInfoTbl[eIdx_PCL], 0, 0 },
    { &voutInfoTbl[eIdx_PCI], 0, 0 },
    { &voutInfoTbl[eIdx_MXN], 0, 0 },
    { &voutInfoTbl[eIdx_NOC], 0, 0 },
    { &voutInfoTbl[eIdx_MNN], 0, 0 },
    { NULL, 0, 0 },
};

static t_pmbScanItem const scanListTbl[];
static t_pmbScanItem const *scanList[] = {
    &scanListTbl[0],
    &scanListTbl[1],
    &scanListTbl[2],
    &scanListTbl[3],
    NULL,
};
static uint8_t scanListLen = 4;
static uint8_t nscanchan = NSCANCHAN;

/***********************************************************************
 * Functions:   Declaration of local functions
 ***********************************************************************/
static regulatorFail_t handlePowerGoodFaults(uint8_t device);
static void clearReguFaults(void);
static void handlePerstDown(powerState_t *pps);
static void handlePerstUp(powerState_t *pps);
static void handleIoxpanderFailureEvent(powerManagerEventType_t ioxpanderEvent, powerState_t *pps);
static void handlePowerManagementEvent(powerManagerEventType_t eventType, uint32_t data, powerState_t *pps);

static bool is12VPower(const analogData_t *pad);
static void waitFor12VPower(const analogData_t *pad);
static bool waitForLTM4680(void);
static bool hwInit(t_cprf *pcprf, powerState_t *pps);
static bool checkRegulatorPgOnPgBus(powerState_t const *pps);
static bool checkRegulatorPgOnIoXp(void);
static bool checkRegulatorPg(powerState_t const *pps);
static uint8_t detailPmcStatus(t_pmcDevs const *pDev);
static void onPowerOk(powerState_t *pps);

/***********************************************************************
 * GLOBAL Functions:   Defininition of global functions
***********************************************************************/
void powerManagerInitialize(void)
{
    gpio_register_EIC_handler(PERST_IN, perstInInterruptCallback);
    gpio_register_EIC_handler(WDT_RST_IN, wdtResetInterruptCallback);
    if (isFeaturePerHwEncodingEnabled(POWER_GOOD_12V_SIGNAL_PRESENT))
    {
        gpio_register_EIC_handler(POWER_GOOD_12V_IN, powerGood12VInterruptCallback);
    }
    if (isFeaturePerHwEncodingEnabled(POWER_GOOD_3P3V_SIGNAL_PRESENT))
    {
        gpio_register_EIC_handler(POWER_GOOD_3P3V_IN, powerGood3P3VInterruptCallback);
    }
    gpio_register_EIC_handler(TEMP_ALERT_IN, alertTempInterruptCallback);
    gpio_register_EIC_handler(MNN_NOC_ALERT_IN, alertMNNorNOCInterruptCallback);
    gpio_register_EIC_handler(SRAM_ALERT_IN, alertSRAMInterruptCallback);
    if (isFeaturePerHwEncodingEnabled(POWER_GOOD_ON_IOXPANDER_PRESENT))
    {
        gpio_register_EIC_handler(IOXP_0_IN, ioxpander0InterruptCallback);
        gpio_register_EIC_handler(IOXP_1_IN, ioxpander1InterruptCallback);
    }
    else
    {
        gpio_register_EIC_handler(POWER_GOOD_BUS0_IN, powerGoodBus0interruptCallback);
        gpio_register_EIC_handler(POWER_GOOD_BUS1_IN, powerGoodBus1InterruptCallback);
    }
    gpio_register_ADC_handler(IMON_12V_AIN, adcIMON12VInterruptCallback);
    gpio_register_ADC_handler(VMON_12V_AIN, adcVMON12VInterruptCallback);
}

void IOTask(void *pvParameters)
{
    t_cprf cprf = { 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0, 0, 0, 0, 0, 0, NULL };
    commonData_t *pCommonData = (commonData_t *)pvParameters;
    powerState_t *pps = &pCommonData->powerState;
    const analogData_t *pad = &pCommonData->analogData;
    pfiShowTime = 1;
    int status = STATUS_SUCCESS;

    if (HW_ENCODING_PCIE_1088 == Hw_Encoding_Get_Hw_Version_Encoding())
    {
        pmcDevs[0][0].eID = eID_TPSM8D6C24p0;
        pmcDevs[0][1].eID = eID_TPSM8D6C24p1;

        vout[1].voutInfo = &voutInfoTbl[eIdx_SRM_TPSM8D6C24];

        scanList[2] = &scanListTbl[4];
        scanList[3] = NULL;
        scanListLen = 3;
        nscanchan = NCHAN;
    }

    globals.commonData.powerState.deferPerstUp = false;
    globals.commonData.powerState.socPerstToggled = false;

    FASTTASKSEM_GIVE // release fastTask

        status = systemEnableHwInterrupts();
    assertMsg((status == STATUS_SUCCESS), "IOTASK: Enable system interrupts failed");

    recordTime("enter IOTask");

    status = i2cmInit();
    assertMsg((status == STATUS_SUCCESS), "IOTASK: Enable I2C master init failed");

    status = ConsoleInit();
    assertMsg((status == STATUS_SUCCESS), "IOTASK: Console init failed");

    PRINTSEM_TAKE
    printf("\n........\n");
    printf("Platform: %s, REV %ld.%ld\n", Hw_Encoding_Get_Hw_Board_Name(), Hw_Encoding_Get_Hw_Design_Revision(),
        Hw_Encoding_Get_Hw_Modification_Revision());
    printf("Build type: %s\n", Image_Metadata_Decode_Build_Type(Image_Metadata_Get_Curr_Build_Type()));
    printf("Fw version: v%ld.%ld.%ld\n", GET_VERSION_MAJOR(Config_Get_Fw_Ver_From_Metadata(BOOT_IMAGE_SLOT)),
        GET_VERSION_MINOR(Config_Get_Fw_Ver_From_Metadata(BOOT_IMAGE_SLOT)),
        GET_VERSION_PATCH(Config_Get_Fw_Ver_From_Metadata(BOOT_IMAGE_SLOT)));
    printf("Commit info: %s, branch: %s, hash: %s\n", LASTCOMMITTIME, BRANCH,
        Config_Get_Image_Hash_From_Metadata(BOOT_IMAGE_SLOT));
    printf("Number of total failed boot attempts: %ld\n", Config_Get_Failed_Boot_Cnt_From_Config_Header());
    printf("........\n\n");

    if (isFeaturePerHwEncodingEnabled(POWER_GOOD_ON_IOXPANDER_PRESENT))
    {
        ioxpander_init();
    }

    gpio_set_out_high_pin_id(VDD_3P3V_EN_OUT);
    if (HW_ENCODING_PCIE_1088 == Hw_Encoding_Get_Hw_Version_Encoding())
    {
        gpio_set_out_high_pin_id(VDD_1P8V_EN_OUT);
    }

    pps->waiting12V = true;
    PRINTSEM_GIVE

    if (Hw_Encoding_Get_Hw_Board_Type() == HW_ENCODING_BOARD_TYPE_BUB)
    {
        OSHWDELAY_MS(50) // helps SRM come up - no penalty delaying on BUB
    }

    // globals.ioSemaphore is initially taken, so CLITask is blocked
    interruptingPrintfOneLine("waiting for 12V power\n");
    recordTime("before waitFor12VPower");
    waitFor12VPower(pad);
    recordTime("after waitFor12VPower");

    bool pwrOk = false;
    pps->reqPwrUp = false;
    if (!checkFS1406Comm())
    {
        interruptingPrintfOneLine("FS1406 Comm failure - Are they programmed for 1.8V?");
    }
    else
    {
        recordTime("after checkFS1406Comm");
        pps->waiting12V = false;

        if (!waitForLTM4680())
        {
            interruptingPrintfOneLine("wait for LTM4680 failed\n");
        }
        recordTime("after waitForLTM4680");

        interruptingPrintfOneLine("Turn on power\n");
        pps->skipReguCheck = false;
        pwrOk = hwInit(&cprf, pps);
        interruptingPrintfOneLine("hwInit %s on power\n", pwrOk ? "turned" : "failed to turn");
        if (pwrOk)
        {
            onPowerOk(pps);
        }
        else
        {
            gpioDown(pps);
        }
    }

    // releases CLITask and etsocCmdHandlerTask
    IOSEM_GIVE

    status = initPmbStats();
    assertMsg((status == STATUS_SUCCESS), "IOTASK: PMB stats init failed");

    uint64_t t = getTimer2();
    uint64_t pmbStatsInitTime = t + CPU_FREQUENCY / 10; // 100 mS
    uint64_t time1SecDelay = t + CPU_FREQUENCY;         // 1 Second
    uint64_t timer1Sec = t;
    while (1)
    {
        uint64_t now = getTimer2();

        if (time1SecDelay != 0 && t >= time1SecDelay)
        {
            time1SecDelay = 0;
            checkPrintfFromInterrupt();
            pfiShowTime = 0;
        }

        if (pps->reqPwrUp && !pps->hwUp)
        {
            IOSEM_TAKE
            pps->reqPwrUp = false;
            pwrOk = hwInit(&cprf, pps);
            interruptingPrintfOneLine("Request power up: hwInit %s on power\n", pwrOk ? "turned" : "failed to turn");
            if (pwrOk)
            {
                onPowerOk(pps);
                pmbStatsInitTime = now + CPU_FREQUENCY / 10; // 100 mS
            }
            else
            {
                gpioDown(pps);
            }
            IOSEM_GIVE
        }

        t += CPU_FREQUENCY / MONITOR_FREQ; // 10 ms
        if (now > pmbStatsInitTime && !testPmbStats)
        {
            status = initPmbStats();
            pmbStatsInitTime = (uint64_t)(-1LL);
        }

        int32_t u = ((int32_t)(t - now));
        if (u > 0)
        {
            hwOsDelayUS((uint32_t)u / (CPU_FREQUENCY / 1000000));
        }

        if (!is12VPower(pad))
        {
            interruptingPrintfOneLine("12V too low. Rebooting in 2 seconds");
            OSHWDELAY_MS(2000) // wait for power really to go away
            reboot();
        }

        pwrOk = pps->hwUp;
        if (pwrOk && (!pps->skipReguCheck))
        {
            readPmbStats();
        }
        if (now >= timer1Sec + (uint64_t)(1.0 * CPU_FREQUENCY))
        {
            FlushPrintfFromInterrupt();
            timer1Sec = now;
        }
    }
}

void fastTask(void *pvParameters)
{
    commonData_t *pCommonData = (commonData_t *)pvParameters;
    powerState_t *pps = &pCommonData->powerState;

    // wait for IOTask to release
    FASTTASKSEM_TAKE
    FASTTASKSEM_GIVE

    while (1)
    {
        powerManagerEvent_t powerManagementEvent;
        xQueueReceive(globals.powerManagementTaskQueueHandle, &powerManagementEvent, portMAX_DELAY);
        powerManagerEventType_t eventType = powerManagementEvent.eventType;
        uint32_t data = powerManagementEvent.data;

        switch (eventType)
        {
            // FT_intrPrint will be removed and handled in Logging task
            case FT_intrPrint:
                checkPrintfFromInterrupt();
                break;
            case FT_oflo:
                interruptingPrintfOneLine("fastTaskQueueHandle overflow");
                break;
            // IoExpander interrupts are handled separatelly, event types are read from register and eventually handlers are invoked
            case POWER_FAILURE_IOXPANDER_0:
            case POWER_FAILURE_IOXPANDER_1:
            {
                if (!pps->hwUp)
                {
                    break;
                }
                handleIoxpanderFailureEvent(eventType, pps);
            }
            break;
            default:
                handlePowerManagementEvent(eventType, data, pps);
                break;
        }
    }
}

uint32_t getPerstDownCount(void)
{
    return perstDownCount;
}

void incrementPerstDownCount(void)
{
    perstDownCount++;
}

void gpioUp(powerState_t *pps)
{
    (void)pps;
    for (unsigned int i = 0; i < ARRAY_SIZE(onOffGpioOrder); i++)
    {
        // Details of why this delay is needed is discussed in SW-20314
        if (onOffGpioOrder[i] == LOGIC_EN_OUT)
        {
            OSHWDELAY_MS(1.5)
        }
        if (onOffGpioOrder[i] == SRAM_EN_OUT)
        {
            OSHWDELAY_MS(1.5)
        }
        bool *pd = findVoutDisableByPinId(onOffGpioOrder[i]);
        if (!pd || !*pd)
        {
            gpio_set_out_high_pin_id(onOffGpioOrder[i]);
        }
    }
}

void gpioDown(powerState_t *pps)
{
    gpio_set_out_low_pin_id(SOC_RST_OUT);
    pps->hwUp = false; // set 0 before power down to avoid race condition
    pps->deferPerstUp = false;
    if (!pps->skipReguCheck)
    {
        globals.blockPTFail = true;
    }
    for (unsigned int i = 0; i < ARRAY_SIZE(onOffGpioOrder); i++)
    {
        gpio_set_out_low_pin_id(onOffGpioOrder[ARRAY_SIZE(onOffGpioOrder) - 1 - i]);
    }
    gpio_set_out_low_pin_id(SOC_PERST_OUT);
    pps->socPerstToggled = false;
}

void cmdPowerOn(powerState_t *pps)
{
    IOSEM_TAKE
    if (pps->hwUp)
    {
        interruptingPrintfOneLine("Power Already On");
        IOSEM_GIVE
        return;
    }
    interruptingPrintfOneLine("Power On");
    pps->reqPwrUp = true;
    IOSEM_GIVE
}

void cmdPowerOff(powerState_t *pps)
{
    IOSEM_TAKE
    if (!pps->hwUp && !gpio_get_out_pin_id(VDD_1P8V_EN_OUT))
    {
        interruptingPrintfOneLine("Power Already Off");
        IOSEM_GIVE
        return;
    }
    interruptingPrintfOneLine("Power Off");
    gpioDown(pps);
    IOSEM_GIVE
}

void cmdResetSoc(void)
{
    interruptingPrintfOneLine("SOC reset");
    gpio_set_out_low_pin_id(SOC_RST_OUT);
    OSDELAY_MS(10)
    gpio_set_out_high_pin_id(SOC_RST_OUT);
}

void cmdSpiPwr(powerState_t *pps)
{
    IOSEM_TAKE
    if (pps->hwUp)
    {
        interruptingPrintf("Power Off\n");
        gpioDown(pps);
    }
    gpio_set_out_high_pin_id(VDD_1P8V_EN_OUT);
    interruptingPrintfOneLine("Only 1.8V on.");
    IOSEM_GIVE
}

bool checkFS1406Comm(void)
{
    bool ok = true;
    for (t_structChipInfo const *p = structChipInfo; p < &structChipInfo[MAX_CHIPS]; ++p)
    {
        uint8_t value = 0;
        if (p->pStructRegInfo != structRegInfo_FS1406)
        {
            continue;
        }
        if ((i2cm_read8(p->addr7, FS1406__REG1A, &value) != I2CM_OK) || !(value & 0x02))
        {
            ok = 0;
            break;
        }
    }
    return ok;
}

void pmbReadback(uint8_t addr7)
{
    uint16_t j;
    char rw[3];

    char const *name = NULL;
    for (t_structChipInfo const *p = structChipInfo; p < &structChipInfo[MAX_CHIPS]; ++p)
    {
        if (p->addr7 != addr7)
        {
            continue;
        }
        name = p->chipName;
        break;
    }
    if (!name)
    {
        interruptingPrintfOneLine("address not found");
        return;
    }

    t_structRegInfo const *structRegInfo = PMBus_RegInfo;

    if (!structRegInfo)
    {
        interruptingPrintfOneLine("not PBM part");
        return;
    }

    interruptingPrintBegin();
    interruptingPrintf("PMB readback(%s)\n", name);

    for (uint8_t i = j = 0; i <= 0xFF; ++i)
    {
        t_structRegInfo const *pj = &structRegInfo[j];
        if (pj->regnum != i)
        {
            while (structRegInfo[j].regnum == i)
            {
                ++j;
            }
            continue;
        }
        uint8_t props = xlatRTtoProp[pj->epType];

        rw[0] = '\0';

        if (props & eP_Read)
        {
            strcat(rw, "R");
        }
        else
        {
            while (structRegInfo[j].regnum == i)
            {
                ++j;
            }
            continue; // skip write only
        }

        if (props & eP_Write)
        {
            strcat(rw, "W");
        }
        else
        {
            while (structRegInfo[j].regnum == i)
            {
                ++j;
            }
            continue; // skip read-only
        }

        uint64_t val64 = 0;
        uint8_t regnum = pj->regnum;
        uint8_t nbytes = pj->nbytes;

        if (!(props & eP_Read))
        {
            nbytes = 0xE0;
        }
        else if (props & eP_8Bit)
        {
            i2c_read_8bit(addr7, i, (uint8_t *)&val64);
        }
        else if (props & eP_16Bit)
        {
            i2c_read_16bit(addr7, i, (uint16_t *)&val64);
        }
        else if (props & eP_Blk)
        {
            i2c_read_blk(addr7, i, &val64, &nbytes, pj->nbytes);
        }
        else
        {
            nbytes = 0xEF;
        }

        if (nbytes != 0xE0)
        {
            interruptingPrintf("%02X %-2s  ", regnum, rw);
            interruptingPrintf("%s\n", (nbytes > 0xE0) ? "(%02X)" : "%0*llX", (nbytes > 0xE0) ? nbytes : 2 * nbytes,
                (nbytes > 0xE0) ? nbytes : val64);
        }

        while (structRegInfo[j].regnum == i)
        {
            ++j;
        }
    }

    interruptingPrintEnd();
}

// TODO: enqueueCommandFasttask will be removed when Logging task is implemented
bool enqueueCommandFasttask(powerManagerEventType_t eventType, BaseType_t *pxHigherPriorityTaskWoken)
{
    UBaseType_t nm = uxQueueMessagesWaitingFromISR(globals.powerManagementTaskQueueHandle);
    if (nm >= POWER_MANAGEMENT_TASK_QUEUE_LENGTH)
    {
        return pdPASS; // drop it
    }

    powerManagerEventType_t ftEvent = (nm < POWER_MANAGEMENT_TASK_QUEUE_LENGTH - 1) ? eventType : FT_oflo;
    powerManagerEvent_t powerManagementEvent = { .eventType = ftEvent, .data = 0 };
    return xQueueSendToBackFromISR(
        globals.powerManagementTaskQueueHandle, &powerManagementEvent, pxHigherPriorityTaskWoken);
}

bool checkRegu(void)
{
    interruptingPrintf("EIC before %04X\n", EIC->INTFLAG.reg);
    uint8_t status = 0;
    for (uint8_t i = 0; i < 2; ++i)
        for (uint8_t page = 0; page < 2; ++page)
        {
            t_pmcDevs const *pDev = &pmcDevs[i][page];
            interruptingPrintf("\ndetail %s\n", pDev->name);
            status = detailPmcStatus(pDev);
            if (status != 0)
                return false;
        }
    return checkVoltageRegisters();
}

int dumpFS1406Regu(void)
{
    uint8_t addr;
    i2cmErr_t rv = 0;
    uint8_t data;

    interruptingPrintBegin();
    static uint8_t const eid[] = { DDR, MXN, QLP, Q, eID_FS1406_LOGIC, eID_FS1406_PCI, eID_FS1406_1P8V };
    static char const *const names[] = { "DDR", "MXN", "QLP", "Q", "LOGIC", "PCI", "1P8V" };
    for (uint32_t i = 0; i < countof(eid); ++i)
    {
        addr = structChipInfo[i].addr7;
        interruptingPrintf("%-6s addr %02X\n", names[i], addr);
        for (uint8_t index = 0; index < MAXREG_FS1406; ++index)
        {
            uint8_t reg = structChipInfo[i].pStructRegInfo[index].regnum;
            rv = i2cm_read8(addr, reg, &data);
            interruptingPrintf(" %s ( %02X) ", structChipInfo[i].pStructRegInfo[index].regName, reg);
            if (rv != I2CM_OK)
            {
                i2cPError(rv, "");
                return false;
            }
            else
            {
                interruptingPrintf("--> %02X\n", data);
            }
        }
    }
    interruptingPrintEnd();
    return STATUS_SUCCESS;
}

void setRegChk(const char *p1, powerState_t *pps)
{
    static const char *offOn[] = { "off", "on" };
    if (p1)
    {
        bool ok = true;
        int v = atodec(p1, &ok);
        if (!ok || (v & ~0x01) != 0)
        {
            interruptingPrintf("must be 0 or 1");
            return;
        }

        pps->skipReguCheck = v == 0;
        globals.blockPTFail = pps->skipReguCheck ? 1 : !pps->hwUp;
    }
    interruptingPrintfOneLine("regulator checking is %s", offOn[!pps->skipReguCheck]);
}

static bool handleI2CError(uint8_t rvs[][NREG], t_pmbScanItem const **scanLists)
{
    for (uint8_t ichan = 0; ichan < nscanchan; ++ichan)
    {
        t_I2cmScanItem const *sList = scanLists[ichan]->items;
        for (uint8_t ireg = 0; ireg < NREG; ++ireg)
        {
            i2cmErr_t rv = rvs[ichan][ireg];
            if (rv != I2CM_OK)
            {
                interruptingPrintf("readPmbStats: I2C error %s, addr %02X, reg %02X\n", i2cm_get_error_message(rv),
                    sList[ireg].addr, sList[ireg].reg);
                return false;
            }
        }
    }
    return true;
}

// Function to update pmbStat_t structure
static void updatePmbStat(pmbStat_t *piThis, uint16_t val, uint16_t raw)
{
    piThis->vars.cur = val;
    piThis->vars.rawcur = raw;
    if (piThis->vars.min > val)
    {
        piThis->vars.min = val;
        piThis->vars.rawmin = raw;
    }
    if (piThis->vars.max < val)
    {
        piThis->vars.max = val;
        piThis->vars.rawmax = raw;
    }
    uint64_t ave64 = piThis->vars.ave;

    ave64 = (ave64 != 0xFFFFFFFF) ?
                ((ave64 * (uint64_t)(N_AVE - 1) + ((uint64_t)val << 16) + ((uint64_t)N_AVE / 2)) / (uint64_t)N_AVE) :
                ((uint64_t)val << 16);

    piThis->vars.ave = (uint32_t)ave64;
}

static uint16_t vals[NSCANCHAN][NREG];
static uint8_t rvs[NSCANCHAN][NREG];

static t_pmbScanItem const scanListTbl[] = {
    {
        .items = {
            { ADR_MNN, TPS53681__MFR_SPECIFIC_04, 2, &vals[0][0] },
            { ADR_MNN, PMBUS__READ_IOUT, 2, &vals[0][1] },
            { ADR_MNN, PMBUS__READ_POUT, 2, &vals[0][2] },
            { ADR_TPS53681, PMBUS__READ_VIN, 2, &vals[0][3] }, /* there is no channel specific VIN/IIN/PIN */
            { ADR_TPS53681, PMBUS__READ_IIN, 2, &vals[0][4] },
            { ADR_TPS53681, PMBUS__READ_PIN, 2, &vals[0][5] },
            { ADR_MNN, PMBUS__READ_TEMPERATURE_1, 2, &vals[0][6] },
        },
    },
    {
        .items = {
            { ADR_NOC, TPS53681__MFR_SPECIFIC_04, 2, &vals[1][0] },
            { ADR_NOC, PMBUS__READ_IOUT, 2, &vals[1][1] },
            { ADR_NOC, PMBUS__READ_POUT, 2, &vals[1][2] },
            { ADR_TPS53681, PMBUS__READ_VIN, 2, &vals[1][3] }, /* there is no channel specific VIN/IIN/PIN */
            { ADR_TPS53681, PMBUS__READ_IIN, 2, &vals[1][4] },
            { ADR_TPS53681, PMBUS__READ_PIN, 2, &vals[1][5] },
            { ADR_NOC, PMBUS__READ_TEMPERATURE_1, 2, &vals[1][6] },
        },
    },
    {
        .items = {
            { ADR_SRAMP0, PMBUS__READ_VOUT, 2, &vals[2][0] },
            { ADR_SRAMP0, PMBUS__READ_IOUT, 2, &vals[2][1] },
            { ADR_SRAMP0, PMBUS__READ_POUT, 2, &vals[2][2] },
            { ADR_SRAMP0, PMBUS__READ_VIN, 2, &vals[2][3] },
            { ADR_SRAMP0, PMBUS__READ_IIN, 2, &vals[2][4] },
            { ADR_SRAMP0, PMBUS__READ_PIN, 2, &vals[2][5] },
            { ADR_SRAMP0, PMBUS__READ_TEMPERATURE_1, 2, &vals[2][6] },
        },
    },
    {
        .items = {
            { ADR_SRAMP1, PMBUS__READ_VOUT, 2, &vals[3][0] },
            { ADR_SRAMP1, PMBUS__READ_IOUT, 2, &vals[3][1] },
            { ADR_SRAMP1, PMBUS__READ_POUT, 2, &vals[3][2] },
            { ADR_SRAMP1, PMBUS__READ_VIN, 2, &vals[3][3] },
            { ADR_SRAMP1, PMBUS__READ_IIN, 2, &vals[3][4] },
            { ADR_SRAMP1, PMBUS__READ_PIN, 2, &vals[3][5] },
            { ADR_SRAMP1, PMBUS__READ_TEMPERATURE_1, 2, &vals[3][6] },
        },
    },
    {
        .items = {
            { ADR_SRAM_TPSM8D6C24, PMBUS__READ_VOUT, 2, &vals[2][0] },
            { ADR_SRAM_TPSM8D6C24, PMBUS__READ_IOUT, 2, &vals[2][1] },
            { ADR_SRAM_TPSM8D6C24, PMBUS__READ_VOUT, 2, &vals[2][2] }, /* POUT needs to be calculated */
            { ADR_SRAM_TPSM8D6C24, PMBUS__READ_VIN, 2, &vals[2][3] },
            { ADR_SRAMP0_TPSM8D6C24, PMBUS__READ_IIN, 0xFF, &vals[2][4] }, /* always zero */
            { ADR_SRAM_TPSM8D6C24, PMBUS__READ_VIN, 0xFF, &vals[2][5] }, /* always zero */
            { ADR_SRAM_TPSM8D6C24, PMBUS__READ_TEMPERATURE_1, 2, &vals[2][6] },
        },
    },
};

static t_I2cmScanBlock const scanBlock = { scanList, (uint8_t *)rvs, &scanListLen };

int getPmbStatsMnnVout(void)
{
    IOSEM_TAKE
    int out = pmbStats.item[echn_MNN][0].vars.cur;
    IOSEM_GIVE
    return out;
}

int getPmbStatsNocVout(void)
{
    IOSEM_TAKE
    int out = pmbStats.item[echn_NOC][0].vars.cur;
    IOSEM_GIVE
    return out;
}

int getPmbStatsSrmVout(void)
{
    IOSEM_TAKE
    int out = pmbStats.item[echn_SRM][0].vars.cur;
    IOSEM_GIVE
    return out;
}

int readPmbStats(void)
{
    memset(vals, 0, sizeof(vals));
    i2cmScanBlock(&scanBlock);
    if (globals.blockPTFail || testPmbStats)
        return PWR_MGR_ERROR_PMB_STATS;

    uint8_t ichan;
    uint8_t ireg;
    if (!handleI2CError(rvs, scanList))
    {
        IOSEM_TAKE
        ++pmbStats.nFails.u64;
        IOSEM_GIVE
        return STATUS_SUCCESS;
    }

    uint32_t val, v = 0, i = 0;
    uint8_t hw_encoding_version = Hw_Encoding_Get_Hw_Version_Encoding();

    for (ichan = 0; ichan < nscanchan; ++ichan)
    {
        for (ireg = 0; ireg < NREG; ++ireg)
        {
            uint16_t val1 = vals[ichan][ireg];
            // different conversion algorithms
            switch (scanList[ichan]->items[ireg].reg)
            {
                case PMBUS__READ_VOUT:
                    switch (hw_encoding_version)
                    {
                        case HW_ENCODING_PCIE_1088:
                            val = rawToMvL16n9(val1);
                            break;
                        default:
                            val = rawToMvL16(val1);
                            break;
                    }
                    break;
                case TPS53681__MFR_SPECIFIC_04:
                default:
                    val = rawToL11FmtX1k(val1);
                    break;
            }
            if ((HW_ENCODING_PCIE_1088 == hw_encoding_version) && (ichan >= echn_SRM))
            {
                switch (ireg)
                {
                    case 0: // VOUT
                        v = val;
                        break;
                    case 1: // IOUT
                        i = val;
                        break;
                    case 2: // 2nd VOUT
                        val = (val + v) * i / 2000;
                        break;
                    default:
                        break;
                }
            }

            IOSEM_TAKE
            pmbStat_t *pOut = &pmbStats.item[ichan][ireg];
            if (echn_SRM2 == ichan)
            {
                pOut = &pmbStats.item[echn_SRM][ireg];
                val = pOut->vars.cur + val;     // all ireg are summed or averaged
                if (((uint8_t)(ireg - 1)) >= 2) // ireg != 1 or 2
                    val >>= 1;
            }
            updatePmbStat(pOut, val, val1);
            IOSEM_GIVE
        }
    }
    IOSEM_TAKE
    ++pmbStats.nSamples.u64;
    IOSEM_GIVE

    return STATUS_SUCCESS;
}

int initPmbStats(void)
{
    if (testPmbStats)
    {
        return GENERAL_ERROR;
    }

    static pmbVars_t const patt = { 0, 0xFFFF, 0x0000, 0xFFFFFFFF, 0, 0, 0 };
    IOSEM_TAKE
    for (uint8_t ichan = 0; ichan < NCHAN; ++ichan)
    {
        for (uint8_t ireg = 0; ireg < NREG; ++ireg)
        {
            memcpy(&pmbStats.item[ichan][ireg].vars, &patt, sizeof(pmbVars_t));
        }
    }
    pmbStats.nSamples.u64 = pmbStats.nFails.u64 = 0;
    IOSEM_GIVE

    return STATUS_SUCCESS;
}

void printPmbStats(void)
{
    IOSEM_TAKE
    for (uint8_t is = 0; is < NSTAT; ++is)
    {
        interruptingPrintf("\n%-4s ID", pmbStatNames[is]);
        for (uint8_t ir = 0; ir < NREG; ++ir)
        {
            interruptingPrintf("%7s", pmbRegNames[ir]);
        }
        interruptingPrintNL();
        for (uint8_t ic = 0; ic < NCHAN; ++ic)
        {
            interruptingPrintf("    %-3s", pmbId[ic]);
            for (uint8_t ir = 0; ir < NREG; ++ir)
            {
                int32_t t = is < 3 ? pmbStats.item[ic][ir].arr.stats[is] :
                                     (pmbStats.item[ic][ir].arr.ave + (1 << 15)) >> 16;
                interruptingPrintf("%3d.%03d", t / 1000, t % 1000);
            }
            interruptingPrintNL();
            if (is < 3)
            {
                interruptingPrintf("       ");
                for (uint8_t ir = 0; ir < NREG; ++ir)
                {
                    int32_t raw = pmbStats.item[ic][ir].arr.raw[is];
                    interruptingPrintf("  x%04X", raw);
                }
                interruptingPrintNL();
            }
        }
    }
    interruptingPrintf("\nNSamples: %llu, NErrors: %llu\n", pmbStats.nSamples.u64, pmbStats.nFails.u64);
    IOSEM_GIVE
}

uint64_t getPmbStatsNumOfFails(void)
{
    return pmbStats.nFails.u64;
}

static int modifyPmbStats(void)
{
    IOSEM_TAKE
    for (int ichan = 0; ichan < NCHAN; ++ichan)
    {
        for (int ireg = 0; ireg < NREG; ++ireg)
        {
            uint16_t vv = VV_VALUE(ireg, ichan);
            vv = 0x5A5A;
            pmbStat_t *pi = &pmbStats.item[ichan][ireg];
            for (int itype = 0; itype < 3; ++itype)
            {
                pi->arr.stats[itype] = vv;
            }
            pi->arr.ave = vv << 16;
        }
    }
    IOSEM_GIVE

    return STATUS_SUCCESS;
}

int cmdU(char const *p1)
{
    bool ok = false;
    int status = STATUS_SUCCESS;
    uint32_t v = atohex(p1, &ok);
    if (!ok)
    {
        return INVALID_ARGUMENT;
    }
    if ((v & ~1) == 0)
    {
        testPmbStats = v != 0;

        if (testPmbStats == 0)
        {
            status = initPmbStats();
        }
        else
        {
            status = modifyPmbStats();
        }
        return status;
    }

    if (v == 4)
    {
        i2c_write(ADR_MAX6660, 0x09, 0x40);
        i2c_write(ADR_MAX6660, 0x0F, 0x0);

        uint8_t v0;
        uint8_t v1;
        i2c_read_8bit(ADR_MAX6660, 1, &v1);
        i2c_read_8bit(ADR_MAX6660, 0, &v0);

        uint16_t t = (uint16_t)(((uint16_t)v1) << 3) | (v0 >> 5);
        bool s = t & 0x400;
        if (s)
        {
            t = 0x800 - t;
        }
        interruptingPrintfOneLine("tempr %c%d.%03d, err=%d", s ? '-' : ' ', t >> 3, (t & 0x07) * 125, (v0 >> 4) & 0x01);
        return GENERAL_ERROR;
    }

    static uint32_t x = 0;
    for (uint32_t j = 0; j < v; ++j)
    {
        printfFromInterruptNoFlush("**%d", ++x, 0);
    }
    FlushPrintfFromInterrupt();

    return status;
}

bool getPowerState()
{
    return globals.commonData.powerState.hwUp;
}

/***********************************************************************
 * Functions:   Defininition of local functions
 ***********************************************************************/
static void onPowerOk(powerState_t *pps)
{
    gpio_set_out_high_pin_id(SOC_RST_OUT); // release
    recordTime("after SOC_RESET_N");
    pps->hwUp = true; // prevent raising PERST before power is up
    if (gpio_get_in_pin_id(PERST_IN))
    { // in case PERST_IN_N came before hwUp
        if (pps->deferPerstUp)
        {
            printfFromInterruptNoFlush("PERST high signal interrupt was detected before hw up.", 0, 0);
        }
        gpio_set_out_high_pin_id(SOC_PERST_OUT);
        pps->deferPerstUp = false;
        pps->socPerstToggled = true;
        printfFromInterrupt("%-25s", (uint32_t) "PERST high at SOC_RESET_N", 0);
    }
    else
    {
        printfFromInterrupt("%-25s", (uint32_t) "PERST low at SOC_RESET_N", 0);
    }
}

static bool is12VPower(const analogData_t *pad)
{
    while (pad->initV)
    {
        OSHWDELAY_MS(1)
    }
    return (pad->vmon12vMV > 9000); // 9 Volts
}

static void waitFor12VPower(const analogData_t *pad)
{
    uint64_t t0 = getTimer2();
    uint64_t period = 5000000 * MHZ; // 5 sec
    uint64_t t = t0 + period;
    while (!is12VPower(pad))
    {
        OSHWDELAY_MS(.1) // allow other tasks to run
        if (getTimer2() >= t)
        {
            interruptingPrintfOneLine("waiting for 12V\n");
            t += period;
            period = 60000000ULL * MHZ; // 60 sec
        }
    }
}

static bool waitForLTM4680(void)
{
    uint64_t t = getTimer2() + 100000 * MHZ; // 100 mS
    uint8_t addr;
    switch (Hw_Encoding_Get_Hw_Version_Encoding())
    {
        case HW_ENCODING_PCIE_1088: //Penguin RevB
            addr = ADR_SRAM_TPSM8D6C24;
            break;
        default:
            addr = ADR_SRAM;
            break;
    }
    while (i2cm_write0(addr, PMBUS__CLEAR_FAULT) != I2CM_OK)
    {
        OSHWDELAY_MS(.1) // allow other tasks to run
        if (getTimer2() >= t)
        {
            return 0;
        }
    }
    return 1;
}

static bool hwInit(t_cprf *pcprf, powerState_t *pps)
{
    configureETSOCInterruptsToDefault();

    chipInit(pcprf);
    recordTime("after chipInit");
    OSHWDELAY_MS(5)
    recordTime("after fixed delay");

    if (!pcprf->okSoFar)
    {
        interruptingPrintf("chipInit failed at macro #%d, msg: %s\n", pcprf->errId, pcprf->errMsg);
        gpioDown(pps);
        return false;
    }

    gpioUp(pps);
    recordTime("after gpioUp");

    if (setAllRegulatorsToDefaultVoltage() != STATUS_SUCCESS)
    { // setting voltage fails on TI regulator if output isn't enabled
        interruptingPrintf("Setting regulators to default voltage failed\n");
        gpioDown(pps);
        return false;
    }
    recordTime("after set all regu");

    // Re-enable Regulator Faults
    globals.blockPTFail = false;

    bool pg = checkRegulatorPg(pps);
    recordTime("after pGoods");
    if (!pg && !pps->skipReguCheck)
    {
        gpioDown(pps);
        return false;
    }

    clearReguFaults(); // after power ok
    recordTime("after clearReguFaults");

    return true;
}

static void handlePowerManagementEvent(powerManagerEventType_t eventType, uint32_t data, powerState_t *pps)
{
    uint32_t status = GENERAL_ERROR;
    regulatorFail_t faults = RF_none;

    interruptingPrintfOneLine("handlePowerManagementEvent: Received event with id: %d\n", eventType);

    if ((eventType & POWER_ALERT_EVENT) || (eventType & POWER_FAILURE_EVENT))
    {
        if (!pps->hwUp)
        {
            interruptingPrintfOneLine("handlePowerManagementEvent: hw not up, return\n");
            return;
        }
        if (globals.blockPTFail && eventType != POWER_FAILURE_PG_12V)
        {
            interruptingPrintfOneLine("handlePowerManagementEvent: blockPTFail == true, return\n");
            return;
        }
        if (pps->skipReguCheck)
        {
            interruptingPrintfOneLine("handlePowerManagementEvent: skipReguCheck == true, return\n");
            return;
        }
    }

    for (uint32_t eventInfoIdx = 0; eventInfoIdx < ARRAY_SIZE(powerManagementEventInfo); eventInfoIdx++)
    {
        if (powerManagementEventInfo[eventInfoIdx].eventType == eventType)
        {
            if ((eventType & POWER_ALERT_EVENT) || (eventType & POWER_FAILURE_EVENT))
            {
                if (gpio_get_in_pin_id(getPinIdForEventType(eventType)))
                {
                    interruptingPrintfOneLine("%s spurious interrupt", powerManagementEventInfo[eventInfoIdx].name);
                    return;
                }
            }

            if (powerManagementEventInfo[eventInfoIdx].eventHandler != NULL)
            {
                const char *name = powerManagementEventInfo[eventInfoIdx].name;
                faults |= powerManagementEventInfo[eventInfoIdx].eventHandler(eventType, data, name, pps);
                status = STATUS_SUCCESS;
            }
            else
            {
                interruptingPrintfOneLine("handlePowerManagementEvent: Missing event handler\n");
                return;
            }
            break;
        }
    }

    if (status != STATUS_SUCCESS)
    {
        interruptingPrintfOneLine("handlePowerManagementEvent: Unknown event.\n");
        return;
    }

    //Handle faults
    if (!(faults == RF_none || ((eventType & POWER_ALERT_EVENT) && !(faults & RF_overtemp))))
    {
        interruptingPrintfOneLine("Warning: Shutting power down\n");
        gpioDown(pps);
    }
}

static regulatorFail_t handlePerstEvent(
    powerManagerEventType_t eventType, uint32_t data, char const *name, powerState_t *pps)
{
    (void)eventType;
    (void)name;

    if (data == 0)
    {
        handlePerstDown(pps);
    }
    else
    {
        handlePerstUp(pps);
    }

    return RF_none;
}

static void handlePerstUp(powerState_t *pps)
{
    if (!globals.commonData.powerState.hwUp)
    {
        globals.commonData.powerState.deferPerstUp = true;
        printfFromInterruptNoFlush("handlePerstUp: hw not yet up, defer perst up and return", 0, 0);
        return;
    }

    if (globals.commonData.powerState.socPerstToggled)
    {
        printfFromInterruptNoFlush("handlePerstUp: perst is already toggled, ignore.", 0, 0);
        return;
    }

    //Drive PERST to ETSOC
    gpio_set_out_high_pin_id(SOC_PERST_OUT);
    globals.commonData.powerState.socPerstToggled = true;
    globals.commonData.powerState.deferPerstUp = false;
    printfFromInterruptNoFlush("handlePerstUp: MCU_PERST_OUT_N high", 0, 0);
}

static void handlePerstDown(powerState_t *pps)
{
    if (getPerstDownCount() == 0)
    {
        printfFromInterruptNoFlush("handlePerstDown: perst down count = 0, increment and return", 0, 0);
        incrementPerstDownCount();
        return;
    }

    if (!globals.commonData.powerState.hwUp)
    {
        printfFromInterruptNoFlush("handlePerstDown: hw not yet up, return", 0, 0);
        return;
    }

    if (globals.commonData.powerState.socPerstToggled)
    {
        printfFromInterruptNoFlush("handlePerstDown: perst is already toggled, report na error.", 0, 0);
        if (setAllRegulatorsToDefaultVoltage() != STATUS_SUCCESS)
        { // setting voltage fails on TI regulator if output isn't enabled
            interruptingPrintfOneLine("setAllRegulatorsToDefaultVoltage failed in falling PERST\n - Power down");
            gpioDown(pps);
            return;
        }
        //todo: send error to etsoc
    }
    else
    {
        //Drive PERST to ETSOC
        gpio_set_out_low_pin_id(SOC_PERST_OUT);
        printfFromInterruptNoFlush("handlePerstDown: MCU_PERST_OUT_N low", 0, 0);
    }
}

static regulatorFail_t handleWatchdogTimerEvent(
    powerManagerEventType_t eventType, uint32_t data, char const *name, powerState_t *pps)
{
    (void)name;
    //Set reset reason and toggle SOC_RESET_N
    setResetCauseCruSysReset();
    cmdResetSoc();
    return RF_none;
}

static regulatorFail_t handleVoltageMonitoringEvent(
    powerManagerEventType_t eventType, uint32_t data, char const *name, powerState_t *pps)
{
    (void)name;
    interruptingPrintfOneLine("Voltage monitoring: input voltage droop too low event interrupt (%d mV)", data);
    if (isInterruptEnabled(INTR_cmd_pwr_fail))
    {
        if (setInterruptCause(INTR_cmd_pwr_fail, data) == STATUS_SUCCESS)
        {
            triggerETSOCInterrupt();
        }
    }
    return RF_none;
}

static regulatorFail_t handlePowerMonitoringEvent(
    powerManagerEventType_t eventType, uint32_t data, char const *name, powerState_t *pps)
{
    (void)name;
    interruptingPrintfOneLine("Power monitoring: over power event interrupt (%d mW)", data);
    if (isInterruptEnabled(INTR_cmd_ov_pwr))
    {
        if (setInterruptCause(INTR_cmd_ov_pwr, data) == STATUS_SUCCESS)
        {
            triggerETSOCInterrupt();
        }
    }
    return RF_none;
}

static regulatorFail_t handleTemperatureMonitoringEvent(
    powerManagerEventType_t eventType, uint32_t data, char const *name, powerState_t *pps)
{
    (void)name;
    interruptingPrintfOneLine("Temperature monitoring: over temperature event interrupt");
    if (isInterruptEnabled(INTR_cmd_ov_temp))
    {
        if (setInterruptCause(INTR_cmd_ov_temp, data) == STATUS_SUCCESS)
        {
            triggerETSOCInterrupt();
        }
    }
    return RF_none;
}

static regulatorFail_t handleTemperatureAlert(
    powerManagerEventType_t eventType, uint32_t data, char const *name, powerState_t *pps)
{
    (void)name;
    interruptingPrintfOneLine("handleTEMP_ALERT_N not implemented");
    return RF_none; // TEMP_ALERT_N should be a fault - return RF_overtemp once implemented
}

static regulatorFail_t handle12VPowerGoodFailure(
    powerManagerEventType_t eventType, uint32_t data, char const *name, powerState_t *pps)
{
    interruptingPrintfOneLine("%s power fail/alarm.", name);
    globals.commonData.powerState.waiting12V = true;
    globals.blockPTFail = true;

    return RF_fault;
}

static regulatorFail_t handle3P3VPowerGoodFailure(
    powerManagerEventType_t eventType, uint32_t data, char const *name, powerState_t *pps)
{
    interruptingPrintfOneLine("%s power fail/alarm.", name);
    return RF_fault;
}

static regulatorFail_t handle1P8VPowerGoodFailure(
    powerManagerEventType_t eventType, uint32_t data, char const *name, powerState_t *pps)
{
    interruptingPrintfOneLine("%s power fail/alarm.", name);
    return RF_fault;
}

static regulatorFail_t handlePowerGoodFailure(
    powerManagerEventType_t eventType, uint32_t data, char const *name, powerState_t *pps)
{
    interruptingPrintfOneLine("%s power fail/alarm.", name);
    // todo: to be implemented
    return RF_fault;
}

static regulatorFail_t handleMNNOrNOCRegulatorAlert(
    powerManagerEventType_t eventType, uint32_t data, char const *name, powerState_t *pps)
{
    interruptingPrintfOneLine("%s regulator alert.", name);
    for (uint8_t page = 0; page < 2; ++page)
    {
        detailPmcStatus(&pmcDevs[E_Dev_MNN_NOC][page]);
    }
    return RF_none;
}

static regulatorFail_t handleSRAMRegulatorAlert(
    powerManagerEventType_t eventType, uint32_t data, char const *name, powerState_t *pps)
{
    interruptingPrintfOneLine("%s regulator alert.", name);
    for (uint8_t page = 0; page < 2; ++page)
    {
        detailPmcStatus(&pmcDevs[E_Dev_SRM][page]);
    }
    return RF_none;
}

static regulatorFail_t handleBUS0PowerGoodFailure(
    powerManagerEventType_t eventType, uint32_t data, char const *name, powerState_t *pps)
{
    regulatorFail_t faults = RF_none;
    interruptingPrintfOneLine("%s power fail/alarm.", name);
    faults |= handlePowerGoodFaults(E_Dev_MNN_NOC) |
              (RF_fault * (!(gpio_get_in_pin_id(POWER_GOOD_BUS0_IN) &
                              gpio_get_in_pin_id(POWER_GOOD_BUS1_IN)))); //todo: check why is this needed!
    return faults;
}

static regulatorFail_t handleBUS1PowerGoodFailure(
    powerManagerEventType_t eventType, uint32_t data, char const *name, powerState_t *pps)
{
    regulatorFail_t faults = RF_none;
    interruptingPrintfOneLine("%s power fail/alarm.", name);
    faults |= handlePowerGoodFaults(E_Dev_SRM) |
              (RF_fault * (!(gpio_get_in_pin_id(POWER_GOOD_BUS0_IN) &
                              gpio_get_in_pin_id(POWER_GOOD_BUS1_IN)))); //todo: check why is this needed!
    return faults;
}

static regulatorFail_t handleMNNPowerGoodFailure(
    powerManagerEventType_t eventType, uint32_t data, char const *name, powerState_t *pps)
{
    regulatorFail_t faults = RF_none;
    interruptingPrintfOneLine("%s power fail/alarm.", name);
    t_pmcDevs const *pDev = &pmcDevs[E_Dev_MNN_NOC][E_DevPage_0];
    vout_t *pv = findId(pDev->id);
    if (!pv->voutDisabled)
    {
        // since we don't know if the PG fail was a PMB device, check it
        faults |= detailPmcStatus(pDev); // fault on pmb device is real
    }
    return faults;
}

static regulatorFail_t handleNOCPowerGoodFailure(
    powerManagerEventType_t eventType, uint32_t data, char const *name, powerState_t *pps)
{
    regulatorFail_t faults = RF_none;
    interruptingPrintfOneLine("%s power fail/alarm.", name);
    t_pmcDevs const *pDev = &pmcDevs[E_Dev_MNN_NOC][E_DevPage_1];
    vout_t *pv = findId(pDev->id);
    if (!pv->voutDisabled)
    {
        // since we don't know if the PG fail was a PMB device, check it
        faults |= detailPmcStatus(pDev); // fault on pmb device is real
    }
    return faults;
}

static regulatorFail_t handleSRAMPowerGoodFailure(
    powerManagerEventType_t eventType, uint32_t data, char const *name, powerState_t *pps)
{
    interruptingPrintfOneLine("%s power fail/alarm.", name);
    return handlePowerGoodFaults(E_Dev_SRM);
}

static regulatorFail_t handlePowerGoodFaults(uint8_t device)
{
    assert1(device < 2, "pmcDevs index device is out of bounds.", device, 0);
    regulatorFail_t faults = RF_none;
    for (uint8_t page = 0; page < 2; ++page)
    {
        t_pmcDevs const *pDev = &pmcDevs[device][page];
        vout_t *pv = findId(pDev->id);
        if (!pv->voutDisabled)
        {
            // since we don't know if the PG fail was a PMB device, check it
            faults |= detailPmcStatus(pDev); // fault on pmb device is real
        }
        else
        {
            if (!anyVoutDisabled)
                faults |= RF_fault;
        }
    }
    return faults;
}

static void handleIoxpanderFailureEvent(powerManagerEventType_t ioxpanderEvent, powerState_t *pps)
{
    uint8_t iIoxp;
    switch (ioxpanderEvent)
    {
        case POWER_FAILURE_IOXPANDER_0:
            iIoxp = 0;
            break;
        case POWER_FAILURE_IOXPANDER_1:
            iIoxp = 1;
            break;
        default:
            printfFromInterrupt("Io expander power failure event id %d not supported.", ioxpanderEvent, 0);
            return;
    }

    uint16_t savedState = ioxpander_get_in_saved_state(iIoxp);
    uint16_t curState = ioxpander_read_in_state(iIoxp);
    uint16_t activeBits = ~curState & savedState; // low = pg fail

    for (uint8_t bit = 7, mask = 0x80; mask != 0; --bit, mask >>= 1)
    {
        if (mask & activeBits)
        {
            uint8_t ioxpBasePin = 0xFF;
            if (ioxpander_get_base_pin(iIoxp, &ioxpBasePin) != STATUS_SUCCESS)
            {
                printfFromInterrupt("Io expander index %d not supported.", iIoxp, 0);
                return;
            }
            uint8_t pin = ioxpBasePin + bit;
            powerManagerEventType_t eventType = getEventTypeForPinId(gpio_get_id_from_pin(pin));
            if (eventType == POWER_UNKNOWN_EVENT)
            {
                printfFromInterrupt("Pin %d not found.", pin, 0);
                assertMsg(0, "Pin not found");
            }

            handlePowerManagementEvent(eventType, 0, pps);
        }
    }
}

static void clearReguFaults(void)
{
    i2cmErr_t rv = 0;
    uint8_t addr;

    rv = i2cm_write0(structChipInfo[eID_TPS53681].addr7, PMBUS__CLEAR_FAULT);
    if (rv != I2CM_OK)
    {
        i2cPError(rv, "write PMBUS__CLEAR_FAULT");
    }

    switch (Hw_Encoding_Get_Hw_Version_Encoding())
    {
        case HW_ENCODING_PCIE_1088: //Penguin RevB
            addr = structChipInfo[eID_TPSM8D6C24].addr7;
            break;
        default:
            addr = structChipInfo[eID_LTM4680].addr7;
            break;
    }
    rv = i2cm_write0(addr, PMBUS__CLEAR_FAULT);
    if (rv != I2CM_OK)
    {
        i2cPError(rv, "write PMBUS__CLEAR_FAULT");
    }
}

static uint8_t detailPmcStatus(t_pmcDevs const *pDev)
{
    if (globals.blockPTFail)
    {
        return 0;
    }

    i2cmErr_t rv = 0;
    uint16_t data0;
    uint8_t data1;
    uint8_t data2;
    uint8_t faults = 0;
    uint8_t addr = structChipInfo[pDev->eID].addr7;
    char const *name = pDev->name;

    // TODO: Currently only registers TPS53681 are used even though the error could be from
    // either regulator. It so happens the register offset are the same. We should move to a
    // more generic option
    rv = i2cm_read16(addr, PMBUS__STATUS_WORD, &data0);
    if (rv != I2CM_OK)
    {
        i2cPError(rv, "I2C ERROR:pmc__STATUS_WORD");
        return faults;
    }
    interruptingPrintf("%s pmc__STATUS_WORD data0 %02X\n", name, data0);

    if (data0 & (PMBUS__STATUS_WORD__VOUT__MASK << PMBUS__STATUS_WORD__VOUT__SHIFT))
    {
        rv = i2cm_read8(addr, PMBUS__STATUS_VOUT, &data1);
        if (rv != I2CM_OK)
        {
            i2cPError(rv, "I2C ERROR:pmc__STATUS_VOUT");
            return faults;
        }
        rv = i2cm_write8(addr, PMBUS__STATUS_VOUT, data1); // write to clear, then see if it's still there
        if (rv != I2CM_OK)
        {
            i2cPError(rv, "I2C ERROR:pmc__STATUS_VOUT");
            return faults;
        }
        interruptingPrintf("%s pmc__STATUS_VOUT data1 #1 %02X\n", name, data1);
        rv = i2cm_read8(addr, PMBUS__STATUS_VOUT, &data2);
        if (rv != I2CM_OK)
        {
            i2cPError(rv, "I2C ERROR:pmc__STATUS_VOUT");
            return faults;
        }
        interruptingPrintf("pmc__STATUS_VOUT data1 #2 %02X\n", data1);
        for (uint8_t i = 0; i < NUM_VOUT_ERROR_TYPES; ++i)
        {
            static char const *pnames[NUM_VOUT_ERROR_TYPES] = { "VOUT_OVF", "", "", "VOUT_UVF", "", "T_ON", "T_OFF" };
            if ((data1 & (PMBUS__STATUS_VOUT__VOUT_OVF__MASK << (PMBUS__STATUS_VOUT__VOUT_OVF__SHIFT - i))) &&
                pnames[i])
            {
                interruptingPrintf(" - %s VOUT Fault %s\n", name, pnames[i]);
                faults |= RF_fault;
            }
        }
        rv = i2cm_write8(addr, PMBUS__STATUS_VOUT, data1);
        if (rv != I2CM_OK)
        {
            i2cPError(rv, "I2C ERROR:write pmc__STATUS_VOUT"); // clear flags
            return faults;
        }
    }

    if (data0 & (PMBUS__STATUS_WORD__IOUT__MASK << PMBUS__STATUS_WORD__IOUT__SHIFT))
    {
        rv = i2cm_read8(addr, PMBUS__STATUS_IOUT, &data1);
        if (rv != I2CM_OK)
        {
            i2cPError(rv, "I2C ERROR:pmc__STATUS_IOUT");
            return faults;
        }
        rv = i2cm_write8(addr, PMBUS__STATUS_IOUT, data1); // write to clear, then see if it's still there
        if (rv != I2CM_OK)
        {
            i2cPError(rv, "I2C ERROR:pmc__STATUS_IOUT");
            return faults;
        }
        interruptingPrintf("%s pmc__STATUS_IOUT data1 #1 %02X\n", name, data1);
        rv = i2cm_read8(addr, PMBUS__STATUS_IOUT, &data2);
        if (rv != I2CM_OK)
        {
            i2cPError(rv, "I2C ERROR:pmc__STATUS_IOUT");
            return faults;
        }
        interruptingPrintf("pmc__STATUS_IOUT data1 #2 %02X\n", data1);
        for (uint8_t i = 0; i < NUM_CUR_ERROR_TYPES; ++i)
        {
            static char const *pnames[NUM_CUR_ERROR_TYPES] = { "IOUT_OCF", "", "", "", "CUR_SHAREF" };
            if ((data1 & (PMBUS__STATUS_IOUT__IOUT_OCF__MASK << (PMBUS__STATUS_IOUT__IOUT_OCF__SHIFT - i))) &&
                pnames[i])
            {
                interruptingPrintf(" - %s IOUT Fault %s\n", name, pnames[i]);
                faults |= RF_fault;
            }
        }
        rv = i2cm_write8(addr, PMBUS__STATUS_IOUT, data1);
        if (rv != I2CM_OK)
        {
            i2cPError(rv, "I2C ERROR:write pmc__STATUS_IOUT"); // clear flags
            return faults;
        }
    }

    if (data0 & (PMBUS__STATUS_WORD__INPUT__MASK << PMBUS__STATUS_WORD__INPUT__SHIFT))
    {
        rv = i2cm_read8(addr, PMBUS__STATUS_INPUT, &data1);
        if (rv != I2CM_OK)
        {
            i2cPError(rv, "I2C ERROR:pmc__STATUS_INPUT");
        }
        rv = i2cm_write8(addr, PMBUS__STATUS_INPUT, data1); // write to clear, then see if it's still there
        if (rv != I2CM_OK)
        {
            i2cPError(rv, "I2C ERROR:pmc__STATUS_INPUT");
        }
        interruptingPrintf("%s pmc__STATUS_INPUT data1 #1 %02X\n", name, data1);
        rv = i2cm_read8(addr, PMBUS__STATUS_INPUT, &data2);
        if (rv != I2CM_OK)
        {
            i2cPError(rv, "I2C ERROR:pmc__STATUS_INPUT");
            return faults;
        }
        interruptingPrintf("%s PMBUS__STATUS_INPUT data1 #2 %02X\n", name, data1);
        for (uint8_t i = 0; i < NUM_VIN_ERROR_TYPES; ++i)
        {
            static char const *pnames[NUM_VIN_ERROR_TYPES] = { "VIN_OVF", "", "", "VIN_UVF", "LOW_VIN", "IIN_OCF" };
            if ((data1 & (PMBUS__STATUS_INPUT__VIN_OVF__MASK << (PMBUS__STATUS_INPUT__VIN_OVF__SHIFT - i))) &&
                pnames[i])
            {
                interruptingPrintf(" - %s VIN Fault %s\n", name, pnames[i]);
                faults |= RF_fault;
            }
        }
        rv = i2cm_write8(addr, PMBUS__STATUS_INPUT, data1);
        if (rv != I2CM_OK)
        {
            i2cPError(rv, "I2C ERROR:write PMBUS__STATUS_INPUT"); // clear flags
            return faults;
        }
    }

    if (data0 & (TPS53681__STATUS_WORD__MFR__MASK << TPS53681__STATUS_WORD__MFR__SHIFT))
    {
        rv = i2cm_read8(addr, TPS53681__STATUS_MFR_SPECIFIC, &data1);
        if (rv != I2CM_OK)
        {
            i2cPError(rv, "I2C ERROR:pmc__STATUS_MFR_SPECIFIC");
        }
        rv = i2cm_write8(addr, TPS53681__STATUS_MFR_SPECIFIC,
            data1); // write to clear, then see if it's still there
        if (rv != I2CM_OK)
        {
            i2cPError(rv, "I2C ERROR:pmc__STATUS_MFR_SPECIFIC");
        }
        interruptingPrintf("%s pmc__STATUS_MFR_SPECIFIC data1 #1 %02X\n", name, data1);
        rv = i2cm_read8(addr, TPS53681__STATUS_MFR_SPECIFIC, &data2);
        if (rv != I2CM_OK)
        {
            i2cPError(rv, "I2C ERROR:pmc__STATUS_MFR_SPECIFIC");
            return faults;
        }
        interruptingPrintf("%s pmc__STATUS_MFR_SPECIFIC data1 #2 %02X\n", name, data1);
        for (uint8_t i = 0; i < NUM_MFR_ERROR_TYPES; ++i)
        {
            static char const *const pnames[MFR_ERROR_GROUP][NUM_MFR_ERROR_TYPES] = {
                { "MFR_FAULT_PS", "VSNS_OPEN", "", "TSNS_LOW", "", "", "", "PHFLT" },
                { "INTTEMPR_FAULT", "INTTEMPR_WARN", "TRIM_CRC", "PLL_UNLOCK", "LOG_PRSNT", "VDD3P3", "SHORTCYCLE",
                    "FAULT_PIN" },
            };
            if ((data1 & (TPS53681__STATUS_MFR_SPECIFIC__MFR_FAULT_PS__MASK
                             << (TPS53681__STATUS_MFR_SPECIFIC__MFR_FAULT_PS__SHIFT - i))) &&
                pnames[0][i])
            {
                interruptingPrintf(" - %s MFR_SPECIFIC Fault %s\n", name, pnames[name[0] == 'S'][i]);
                faults |= RF_fault;
            }
        }
        rv = i2cm_write8(addr, TPS53681__STATUS_MFR_SPECIFIC, data1);
        if (rv != I2CM_OK)
        {
            i2cPError(rv, "I2C ERROR:write pmc__STATUS_MFR_SPECIFIC"); // clear flags
            return faults;
        }
    }

    if (data0 & (PMBUS__STATUS_WORD__TEMP__MASK << PMBUS__STATUS_WORD__TEMP__SHIFT))
    {
        rv = i2cm_read8(addr, PMBUS__STATUS_TEMPERATURE, &data1);
        if (rv != I2CM_OK)
        {
            i2cPError(rv, "I2C ERROR:pmc__STATUS_TEMPERATURE");
        }
        rv = i2cm_write8(addr, PMBUS__STATUS_TEMPERATURE, data1); // write to clear, then see if it's still there
        if (rv != I2CM_OK)
        {
            i2cPError(rv, "I2C ERROR:pmc__STATUS_TEMPERATURE");
        }
        interruptingPrintf("%s pmc__STATUS_TEMPERATURE data1 #1 %02X\n", name, data1);
        rv = i2cm_read8(addr, PMBUS__STATUS_TEMPERATURE, &data2);
        if (rv != I2CM_OK)
        {
            i2cPError(rv, "I2C ERROR:pmc__STATUS_TEMPERATURE");
            return faults;
        }
        interruptingPrintf("pmc__STATUS_TEMPERATURE data1 #2 %02X\n", data1);
        static char const *pnames[NUM_TEMP_ERROR_TYPES] = { "OTF" };
        if ((data1 & (PMBUS__STATUS_TEMPERATURE__OTF__MASK << (PMBUS__STATUS_TEMPERATURE__OTF__SHIFT))) && pnames[0])
        {
            faults |= RF_fault | RF_overtemp;
            interruptingPrintf(" - %s TEMPERATURE Fault %s\n", name, pnames[0]);
        }
        rv = i2cm_write8(addr, PMBUS__STATUS_TEMPERATURE, data1);
        if (rv != I2CM_OK)
        {
            i2cPError(rv, "I2C ERROR:write pmc__STATUS_TEMPERATURE"); // clear flags
            return faults;
        }
    }

    if (data0 & (PMBUS__STATUS_WORD__CML__MASK << PMBUS__STATUS_WORD__CML__SHIFT))
    {
        rv = i2cm_read8(addr, PMBUS__STATUS_CML, &data1);
        if (rv != I2CM_OK)
        {
            i2cPError(rv, "pmc__STATUS_CML Read");
            return faults;
        }
        rv = i2cm_write8(addr, PMBUS__STATUS_CML, data1); // write to clear, then see if it's still there
        if (rv != I2CM_OK)
        {
            i2cPError(rv, "pmc__STATUS_CML Write");
            return faults;
        }
        interruptingPrintf("%s pmc__STATUS_CML data1 #1 %02X\n", name, data1);
        rv = i2cm_read8(addr, PMBUS__STATUS_CML, &data2);
        if (rv != I2CM_OK)
        {
            i2cPError(rv, "pmc__STATUS_CML Read");
            return faults;
        }
        interruptingPrintf("pmc__STATUS_CML data1 #2 %02X\n", data1);
        for (uint8_t i = 0; i < NUM_CML_ERROR_TYPES; ++i)
        {
            static char const *pnames[NUM_CML_ERROR_TYPES] = {
                "Bad Cmd",
                "Bad Data",
                "Bad Cksum",
                "Mem Fault",
                "Proc Fault",
                "Reserved",
                "Other Fault",
                "Other Proc/Mem Fault",
            };
            if ((data1 & (PMBUS__STATUS_TEMPERATURE__OTF__MASK << (PMBUS__STATUS_CML__IV_CMD__SHIFT - i))) && pnames[i])
            {
                interruptingPrintf(" - %s CML Fault %s\n", name, pnames[i]);
            }
        }
        rv = i2cm_write8(addr, PMBUS__STATUS_CML, data1);
        if (rv != I2CM_OK)
        {
            i2cPError(rv, "write pmc__STATUS_CML"); // clear flags
            return faults;
        }
    }

    return faults;
}

static bool checkRegulatorPgOnPgBus(powerState_t const *pps)
{
    uint64_t t = getTimer2();
    bool ok0 = false;
    bool ok1 = false;
    recordTime("Before PG");
    do
    {
        ok0 = gpio_get_in_pin_id(POWER_GOOD_BUS0_IN);
        ok1 = gpio_get_in_pin_id(POWER_GOOD_BUS1_IN);
    } while (!(ok0 && ok1) && getTimer2() < t + CPU_FREQUENCY); //1 sec timeout
    recordTime("After PG");

    if (!ok0)
    {
        interruptingPrintfOneLine("PG_BUS0 bad");
        // we don't know if bad PG for NOC or MNN, so always check them
        detailPmcStatus(&pmcDevs[E_Dev_MNN_NOC][E_DevPage_0]);
        detailPmcStatus(&pmcDevs[E_Dev_MNN_NOC][E_DevPage_1]);
    }

    if (!ok1)
    {
        interruptingPrintfOneLine("PG_BUS1 bad");
        // we don't know if bad PG for SRM, so always check both halves
        detailPmcStatus(&pmcDevs[E_Dev_SRM][E_DevPage_0]);
        detailPmcStatus(&pmcDevs[E_Dev_SRM][E_DevPage_1]);
    }

    return ((ok0 && ok1) || pps->skipReguCheck);
}

static bool checkRegulatorPgOnIoXp(void)
{
    uint64_t t = getTimer2();
    uint32_t failedPgBitField;
    int j;
    voutInfo_t const *p;
    vout_t *pv;
    recordTime("Before PG");
    do
    {
        ioxpander_update_all_input_state();
        failedPgBitField = 0;
        for (j = 1, pv = vout; (p = pv->voutInfo); ++pv, j <<= 1)
        {
            uint8_t pgPin = p->id ? gpio_get_pg_pin_from_en(p->enablePinId) :
                                    gpio_get_pin_from_id(POWER_GOOD_1P8_IN); //TODO: refactor
            if (!ioxpander_get_in_saved(pgPin) && !pv->voutDisabled)
            {
                failedPgBitField |= j;
            }
        }
    } while (failedPgBitField != 0 && getTimer2() < t + CPU_FREQUENCY); // 1 sec timeout

    recordTime("After PG");

    if (failedPgBitField != 0)
    {
        interruptingPrintBegin();
        interruptingPrintf("Bad PG(s): ");
        bool frst = true;
        for (j = 1, pv = vout; (p = pv->voutInfo); ++pv, j <<= 1)
        {
            if ((j & failedPgBitField) != 0)
            {
                if (!frst)
                {
                    interruptingPrintf(", ");
                }
                frst = 0;
                interruptingPrintf("%s", p->id ? p->id : "P1P8V");
            }
        }
        interruptingPrintf("\n");
        interruptingPrintEnd();
    }
    for (j = 1, pv = vout; (p = pv->voutInfo); ++pv, j <<= 1)
    {
        if ((j & failedPgBitField) == 0)
        {
            continue;
        }

        switch (p->enablePinId)
        {
            case MNN_EN_OUT:
                detailPmcStatus(&pmcDevs[E_Dev_MNN_NOC][E_DevPage_0]);
                break;
            case NOC_EN_OUT:
                detailPmcStatus(&pmcDevs[E_Dev_MNN_NOC][E_DevPage_1]);
                break;
            case SRAM_EN_OUT:
                detailPmcStatus(&pmcDevs[E_Dev_SRM][E_DevPage_0]);
                detailPmcStatus(&pmcDevs[E_Dev_SRM][E_DevPage_1]);
                break;
            default:
                break;
        }
    }
    return failedPgBitField == 0;
}

static bool checkRegulatorPg(powerState_t const *pps)
{
    if (isFeaturePerHwEncodingEnabled(POWER_GOOD_ON_IOXPANDER_PRESENT))
    {
        return checkRegulatorPgOnIoXp();
    }
    else
    {
        return checkRegulatorPgOnPgBus(pps);
    }
}
