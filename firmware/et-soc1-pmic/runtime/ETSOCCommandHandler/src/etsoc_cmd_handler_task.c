/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file etsoc_cmd_handler_task.c
    \brief C file for PMIC control from ET-SOC.
*/
/***********************************************************************/

#include "FreeRTOS.h"
#include "etsoc_cmd_handler_task.h"
#include "etsoc_cmd_handler_event.h"
#include "gpio.h"
#include "cli.h"
#include "uart.h"
#include "i2cs.h"
#include "utils.h"
#include "i2cm.h"
#include "adc.h"
#include "bootimage.h"
#include "CLITask.h"
#include "vreg.h"
#include "iostructs.h"
#include "IoTask.h"
#include "boardchipinfo.h"
#include "commitinfo.h"
#include "pmbstats.h"
#include "error_codes.h"
#include "image_metadata.h"
#include "hw_encoding.h"
#include "fruinfo.h"

/***********************************************************************
 * Data types:      Defininition of local data types
***********************************************************************/
enum { RE_ok = 0, RE_i2c, RE_outOfRange };

typedef enum {
    RO,  //read only
    WO,  //write only
    RW,  //read/write
    ROC, //read only with auto clear after read
} spCmdAccessType_t;

typedef enum {
    NUM_OF_BYTES_0 = 0,
    NUM_OF_BYTES_1 = 1,
    NUM_OF_BYTES_2 = 2,
    NUM_OF_BYTES_4 = 4,
} spCmdNumberOfBytes_t;

typedef struct {
    spCommandIndex_t cmdIdx;
    const char *name;
    spCmdNumberOfBytes_t nbytes;
    spCmdAccessType_t spOps;
    uint32_t init;
    uint32_t min;
    uint32_t max;
    int (*const doWrCmd)(ETSOCVirtualRegisterIdx_t regIdx, uint32_t data);
    size_t regOffset;
} SpItem_t;

typedef struct {
    uint32_t firmwareVersion;
    uint32_t boardType;
    uint32_t regulatorFault;
    uint32_t reguCommFail;
    uint32_t cmdCommFail;
    uint32_t pmbStats;
    uint32_t updateData;
    uint16_t powerIn;
    uint16_t averagePower;
    uint8_t voltageIn;
    uint8_t systemTemp;
    uint8_t powerAlarm;
    uint8_t tempAlarm;
    uint8_t intConfig;
    uint32_t intCause;
    uint8_t watchdogConfig;
    uint8_t watchdogReset;
    uint8_t resetCause;
    uint8_t resetCommand;
    uint8_t resetControl;
    uint8_t GPIOConfig;
    uint8_t GPIOControl;
    uint8_t updateCmd;
    uint8_t DDQLPVoltage;
    uint8_t L2CacheVoltage;
    uint8_t DDRVoltage;
    uint8_t DDQVoltage;
    uint8_t PCIeLogicVoltage;
    uint8_t PCIeVoltage;
    uint8_t MaxionVoltage;
    uint8_t NOCVoltage;
    uint8_t allMinionVoltage;
    uint8_t minionG1Voltage;
    uint8_t minionG2Voltage;
    uint8_t minionG3Voltage;
    uint8_t minionG4Voltage;
    uint8_t minionG5Voltage;
    uint8_t minionG6Voltage;
    uint8_t minionG7Voltage;
    uint8_t minionG8Voltage;
    uint8_t minionG9Voltage;
    uint8_t minionG10Voltage;
    uint8_t minionG11Voltage;
    uint8_t minionG12Voltage;
    uint8_t minionG13Voltage;
    uint8_t minionG14Voltage;
    uint8_t minionG15Voltage;
    uint8_t minionG16Voltage;
    uint8_t minionG17Voltage;
    uint8_t fruOpsCmd;
    uint8_t fruOpsData;
} spCmdVirtualRegisterStorage_t;

typedef struct {
    ETSOCVirtualRegisterIdx_t regIdx;
    uint16_t base_mV;
    uint16_t delta_uV;
} regulatorCodeConversionInfo_t;

/***********************************************************************
 * Functions:   Declaration of local functions used as func pointers
***********************************************************************/
static void commandNotImplemented(ETSOCVirtualRegisterIdx_t regIdx,
    uint32_t data); //TODO: SW-15170 to be removed when all commands are implemented

static int setPowerAlarm(ETSOCVirtualRegisterIdx_t regIdx, uint32_t data);
static int setTemperatureAlarm(ETSOCVirtualRegisterIdx_t regIdx, uint32_t data);
static int setIntrCfg(ETSOCVirtualRegisterIdx_t regIdx, uint32_t data);
static int doWdt(ETSOCVirtualRegisterIdx_t regIdx, uint32_t data);
static int resetWdt(ETSOCVirtualRegisterIdx_t regIdx, uint32_t data);
static int doReset(ETSOCVirtualRegisterIdx_t regIdx, uint32_t data);
static int setRstCtrl(ETSOCVirtualRegisterIdx_t regIdx, uint32_t data);
static int setGpioCfg(ETSOCVirtualRegisterIdx_t regIdx, uint32_t data);
static int setGetGpio(ETSOCVirtualRegisterIdx_t regIdx, uint32_t data);
static int wrPmbStats(ETSOCVirtualRegisterIdx_t regIdx, uint32_t data);
static int doFwUpdateCmd(ETSOCVirtualRegisterIdx_t regIdx, uint32_t data);
static int doFwUpdateData(ETSOCVirtualRegisterIdx_t regIdx, uint32_t data);
static int doFruOpsCmd(ETSOCVirtualRegisterIdx_t regIdx, uint32_t data);
static int doFruOpsData(ETSOCVirtualRegisterIdx_t regIdx, uint32_t data);

/***********************************************************************
 * Variables:  Defininition of local variables and constants
***********************************************************************/
pmbStats_t pmbStatsSnapshot;
uint8_t pmbStatsReadDevc = 0;
uint8_t pmbStatsReadReg = 0;
uint8_t pmbStatsReadStat = 0;

spCmdVirtualRegisterStorage_t volatile spCmdVirtualRegisterStorage;

// clang-format off
#define CMD_REG_OFFSET(x) (offsetof(spCmdVirtualRegisterStorage_t, x))

//todo:
// - add all doWrCmd functions

SpItem_t const spCmd[NUMBER_OF_REGISTERS] = {
    [RESERVED]            = {},
    [FIRMWARE_VERSION]    = {.cmdIdx = firmwareVersionSpCmd,  .name = "fwversion RO",        .regOffset = CMD_REG_OFFSET(firmwareVersion),  .nbytes = NUM_OF_BYTES_4, .spOps = RO,  .doWrCmd = NULL,          .init = 0,                       .min = 0,                  .max = UINT32_MAX },
    [BOARD_TYPE]          = {.cmdIdx = boardTypeSpCmd,        .name = "board RO",            .regOffset = CMD_REG_OFFSET(boardType),        .nbytes = NUM_OF_BYTES_4, .spOps = RO,  .doWrCmd = NULL,          .init = 0,                       .min = 0,                  .max = UINT32_MAX },
    [VOLTAGE_IN]          = {.cmdIdx = voltageInSpCmd,        .name = "voltagein RO",        .regOffset = CMD_REG_OFFSET(voltageIn),        .nbytes = NUM_OF_BYTES_1, .spOps = RO,  .doWrCmd = NULL,          .init = 0,                       .min = 0,                  .max = UINT8_MAX },
    [POWER_IN]            = {.cmdIdx = powerInSpCmd,          .name = "powerin RO",          .regOffset = CMD_REG_OFFSET(powerIn),          .nbytes = NUM_OF_BYTES_2, .spOps = RO,  .doWrCmd = NULL,          .init = 0,                       .min = 0,                  .max = UINT16_MAX },
    [AVERAGE_POWER]       = {.cmdIdx = averagePowerSpCmd,     .name = "averagepower RO",     .regOffset = CMD_REG_OFFSET(averagePower),     .nbytes = NUM_OF_BYTES_2, .spOps = RO,  .doWrCmd = NULL,          .init = 0,                       .min = 0,                  .max = UINT16_MAX },
    [SYSTEM_TEMPERATURE]  = {.cmdIdx = systemTempSpCmd,       .name = "systemtemp RO",       .regOffset = CMD_REG_OFFSET(systemTemp),       .nbytes = NUM_OF_BYTES_1, .spOps = RO,  .doWrCmd = NULL,          .init = 0,                       .min = SYS_TEMP_MIN_VAL,   .max = SYS_TEMP_MAX_VAL },
    [POWER_ALARM]         = {.cmdIdx = powerAlarmSpCmd,       .name = "poweralarm RW",       .regOffset = CMD_REG_OFFSET(powerAlarm),       .nbytes = NUM_OF_BYTES_1, .spOps = RW,  .doWrCmd = &setPowerAlarm,       .init = POWER_ALARM_DEFAULT_VAL, .min = 0,                  .max = UINT8_MAX },
    [TEMPERATURE_ALARM]   = {.cmdIdx = tempAlarmSpCmd,        .name = "tempalarm RW",        .regOffset = CMD_REG_OFFSET(tempAlarm),        .nbytes = NUM_OF_BYTES_1, .spOps = RW,  .doWrCmd = &setTemperatureAlarm, .init = TEMP_ALARM_DEFAULT_VAL,  .min = TEMP_ALARM_MIN_VAL, .max = TEMP_ALARM_MAX_VAL },
    [INTERRUPT_CONF]      = {.cmdIdx = intConfigSpCmd,        .name = "intconfig RW",        .regOffset = CMD_REG_OFFSET(intConfig),        .nbytes = NUM_OF_BYTES_1, .spOps = RW,  .doWrCmd = &setIntrCfg,          .init = INT_CONFIG_DEFAULT_VAL,  .min = 0,                  .max = UINT8_MAX },
    [INTERRUPT_CAUSE]     = {.cmdIdx = intCauseSpCmd,         .name = "intcause ROC",        .regOffset = CMD_REG_OFFSET(intCause),         .nbytes = NUM_OF_BYTES_4, .spOps = ROC, .doWrCmd = NULL,          .init = 0,                       .min = 0,                  .max = UINT32_MAX },
    [REGULATOR_FAULT]     = {.cmdIdx = regulatorFaultSpCmd,   .name = "regufault ROC",       .regOffset = CMD_REG_OFFSET(regulatorFault),   .nbytes = NUM_OF_BYTES_4, .spOps = ROC, .doWrCmd = NULL,          .init = 0,                       .min = 0,                  .max = REGU_FAULT_MAX_VAL },
    [REGULATOR_COMM_FAIL] = {.cmdIdx = reguCommFailSpCmd,     .name = "regucommfail ROC",    .regOffset = CMD_REG_OFFSET(reguCommFail),     .nbytes = NUM_OF_BYTES_4, .spOps = ROC, .doWrCmd = NULL,          .init = 0,                       .min = 0,                  .max = REGU_COMM_FAIL_MAX_VAL },
    [ETSOC_COMM_FAIL]     = {.cmdIdx = cmdCommFailSpCmd,      .name = "commandfault ROC",    .regOffset = CMD_REG_OFFSET(cmdCommFail),      .nbytes = NUM_OF_BYTES_4, .spOps = ROC, .doWrCmd = NULL,          .init = 0,                       .min = 0,                  .max = UINT32_MAX },
    [WATCHDOG_CONF]       = {.cmdIdx = watchdogConfigSpCmd,   .name = "watchdogconfig RW",   .regOffset = CMD_REG_OFFSET(watchdogConfig),   .nbytes = NUM_OF_BYTES_1, .spOps = RW,  .doWrCmd = &doWdt,        .init = 0,                       .min = 0,                  .max = WATCHDOG_CONFIG_MAX_VAL }, //TODO: SW-15170 cmd not yet implemented
    [WATCHDOG_RESET]      = {.cmdIdx = watchdogResetSpCmd,    .name = "watchdogreset WO",    .regOffset = CMD_REG_OFFSET(watchdogReset),    .nbytes = NUM_OF_BYTES_0, .spOps = WO,  .doWrCmd = &resetWdt,     .init = 0,                       .min = 0,                  .max = 1 }, //TODO: SW-15170 cmd not yet implemented
    [RESET_CAUSE]         = {.cmdIdx = resetCauseSpCmd,       .name = "resetcause ROC",      .regOffset = CMD_REG_OFFSET(resetCause),       .nbytes = NUM_OF_BYTES_1, .spOps = ROC, .doWrCmd = NULL,          .init = RST_cause_no_rst,        .min = RST_cause_no_rst,   .max = RST_cause_ovrtempr_pwrcycle },
    [RESET_CMD]           = {.cmdIdx = resetCommandSpCmd,     .name = "resetcommand RW",     .regOffset = CMD_REG_OFFSET(resetCommand),     .nbytes = NUM_OF_BYTES_1, .spOps = RW,  .doWrCmd = &doReset,      .init = RST_cmd_none,            .min = RST_cmd_none,       .max = RST_cmd_golden },
    [RESET_CONTROL]       = {.cmdIdx = resetControlSpCmd,     .name = "resetcontrol WO",     .regOffset = CMD_REG_OFFSET(resetControl),     .nbytes = NUM_OF_BYTES_1, .spOps = WO,  .doWrCmd = &setRstCtrl,   .init = 0,                       .min = 0,                  .max = RST_CTRL_MAX_VAL }, //TODO: SW-15170 cmd not yet implemented
    [GPIO_CONF]           = {.cmdIdx = GPIOConfigSpCmd,       .name = "gpioconfig RW",       .regOffset = CMD_REG_OFFSET(GPIOConfig),       .nbytes = NUM_OF_BYTES_1, .spOps = RW,  .doWrCmd = &setGpioCfg,   .init = 0,                       .min = 0,                  .max = UINT8_MAX }, //TODO: SW-15170 cmd not yet implemented
    [GPIO_CONTROL]        = {.cmdIdx = GPIOControlSpCmd,      .name = "gpiocontrol RW",      .regOffset = CMD_REG_OFFSET(GPIOControl),      .nbytes = NUM_OF_BYTES_1, .spOps = RW,  .doWrCmd = &setGetGpio,   .init = 0,                       .min = 0,                  .max = UINT8_MAX }, //TODO: SW-15170 cmd not yet implemented
    [PMB_STATS]           = {.cmdIdx = pmbStatsSpCmd,         .name = "pmbstats RW",         .regOffset = CMD_REG_OFFSET(pmbStats),         .nbytes = NUM_OF_BYTES_4, .spOps = RW,  .doWrCmd = &wrPmbStats,   .init = 0,                       .min = 0,                  .max = UINT32_MAX },
    [FW_UPDATE_CMD]       = {.cmdIdx = updateCmdSpCmd,        .name = "fwupdatecmd RW",      .regOffset = CMD_REG_OFFSET(updateCmd),        .nbytes = NUM_OF_BYTES_1, .spOps = RW,  .doWrCmd = &doFwUpdateCmd, .init = 0,                      .min = 0,                  .max = UINT8_MAX },
    [FW_UPDATE_DATA]      = {.cmdIdx = updateDataSpCmd,       .name = "fwupdatedata RW",     .regOffset = CMD_REG_OFFSET(updateData),       .nbytes = NUM_OF_BYTES_4, .spOps = RW,  .doWrCmd = &doFwUpdateData,.init = 0,                      .min = 0,                  .max = UINT32_MAX },
    [DDQLP_VOLTAGE]       = {.cmdIdx = DDQLPVoltageSpCmd,     .name = "qlpvoltage RW",       .regOffset = CMD_REG_OFFSET(DDQLPVoltage),     .nbytes = NUM_OF_BYTES_1, .spOps = RW,  .doWrCmd = &writeRegulatorReg, .init = QLP_DEFAULT_VAL,         .min = QLP_MIN_VAL,        .max = QLP_MAX_VAL },
    [L2_CACHE_VOLTAGE]    = {.cmdIdx = L2CacheVoltageSpCmd,   .name = "l2cachevoltage RW",   .regOffset = CMD_REG_OFFSET(L2CacheVoltage),   .nbytes = NUM_OF_BYTES_1, .spOps = RW,  .doWrCmd = &writeRegulatorReg, .init = SRM_DEFAULT_VAL,         .min = SRM_MIN_VAL,        .max = SRM_MAX_VAL },
    [DDR_VOLTAGE]         = {.cmdIdx = DDRVoltageSpCmd,       .name = "ddrvoltage RW",       .regOffset = CMD_REG_OFFSET(DDRVoltage),       .nbytes = NUM_OF_BYTES_1, .spOps = RW,  .doWrCmd = &writeRegulatorReg, .init = DDR_DEFAULT_VAL,         .min = DDR_MIN_VAL,        .max = DDR_MAX_VAL },
    [DDQ_VOLTAGE]         = {.cmdIdx = DDQVoltageSpCmd,       .name = "ddqvoltage RW",       .regOffset = CMD_REG_OFFSET(DDQVoltage),       .nbytes = NUM_OF_BYTES_1, .spOps = RW,  .doWrCmd = &writeRegulatorReg, .init = DDQ_DEFAULT_VAL,         .min = DDQ_MIN_VAL,        .max = DDQ_MAX_VAL },
    [PCIE_LOGIC_VOLTAGE]  = {.cmdIdx = PCIeLogicVoltageSpCmd, .name = "pcielogicvoltage RW", .regOffset = CMD_REG_OFFSET(PCIeLogicVoltage), .nbytes = NUM_OF_BYTES_1, .spOps = RW,  .doWrCmd = &writeRegulatorReg, .init = PCL_DEFAULT_VAL,         .min = PCL_MIN_VAL,        .max = PCL_MAX_VAL },
    [PCIE_VOLTAGE]        = {.cmdIdx = PCIeVoltageSpCmd,      .name = "pcievoltage RW",      .regOffset = CMD_REG_OFFSET(PCIeVoltage),      .nbytes = NUM_OF_BYTES_1, .spOps = RW,  .doWrCmd = &writeRegulatorReg, .init = PCI_DEFAULT_VAL,         .min = PCI_MIN_VAL,        .max = PCI_MAX_VAL },
    [MAXION_VOLTAGE]      = {.cmdIdx = maxionVoltageSpCmd,    .name = "maxionvoltage RW",    .regOffset = CMD_REG_OFFSET(MaxionVoltage),    .nbytes = NUM_OF_BYTES_1, .spOps = RW,  .doWrCmd = &writeRegulatorReg, .init = MXN_DEFAULT_VAL,         .min = MXN_MIN_VAL,        .max = MXN_MAX_VAL },
    [NOC_VOLTAGE]         = {.cmdIdx = NOCVoltageSpCmd,       .name = "nocvoltage RW",       .regOffset = CMD_REG_OFFSET(NOCVoltage),       .nbytes = NUM_OF_BYTES_1, .spOps = RW,  .doWrCmd = &writeRegulatorReg, .init = NOC_DEFAULT_VAL,         .min = NOC_MIN_VAL,        .max = NOC_MAX_VAL },
    [ALL_MINION_VOLTAGE]  = {.cmdIdx = allMinionVoltageSpCmd, .name = "allminionvoltage RW", .regOffset = CMD_REG_OFFSET(allMinionVoltage), .nbytes = NUM_OF_BYTES_1, .spOps = RW,  .doWrCmd = &writeRegulatorReg/*todo setAllMinionVoltage*/, .init = MNN_DEFAULT_VAL,         .min = MNN_MIN_VAL,        .max = MNN_MAX_VAL },
    [MINION_G1_VOLTAGE]   = {.cmdIdx = minionG1VoltageSpCmd,  .name = "miniong1voltage RO",  .regOffset = CMD_REG_OFFSET(minionG1Voltage),  .nbytes = NUM_OF_BYTES_1, .spOps = RO,  .doWrCmd = NULL,          .init = MNN_DEFAULT_VAL,         .min = MNN_MIN_VAL,        .max = MNN_MAX_VAL },
    [MINION_G2_VOLTAGE]   = {.cmdIdx = minionG2VoltageSpCmd,  .name = "miniong2voltage RO",  .regOffset = CMD_REG_OFFSET(minionG2Voltage),  .nbytes = NUM_OF_BYTES_1, .spOps = RO,  .doWrCmd = NULL,          .init = MNN_DEFAULT_VAL,         .min = MNN_MIN_VAL,        .max = MNN_MAX_VAL },
    [MINION_G3_VOLTAGE]   = {.cmdIdx = minionG3VoltageSpCmd,  .name = "miniong3voltage RO",  .regOffset = CMD_REG_OFFSET(minionG3Voltage),  .nbytes = NUM_OF_BYTES_1, .spOps = RO,  .doWrCmd = NULL,          .init = MNN_DEFAULT_VAL,         .min = MNN_MIN_VAL,        .max = MNN_MAX_VAL },
    [MINION_G4_VOLTAGE]   = {.cmdIdx = minionG4VoltageSpCmd,  .name = "miniong4voltage RO",  .regOffset = CMD_REG_OFFSET(minionG4Voltage),  .nbytes = NUM_OF_BYTES_1, .spOps = RO,  .doWrCmd = NULL,          .init = MNN_DEFAULT_VAL,         .min = MNN_MIN_VAL,        .max = MNN_MAX_VAL },
    [MINION_G5_VOLTAGE]   = {.cmdIdx = minionG5VoltageSpCmd,  .name = "miniong5voltage RO",  .regOffset = CMD_REG_OFFSET(minionG5Voltage),  .nbytes = NUM_OF_BYTES_1, .spOps = RO,  .doWrCmd = NULL,          .init = MNN_DEFAULT_VAL,         .min = MNN_MIN_VAL,        .max = MNN_MAX_VAL },
    [MINION_G6_VOLTAGE]   = {.cmdIdx = minionG6VoltageSpCmd,  .name = "miniong6voltage RO",  .regOffset = CMD_REG_OFFSET(minionG6Voltage),  .nbytes = NUM_OF_BYTES_1, .spOps = RO,  .doWrCmd = NULL,          .init = MNN_DEFAULT_VAL,         .min = MNN_MIN_VAL,        .max = MNN_MAX_VAL },
    [MINION_G7_VOLTAGE]   = {.cmdIdx = minionG7VoltageSpCmd,  .name = "miniong7voltage RO",  .regOffset = CMD_REG_OFFSET(minionG7Voltage),  .nbytes = NUM_OF_BYTES_1, .spOps = RO,  .doWrCmd = NULL,          .init = MNN_DEFAULT_VAL,         .min = MNN_MIN_VAL,        .max = MNN_MAX_VAL },
    [MINION_G8_VOLTAGE]   = {.cmdIdx = minionG8VoltageSpCmd,  .name = "miniong8voltage RO",  .regOffset = CMD_REG_OFFSET(minionG8Voltage),  .nbytes = NUM_OF_BYTES_1, .spOps = RO,  .doWrCmd = NULL,          .init = MNN_DEFAULT_VAL,         .min = MNN_MIN_VAL,        .max = MNN_MAX_VAL },
    [MINION_G9_VOLTAGE]   = {.cmdIdx = minionG9VoltageSpCmd,  .name = "miniong9voltage RO",  .regOffset = CMD_REG_OFFSET(minionG9Voltage),  .nbytes = NUM_OF_BYTES_1, .spOps = RO,  .doWrCmd = NULL,          .init = MNN_DEFAULT_VAL,         .min = MNN_MIN_VAL,        .max = MNN_MAX_VAL },
    [MINION_G10_VOLTAGE]  = {.cmdIdx = minionG10VoltageSpCmd, .name = "miniong10voltage RO", .regOffset = CMD_REG_OFFSET(minionG10Voltage), .nbytes = NUM_OF_BYTES_1, .spOps = RO,  .doWrCmd = NULL,          .init = MNN_DEFAULT_VAL,         .min = MNN_MIN_VAL,        .max = MNN_MAX_VAL },
    [MINION_G11_VOLTAGE]  = {.cmdIdx = minionG11VoltageSpCmd, .name = "miniong11voltage RO", .regOffset = CMD_REG_OFFSET(minionG11Voltage), .nbytes = NUM_OF_BYTES_1, .spOps = RO,  .doWrCmd = NULL,          .init = MNN_DEFAULT_VAL,         .min = MNN_MIN_VAL,        .max = MNN_MAX_VAL },
    [MINION_G12_VOLTAGE]  = {.cmdIdx = minionG12VoltageSpCmd, .name = "miniong12voltage RO", .regOffset = CMD_REG_OFFSET(minionG12Voltage), .nbytes = NUM_OF_BYTES_1, .spOps = RO,  .doWrCmd = NULL,          .init = MNN_DEFAULT_VAL,         .min = MNN_MIN_VAL,        .max = MNN_MAX_VAL },
    [MINION_G13_VOLTAGE]  = {.cmdIdx = minionG14VoltageSpCmd, .name = "miniong13voltage RO", .regOffset = CMD_REG_OFFSET(minionG14Voltage), .nbytes = NUM_OF_BYTES_1, .spOps = RO,  .doWrCmd = NULL,          .init = MNN_DEFAULT_VAL,         .min = MNN_MIN_VAL,        .max = MNN_MAX_VAL },
    [MINION_G14_VOLTAGE]  = {.cmdIdx = minionG13VoltageSpCmd, .name = "miniong14voltage RO", .regOffset = CMD_REG_OFFSET(minionG13Voltage), .nbytes = NUM_OF_BYTES_1, .spOps = RO,  .doWrCmd = NULL,          .init = MNN_DEFAULT_VAL,         .min = MNN_MIN_VAL,        .max = MNN_MAX_VAL },
    [MINION_G15_VOLTAGE]  = {.cmdIdx = minionG15VoltageSpCmd, .name = "miniong15voltage RO", .regOffset = CMD_REG_OFFSET(minionG15Voltage), .nbytes = NUM_OF_BYTES_1, .spOps = RO,  .doWrCmd = NULL,          .init = MNN_DEFAULT_VAL,         .min = MNN_MIN_VAL,        .max = MNN_MAX_VAL },
    [MINION_G16_VOLTAGE]  = {.cmdIdx = minionG16VoltageSpCmd, .name = "miniong16voltage RO", .regOffset = CMD_REG_OFFSET(minionG16Voltage), .nbytes = NUM_OF_BYTES_1, .spOps = RO,  .doWrCmd = NULL,          .init = MNN_DEFAULT_VAL,         .min = MNN_MIN_VAL,        .max = MNN_MAX_VAL },
    [MINION_G17_VOLTAGE]  = {.cmdIdx = minionG17VoltageSpCmd, .name = "miniong17voltage RO", .regOffset = CMD_REG_OFFSET(minionG17Voltage), .nbytes = NUM_OF_BYTES_1, .spOps = RO,  .doWrCmd = NULL,          .init = MNN_DEFAULT_VAL,         .min = MNN_MIN_VAL,        .max = MNN_MAX_VAL },
    [FRU_OPS_CMD]         = {.cmdIdx = fruOpsCmdSpCmd,        .name = "fruopscmd RW",        .regOffset = CMD_REG_OFFSET(fruOpsCmd),        .nbytes = NUM_OF_BYTES_1, .spOps = RW,  .doWrCmd = &doFruOpsCmd,  .init = 0,                       .min = 0,                  .max = UINT8_MAX },
    [FRU_OPS_DATA]        = {.cmdIdx = fruOpsDataSpCmd,       .name = "fruopsdata RW",       .regOffset = CMD_REG_OFFSET(fruOpsData),       .nbytes = NUM_OF_BYTES_1, .spOps = RW,  .doWrCmd = &doFruOpsData, .init = 0,                       .min = 0,                  .max = UINT8_MAX },
};
// clang-format on

// clang-format off
//TODO SW-19042: Conversion is defined by the ETSOC-PMIC communication protocol and 
// this should be moved to dedicated shared location
regulatorCodeConversionInfo_t const regulatorCodeConversionInfo[] = {
    {.regIdx = DDQLP_VOLTAGE,      .base_mV = QLP_BASE_mV, .delta_uV = QLP_DELTA_uV },
    {.regIdx = L2_CACHE_VOLTAGE,   .base_mV = SRM_BASE_mV, .delta_uV = SRM_DELTA_uV },
    {.regIdx = DDR_VOLTAGE,        .base_mV = DDR_BASE_mV, .delta_uV = DDR_DELTA_uV },
    {.regIdx = DDQ_VOLTAGE,        .base_mV = DDQ_BASE_mV, .delta_uV = DDQ_DELTA_uV },
    {.regIdx = PCIE_LOGIC_VOLTAGE, .base_mV = PCL_BASE_mV, .delta_uV = PCL_DELTA_uV },
    {.regIdx = PCIE_VOLTAGE,       .base_mV = PCI_BASE_mV, .delta_uV = PCI_DELTA_uV },
    {.regIdx = MAXION_VOLTAGE,     .base_mV = MXN_BASE_mV, .delta_uV = MXN_DELTA_uV },
    {.regIdx = NOC_VOLTAGE,        .base_mV = NOC_BASE_mV, .delta_uV = NOC_DELTA_uV },
    {.regIdx = ALL_MINION_VOLTAGE, .base_mV = MNN_BASE_mV, .delta_uV = MNN_DELTA_uV },
};
// clang-format on

/***********************************************************************
 * Functions:   Declaration of local functions
***********************************************************************/
static int initSPVars(void);
static bool isValueOutOfRange(ETSOCVirtualRegisterIdx_t regIdx, uint32_t value);
static bool isIndexInvalid(ETSOCVirtualRegisterIdx_t regIdx);
static int ETSOCCommandExecute(spCommandIndex_t cmdIdx, bool isCmdRead, uint32_t data);
static uint32_t getValueToSetMultiByteRegister(ETSOCVirtualRegisterIdx_t regIdx, uint32_t data);
static int hexCodeToMilliVolt(ETSOCVirtualRegisterIdx_t regIdx, uint8_t hexCode, uint16_t *p_mV);
static void copyMinionVoltage(uint8_t hexCode);
static void setConfigRegistersToInitValue(void);

/***********************************************************************
 * GLOBAL Functions:   Defininition of global functions
***********************************************************************/
void etsocCmdHandlerInitialize(void)
{
    int status = INVALID_ARGUMENT;

    status = initSPVars();
    assert1(status == STATUS_SUCCESS, "etsocCmdHandlerTask: SP vars init failed", status, 0);

    I2CSRegisterCMDHandler(etsocCommandSendEvent);
}

void etsocCmdHandlerTask(void *pvParameters)
{
    (void)pvParameters;
    int status = INVALID_ARGUMENT;

    while (true)
    {
        etsocCommandEvent_t etsocCmdHandlerEvent;
        status =
            (xQueueReceive(globals.etsocCmdHandlerTaskQueueHandle, &etsocCmdHandlerEvent, portMAX_DELAY) == pdPASS) ?
                STATUS_SUCCESS :
                CMD_HANDLER_ERROR_QUEUE_RECIEVE;

        I2CS_Set_Slave_Busy();
        if (status == STATUS_SUCCESS)
        {
            status = ETSOCCommandExecute(
                etsocCmdHandlerEvent.cmdIdx, etsocCmdHandlerEvent.isDataReadRequested, etsocCmdHandlerEvent.data);
            if (status != STATUS_SUCCESS)
            {
                interruptingPrintfOneLine("etsocCmdHandlerTask: command execution error: %d ", status);
            }
        }
        else
        {
            interruptingPrintfOneLine("etsocCmdHandlerTask: error: %d ", status);
        }
        I2CS_Set_Slave_Ready();
    }
}

int setRegisterValue(ETSOCVirtualRegisterIdx_t regIdx, uint32_t value)
{
    int status = STATUS_SUCCESS;

    if (isIndexInvalid(regIdx))
    {
        interruptingPrintfOneLine("Register index %d is invalid.", regIdx);
        return CMD_HANDLER_ERROR_INVALID_REG_IDX;
    }
    if (isValueOutOfRange(regIdx, value))
    {
        interruptingPrintfOneLine("Value %d for the register index %d is out of range.", value, regIdx);
        return CMD_HANDLER_ERROR_VALUE_OUT_OF_RANGE;
    }

    //store value
    switch (spCmd[regIdx].nbytes)
    {
        case NUM_OF_BYTES_0:
        case NUM_OF_BYTES_1:
            *((uint8_t *)getRegisterAddress(regIdx)) = (uint8_t)value;
            break;
        case NUM_OF_BYTES_2:
            *((uint16_t *)getRegisterAddress(regIdx)) = (uint16_t)value;
            break;
        case NUM_OF_BYTES_4:
            *((uint32_t *)getRegisterAddress(regIdx)) = value;
            break;
        default:
            interruptingPrintfOneLine("Unsupported number of bytes %d.", spCmd[regIdx].nbytes);
            status = CMD_HANDLER_ERROR_INVALID_NUM_BYTES;
            break;
    }
    return status;
}

int getRegisterValue(ETSOCVirtualRegisterIdx_t regIdx, uint32_t *data)
{
    int status = STATUS_SUCCESS;

    if (data == NULL)
    {
        return INVALID_ARGUMENT;
    }

    if (isIndexInvalid(regIdx))
    {
        interruptingPrintfOneLine("Register index %d is invalid.", regIdx);
        return CMD_HANDLER_ERROR_INVALID_REG_IDX;
    }
    switch (spCmd[regIdx].nbytes)
    {
        case NUM_OF_BYTES_0:
        case NUM_OF_BYTES_1:
        {
            uint8_t res = *((uint8_t *)getRegisterAddress(regIdx));
            *data = (uint32_t)res;
            break;
        }
        case NUM_OF_BYTES_2:
        {
            uint16_t res = *((uint16_t *)getRegisterAddress(regIdx));
            *data = (uint32_t)res;
            break;
        }
        case NUM_OF_BYTES_4:
        {
            uint32_t res = *((uint32_t *)getRegisterAddress(regIdx));
            *data = res;
            break;
        }
        default:
            interruptingPrintfOneLine("Unsupported number of bytes %d.", spCmd[regIdx].nbytes);
            status = CMD_HANDLER_ERROR_INVALID_NUM_BYTES;
            break;
    }

    return status;
}

bool getSpCmdReadData(spCommandIndex_t cmdIdx, uint32_t *data, uint32_t *numberOfBytes)
{
    ETSOCVirtualRegisterIdx_t regIdx = getRegisterIndexFromSpCmdIndex(cmdIdx);

    if (regIdx == RESERVED)
    {
        interruptingPrintfOneLine("ETSOC command index %d is invalid.", cmdIdx);
        i2csSetClientError(cmdIdx, I2CS_REG_FORBID);
        return false;
    }

    if (isAccessTypeWriteOnly(cmdIdx))
    {
        interruptingPrintfOneLine("Request read but ETSOC command %d is write only.", cmdIdx);
        i2csSetClientError(cmdIdx, I2CS_RD_FORBID);
        return false;
    }

    *numberOfBytes = getNumberOfBytes(cmdIdx);

    return (getRegisterValue(regIdx, data) == STATUS_SUCCESS);
}

void *getRegisterAddress(ETSOCVirtualRegisterIdx_t regIdx)
{
    return ((uint8_t *)(void *)&spCmdVirtualRegisterStorage) + spCmd[regIdx].regOffset;
}

bool isAccessTypeReadOnly(spCommandIndex_t cmdIdx)
{
    ETSOCVirtualRegisterIdx_t regIdx = getRegisterIndexFromSpCmdIndex(cmdIdx);
    return ((spCmd[regIdx].spOps == RO) || (spCmd[regIdx].spOps == ROC));
}

bool isAccessTypeWriteOnly(spCommandIndex_t cmdIdx)
{
    ETSOCVirtualRegisterIdx_t regIdx = getRegisterIndexFromSpCmdIndex(cmdIdx);
    return (spCmd[regIdx].spOps == WO);
}

bool isCleanAfterReadRequired(spCommandIndex_t cmdIdx)
{
    ETSOCVirtualRegisterIdx_t regIdx = getRegisterIndexFromSpCmdIndex(cmdIdx);
    return (spCmd[regIdx].spOps == ROC);
}

uint32_t getNumberOfBytes(spCommandIndex_t cmdIdx)
{
    ETSOCVirtualRegisterIdx_t regIdx = getRegisterIndexFromSpCmdIndex(cmdIdx);
    return spCmd[regIdx].nbytes;
}

const char *getCmdName(spCommandIndex_t cmdIdx)
{
    ETSOCVirtualRegisterIdx_t regIdx = getRegisterIndexFromSpCmdIndex(cmdIdx);
    return spCmd[regIdx].name;
}

ETSOCVirtualRegisterIdx_t getRegisterIndexFromSpCmdIndex(spCommandIndex_t cmdIdx)
{
    ETSOCVirtualRegisterIdx_t regIdx = RESERVED;
    for (ETSOCVirtualRegisterIdx_t i = RESERVED + 1; i < NUMBER_OF_REGISTERS; i++)
    {
        if (spCmd[i].cmdIdx == cmdIdx)
        {
            regIdx = i;
            break;
        }
    }
    return regIdx;
}

int writeRegulatorReg(ETSOCVirtualRegisterIdx_t regIdx, uint32_t data)
{
    int status = STATUS_SUCCESS;

    uint8_t hexCode = (uint8_t)data;
    uint16_t mvVal = 0;
    status = hexCodeToMilliVolt(regIdx, hexCode, &mvVal);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    if (isValueOutOfRange(regIdx, data))
    {
        printfFromInterrupt("Code 0x%x for the register index %d is out of range.", data, regIdx);
        return CMD_HANDLER_ERROR_VALUE_OUT_OF_RANGE;
    }

    vout_t *pv = findReguInfoByRegIdx(regIdx);
    if (pv == NULL)
    {
        return CMD_HANDLER_ERROR_REGULATOR_INFO;
    }

    voutInfo_t const *p = pv->voutInfo;
    t_structChipInfo const *pChip = &structChipInfo[p->voltageReguId];
    uint8_t chipaddr = pChip->addr7;
    i2cmErr_t rv;

    uint16_t val = multiMvToRaw(mvVal, p);

    for (uint8_t i = 0; i < 2; ++i)
    {
        uint8_t reg = p->reg[i];
        if (reg == NO_REG)
        {
            continue;
        }

        printfFromInterruptNoFlush("writeRegulatorReg: regIdx 0x%02X, hexCode 0x%02X", regIdx, hexCode);
        printfFromInterruptNoFlush("\tWriting 0x%02X to register addr 0x%02X", val, chipaddr);
        rv = reguRegWrite(chipaddr, reg, p->i2clng, val);
        if (rv == I2CM_OK)
        {
            pv->voutRegCurVal = val;

            setRegisterValue(regIdx, hexCode);
            if (p->etsocVirtualRegIdx == ALL_MINION_VOLTAGE)
            {
                copyMinionVoltage(hexCode);
            }
        }
        else
        {
            printfFromInterrupt("writeRegulatorReg: error %d", rv, 0);
            status = I2C_MASTER_ERROR_REG_WRITE;
        }
    }
    return status;
}

int setAllRegulatorsToDefaultVoltage(void)
{
    int status = STATUS_SUCCESS;

    for (unsigned int i = 0; i < ARRAY_SIZE(regulatorCodeConversionInfo); i++)
    {
        ETSOCVirtualRegisterIdx_t regIdx = regulatorCodeConversionInfo[i].regIdx;
        status = writeRegulatorReg(regIdx, spCmd[regIdx].init);
        if (status != STATUS_SUCCESS)
        {
            printfFromInterruptNoFlush("Failed writing to Regulator regIdx 0x%02X, Value: %d", regIdx,
                getDefaultRegulatorMilliVoltValue(regIdx));
            return status;
        }
    }
    return status;
}

int milliVoltToHexCode(ETSOCVirtualRegisterIdx_t regIdx, uint32_t mV, uint8_t *p_hexCode)
{
    for (unsigned int i = 0; i < ARRAY_SIZE(regulatorCodeConversionInfo); i++)
    {
        if (regulatorCodeConversionInfo[i].regIdx == regIdx)
        {
            *p_hexCode = MV2HEX(mV, regulatorCodeConversionInfo[i].base_mV, regulatorCodeConversionInfo[i].delta_uV);
            return STATUS_SUCCESS;
        }
    }
    return CMD_HANDLER_ERROR_REGULATOR_INFO;
}

uint32_t getDefaultRegulatorMilliVoltValue(ETSOCVirtualRegisterIdx_t regIdx)
{
    uint16_t mV = 0;
    hexCodeToMilliVolt(regIdx, (uint8_t)spCmd[regIdx].init, &mV);
    return mV;
}

uint32_t getMinRegulatorMilliVoltValue(ETSOCVirtualRegisterIdx_t regIdx)
{
    uint16_t mV = 0;
    hexCodeToMilliVolt(regIdx, (uint8_t)spCmd[regIdx].min, &mV);
    return mV;
}

uint32_t getMaxRegulatorMilliVoltValue(ETSOCVirtualRegisterIdx_t regIdx)
{
    uint16_t mV = 0;
    hexCodeToMilliVolt(regIdx, (uint8_t)spCmd[regIdx].max, &mV);
    return mV;
}

uint16_t getRegulatorCodeConversionDelta(ETSOCVirtualRegisterIdx_t regIdx)
{
    for (unsigned int i = 0; i < ARRAY_SIZE(regulatorCodeConversionInfo); i++)
    {
        if (regulatorCodeConversionInfo[i].regIdx == regIdx)
        {
            return regulatorCodeConversionInfo[i].delta_uV;
        }
    }
    return 0;
}

void handlePostReadUpdate(spCommandIndex_t cmdIdx)
{
    switch (cmdIdx)
    {
        case pmbStatsSpCmd:
            updatePmbStats();
            break;
        case updateCmdSpCmd:
            rdUpdatePostCmd();
            break;
        case updateDataSpCmd:
            rdUpdatePostData();
            break;
        case fruOpsDataSpCmd:
            fruOpsDataRead();
        default:
            //other commands don't require post-read action
            break;
    }
}

int getPowerAlarmSetPoint(uint32_t *setPoint_mW)
{
    int status = STATUS_SUCCESS;
    uint32_t data = UINT8_MAX;
    status = getRegisterValue(POWER_ALARM, &data);
    uint16_t offset_mW = (uint16_t)(data * POWER_ALARM_SET_POINT_STEP_mW);
    *setPoint_mW = offset_mW + POWER_ALARM_SET_POINT_BASE * 1000;
    return status;
}

bool isInterruptEnabled(interruptController_t intrType)
{
    uint32_t intr_conf_reg_data = 0;
    int status = STATUS_SUCCESS;

    status = getRegisterValue(INTERRUPT_CONF, &intr_conf_reg_data);
    if (status != STATUS_SUCCESS)
    {
        printfFromInterruptNoFlush("Error getting INTERRUPT_CONF register, status = %d", status, 0);
        return false;
    }

    return (intr_conf_reg_data & (1 << intrType));
}

int setInterruptCause(interruptController_t intrType, uint32_t data)
{
    uint32_t intr_cause_reg_data = 0;
    uint32_t setPoint = 0;
    int status = STATUS_SUCCESS;

    if (intrType >= INTR_cmd_invalid)
    {
        printfFromInterruptNoFlush("Invalid interrupt type", 0, 0);
        return INVALID_ARGUMENT;
    }

    status = getRegisterValue(INTERRUPT_CAUSE, &intr_cause_reg_data);
    if (status != STATUS_SUCCESS)
    {
        printfFromInterruptNoFlush("Error getting INTERRUPT_CAUSE register, status = %d", status, 0);
        return status;
    }

    switch (intrType)
    {
        case INTR_cmd_ov_temp:
            status = getRegisterValue(TEMPERATURE_ALARM, &setPoint);
            intr_cause_reg_data = (intr_cause_reg_data & INTR_OVER_TEMP_VALUE_BIT_MASK) |
                                  (((data - setPoint) & 0xff) << INTR_OVER_TEMP_VALUE_BIT_OFFSET);
            break;
        case INTR_cmd_ov_pwr:
            status = getPowerAlarmSetPoint(&setPoint);
            uint32_t delta_mW = (data - setPoint) > POWER_ALARM_SET_POINT_MAX_VALUE_mW ?
                                    POWER_ALARM_SET_POINT_MAX_VALUE_mW :
                                    (data - setPoint);
            uint32_t delta_scaled_mW = delta_mW / POWER_ALARM_SET_POINT_STEP_mW;
            intr_cause_reg_data = (intr_cause_reg_data & INTR_OVER_POWER_VALUE_BIT_MASK) |
                                  ((delta_scaled_mW & 0xff) << INTR_OVER_POWER_VALUE_BIT_OFFSET);
            break;
        case INTR_cmd_minion_droop:
            //todo: to be implemented
            setPoint = 0;
            intr_cause_reg_data = (intr_cause_reg_data & INTR_MNN_DROOP_VALUE_BIT_MASK) |
                                  (((setPoint - data) & 0xff) << INTR_MNN_DROOP_VALUE_BIT_OFFSET);
            break;
        default:
            break;
    }

    if (status != STATUS_SUCCESS)
    {
        printfFromInterruptNoFlush("Error setting INTERRUPT_CAUSE register value, status = %d", status, 0);
        return status;
    }

    status = setRegisterValue(INTERRUPT_CAUSE, (intr_cause_reg_data | ((1 << intrType) & 0xff)));
    if (status != STATUS_SUCCESS)
    {
        printfFromInterruptNoFlush("Error setting INTERRUPT_CAUSE register cause, status = %d", status, 0);
        return status;
    }

    return status;
}

void triggerETSOCInterrupt(void)
{
    gpio_set_out_high_pin_id(PMIC_INT_OUT);
    OSDELAY_MS(10)
    gpio_set_out_low_pin_id(PMIC_INT_OUT);
}

void configureETSOCInterruptsToDefault(void)
{
    setRegisterValue(INTERRUPT_CONF, spCmd[INTERRUPT_CONF].init);
}

void setResetCauseCruSysReset(void)
{
    setRegisterValue(RESET_CAUSE, RST_cause_cru_sys_reset);
}

void i2csSetHandlerError(uint8_t regAddr, i2c_slave_handlerErrorCode code)
{
    printfFromInterruptNoFlush("I2CS handler error %d, reg 0x%x", code, regAddr);

    if (isInterruptEnabled(INTR_cmd_msg_error))
    {
        uint32_t commFailValue;
        uint32_t status = STATUS_SUCCESS;

        status = getRegisterValue(ETSOC_COMM_FAIL, &commFailValue);
        if (status != STATUS_SUCCESS)
        {
            printfFromInterruptNoFlush("Failed to get ETSOC_COMM_FAIL reg value, status = %d", status, 0);
            return;
        }
        //Set multiple err and valid bit
        if (commFailValue & (1 << CMD_COMM_FAIL_DETAILS_VALID_POS))
        {
            commFailValue = commFailValue | (1 << CMD_COMM_FAIL_DETAILS_MULTI_ERR_POS);
        }
        else
        {
            commFailValue |= 1 << CMD_COMM_FAIL_DETAILS_VALID_POS;
            //Set register (command) number
            commFailValue = (commFailValue & CMD_COMM_FAIL_DETAILS_REG_ADDR_MASK) |
                            ((regAddr & CMD_COMM_FAIL_DETAILS_REG_ADDR_MAX_VAL) << CMD_COMM_FAIL_DETAILS_REG_ADDR_POS);
        }
        //Set error code
        commFailValue |= 1 << code;

        status = setRegisterValue(ETSOC_COMM_FAIL, commFailValue);
        if (status != STATUS_SUCCESS)
        {
            printfFromInterruptNoFlush("Failed to set ETSOC_COMM_FAIL reg value, status = %d", status, 0);
            return;
        }

        status = setInterruptCause(INTR_cmd_msg_error, 0);
        if (status != STATUS_SUCCESS)
        {
            printfFromInterruptNoFlush("Failed to set interrupt cause, status = %d", status, 0);
            return;
        }

        triggerETSOCInterrupt();
    }
}

void i2csSetClientError(uint8_t regAddr, i2c_slave_clientErrorCode code)
{
    printfFromInterruptNoFlush("I2CS client error %d, reg 0x%x", code, regAddr);

    if (isInterruptEnabled(INTR_cmd_msg_error))
    {
        uint32_t commFailValue;
        uint32_t status = STATUS_SUCCESS;

        status = getRegisterValue(ETSOC_COMM_FAIL, &commFailValue);
        if (status != STATUS_SUCCESS)
        {
            printfFromInterruptNoFlush("Failed to get ETSOC_COMM_FAIL reg value, status = %d", status, 0);
            return;
        }
        //Set multiple err and valid bit
        if (commFailValue & (1 << CMD_COMM_FAIL_DETAILS_VALID_POS))
        {
            commFailValue = commFailValue | (1 << CMD_COMM_FAIL_DETAILS_MULTI_ERR_POS);
        }
        else
        {
            commFailValue |= 1 << CMD_COMM_FAIL_DETAILS_VALID_POS;
            //Set register (command) number
            commFailValue = (commFailValue & CMD_COMM_FAIL_DETAILS_REG_ADDR_MASK) |
                            ((regAddr & CMD_COMM_FAIL_DETAILS_REG_ADDR_MAX_VAL) << CMD_COMM_FAIL_DETAILS_REG_ADDR_POS);
            //Set error code
            commFailValue = (commFailValue & CMD_COMM_FAIL_DETAILS_CLIENT_ERR_MASK) |
                            ((code & CMD_COMM_FAIL_DETAILS_CLIENT_ERR_MAX_VAL) << CMD_COMM_FAIL_DETAILS_CLIENT_ERR_POS);
        }

        status = setRegisterValue(ETSOC_COMM_FAIL, commFailValue);
        if (status != STATUS_SUCCESS)
        {
            printfFromInterruptNoFlush("Failed to set ETSOC_COMM_FAIL reg value, status = %d", status, 0);
            return;
        }

        status = setInterruptCause(INTR_cmd_msg_error, 0);
        if (status != STATUS_SUCCESS)
        {
            printfFromInterruptNoFlush("Failed to set interrupt cause, status = %d", status, 0);
            return;
        }

        triggerETSOCInterrupt();
    }
}

void setRegulatorCommFailError(uint8_t regulatorAddr, uint8_t cmdRegAddr, i2cmErr_t code)
{
    if (isInterruptEnabled(INTR_cmd_reg_comm_fail))
    {
        uint32_t commFailValue;
        uint32_t status = STATUS_SUCCESS;

        status = getRegisterValue(REGULATOR_COMM_FAIL, &commFailValue);
        if (status != STATUS_SUCCESS)
        {
            printfFromInterruptNoFlush("Failed to get REGULATOR_COMM_FAIL reg value, status = %d", status, 0);
            return;
        }

        //Set multiple err and valid bit
        if (commFailValue & (1 << REG_COMM_FAIL_DETAILS_VALID_POS))
        {
            commFailValue = commFailValue | (1 << REG_COMM_FAIL_DETAILS_MULTI_ERR_POS);
        }
        else
        {
            commFailValue |= 1 << REG_COMM_FAIL_DETAILS_VALID_POS;
            //Set regulator address
            commFailValue =
                (commFailValue & REG_COMM_FAIL_DETAILS_REG_ADDR_MASK) |
                ((regulatorAddr & REG_COMM_FAIL_DETAILS_REG_ADDR_MAX_VAL) << REG_COMM_FAIL_DETAILS_REG_ADDR_POS);
            //Set command address
            commFailValue =
                (commFailValue & REG_COMM_FAIL_DETAILS_CMD_ADDR_MASK) |
                ((cmdRegAddr & REG_COMM_FAIL_DETAILS_CMD_ADDR_MAX_VAL) << REG_COMM_FAIL_DETAILS_CMD_ADDR_POS);
            //Set error code
            commFailValue = (commFailValue & REG_COMM_FAIL_DETAILS_ERR_MASK) |
                            ((code & REG_COMM_FAIL_DETAILS_ERR_MAX_VAL) << REG_COMM_FAIL_DETAILS_ERR_POS);
        }

        status = setRegisterValue(REGULATOR_COMM_FAIL, commFailValue);
        if (status != STATUS_SUCCESS)
        {
            printfFromInterruptNoFlush("Failed to set REGULATOR_COMM_FAIL reg value, status = %d", status, 0);
            return;
        }

        status = setInterruptCause(INTR_cmd_reg_comm_fail, 0);
        if (status != STATUS_SUCCESS)
        {
            printfFromInterruptNoFlush("Failed to set interrupt cause, status = %d", status, 0);
            return;
        }

        triggerETSOCInterrupt();
    }
}

/***********************************************************************
 * Functions:   Defininition of local functions
***********************************************************************/
//TODO: SW-15170 to be removed when all commands are implemented
static void commandNotImplemented(ETSOCVirtualRegisterIdx_t regIdx, uint32_t data)
{
    // cmd not implemented
    interruptingPrintfOneLine("ETSOC command with index %d is not yet implemented.", spCmd[regIdx].cmdIdx);
    setRegisterValue(regIdx, data);
}

static int setPowerAlarm(ETSOCVirtualRegisterIdx_t regIdx, uint32_t data)
{
    setRegisterValue(regIdx, data);
    return STATUS_SUCCESS;
}

static int setTemperatureAlarm(ETSOCVirtualRegisterIdx_t regIdx, uint32_t data)
{
    setRegisterValue(regIdx, data);
    return STATUS_SUCCESS;
}

static int setIntrCfg(ETSOCVirtualRegisterIdx_t regIdx, uint32_t data)
{
    setRegisterValue(regIdx, data);
    return STATUS_SUCCESS;
}

static int setRstCtrl(ETSOCVirtualRegisterIdx_t regIdx, uint32_t data)
{
    commandNotImplemented(regIdx, data);

    return STATUS_SUCCESS;
}

static int setGpioCfg(ETSOCVirtualRegisterIdx_t regIdx, uint32_t data)
{
    commandNotImplemented(regIdx, data);

    return STATUS_SUCCESS;
}

static int setGetGpio(ETSOCVirtualRegisterIdx_t regIdx, uint32_t data)
{
    commandNotImplemented(regIdx, data);

    return STATUS_SUCCESS;
}

static int hexCodeToMilliVolt(ETSOCVirtualRegisterIdx_t regIdx, uint8_t hexCode, uint16_t *p_mV)
{
    for (unsigned int i = 0; i < ARRAY_SIZE(regulatorCodeConversionInfo); i++)
    {
        if (regulatorCodeConversionInfo[i].regIdx == regIdx)
        {
            *p_mV = HEX2MV(hexCode, regulatorCodeConversionInfo[i].base_mV, regulatorCodeConversionInfo[i].delta_uV);
            return STATUS_SUCCESS;
        }
    }
    return CMD_HANDLER_ERROR_REGULATOR_INFO;
}

static void copyMinionVoltage(uint8_t hexCode)
{
    int status = STATUS_SUCCESS;

    for (uint8_t i = 0; i < NUMBER_OF_MINION_VOLTAGE_GROUPS; ++i)
    {
        status = setRegisterValue((MINION_G1_VOLTAGE + i), hexCode);
        if (status != STATUS_SUCCESS)
        {
            interruptingPrintfOneLine("copyMinionVoltage[%d]: Error %d ", i, status);
        }
    }
}

static int doWdt(ETSOCVirtualRegisterIdx_t regIdx, uint32_t data)
{
    commandNotImplemented(regIdx, data);

    return STATUS_SUCCESS;
}

static int resetWdt(ETSOCVirtualRegisterIdx_t regIdx, uint32_t data)
{
    commandNotImplemented(regIdx, data);

    return STATUS_SUCCESS;
}

static int doReset(ETSOCVirtualRegisterIdx_t regIdx, uint32_t data)
{
    (void)regIdx;
    int status = STATUS_SUCCESS;

    switch ((uint8_t)data)
    {
        case RST_cmd_perst:
            gpio_set_out_low_pin_id(SOC_PERST_OUT);
            OSDELAY_MS(10)
            interruptingPrintfOneLine("SOC commanded: toggle PERST");
            gpio_set_out_high_pin_id(SOC_PERST_OUT);
            break;
        case RST_cmd_soc:
            //Set configuration registers back to their init values and let ETSOC reinitialize them
            setConfigRegistersToInitValue();
            gpio_set_out_low_pin_id(SOC_RST_OUT);
            OSDELAY_MS(10)
            interruptingPrintfOneLine("SOC commanded: toggle SOC RESET");
            gpio_set_out_high_pin_id(SOC_RST_OUT);
            break;
        case RST_cmd_pwrCycle:
            interruptingPrintfOneLine("SOC commanded: Power cycle - PMIC will reboot and await PERST");
            IOSEM_TAKE
            gpioDown(&globals.commonData.powerState);
            IOSEM_GIVE
            OSDELAY_MS(100)
            reboot();
            break;
        case RST_cmd_shutdown:
            interruptingPrintBegin();
            interruptingPrintf("SOC commanded: Shut down and await Power Cycle\n");
            interruptingPrintf("Power down\n");
            IOSEM_TAKE
            gpioDown(&globals.commonData.powerState);
            IOSEM_GIVE
            interruptingPrintf("For proper PERST behavior, Reboot host.  Otherwise use 'on' command to power on.\n");
            interruptingPrintEnd();
            break;
        case RST_cmd_current:
            interruptingPrintfOneLine("SOC commanded: Boot to current");
            bootToImage(BT_current);
            interruptingPrintfOneLine("Boot to current failed");
            break;
        case RST_cmd_golden:
            interruptingPrintfOneLine("SOC commanded: Boot to golden");
            bootToImage(BT_golden);
            interruptingPrintfOneLine("Boot to golden failed");
            break;
        default:
            interruptingPrintfOneLine("Invalid SOC reset command");
            break;
    }

    return status;
}

static int wrPmbStats(ETSOCVirtualRegisterIdx_t regIdx, uint32_t data)
{
    /* This function has blocking calls since 
    access to pmbStats buffer is guarded with semaphore */
    (void)regIdx;
    wrPmbStats2((uint8_t)data); //PMB_STATS register is set in updatePmbStats()

    return STATUS_SUCCESS;
}

static int doFwUpdateCmd(ETSOCVirtualRegisterIdx_t regIdx, uint32_t data)
{
    int status = STATUS_SUCCESS;
    uint32_t reg_data;
    //handle different max value when register is written from SP
    //bits 7-5 are read-only if register is writen from SP
    status = getRegisterValue(regIdx, &reg_data);
    if (status == STATUS_SUCCESS)
    {
        uint32_t dataToSet = (reg_data & ~FW_UPDATE_CMD_SP_WR_MASK) | (data & FW_UPDATE_CMD_SP_WR_MASK);

        status = setRegisterValue(regIdx, dataToSet);
        if (status == STATUS_SUCCESS)
        {
            status = wrUpdateCmd(regIdx);
        }
    }

    return status;
}

static int doFwUpdateData(ETSOCVirtualRegisterIdx_t regIdx, uint32_t data)
{
    int status = STATUS_SUCCESS;

    //handle 4-bytes write command
    uint32_t dataToSet = getValueToSetMultiByteRegister(regIdx, data);

    status = setRegisterValue(regIdx, dataToSet);
    if (status == STATUS_SUCCESS)
    {
        //multi-bytes (4) write command, wait for all bytes before triggering the action
        static uint32_t byteCount = 0;
        byteCount = (byteCount + 1) % spCmd[regIdx].nbytes;
        if (byteCount == 0)
        {
            wrUpdateData(dataToSet);
        }
    }

    return status;
}

static int doFruOpsCmd(ETSOCVirtualRegisterIdx_t regIdx, uint32_t data)
{
    return fruOpsCmdSet(regIdx, data);
}

static int doFruOpsData(ETSOCVirtualRegisterIdx_t regIdx, uint32_t data)
{
    return fruOpsDataWrite(data);
}

static uint32_t getValueToSetMultiByteRegister(ETSOCVirtualRegisterIdx_t regIdx, uint32_t data)
{
    static uint32_t byteCount = 0;
    uint32_t reg_data;
    uint32_t value = UINT32_MAX;
    int status = STATUS_SUCCESS;

    if (spCmd[regIdx].nbytes == NUM_OF_BYTES_0)
    {
        return data;
    }

    status = getRegisterValue(regIdx, &reg_data);
    if (status == STATUS_SUCCESS)
    {
        value = (byteCount == 0) ? data : (reg_data | data << (8 * byteCount));
        byteCount = (byteCount + 1) % spCmd[regIdx].nbytes;
    }
    else
    {
        interruptingPrintfOneLine("getValueToSetMultiByteRegister: Error %d", status);
    }

    return value;
}

static int initSPVars(void)
{
    int status = STATUS_SUCCESS;

    for (ETSOCVirtualRegisterIdx_t i = 1; i < NUMBER_OF_REGISTERS; i++)
    {
        status = setRegisterValue(i, spCmd[i].init);
        if (status != STATUS_SUCCESS)
        {
            return status;
        }
    }

    status = setRegisterValue(FIRMWARE_VERSION, Image_Metadata_Get_Curr_Fw_Ver());
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    uint32_t hwBoardType = (Hw_Encoding_Get_Hw_Board_Type() << BOARD_TYPE_BIT_POS) |
                           (Hw_Encoding_Get_Hw_Design_Revision() << BOARD_DESIGN_REV_BIT_POS) |
                           (Hw_Encoding_Get_Hw_Modification_Revision() << BOARD_MODIFICATION_REV_BIT_POS) |
                           (Hw_Encoding_Get_Hw_Board_Unique_Id() << BOARD_UID_BIT_POS);
    status = setRegisterValue(BOARD_TYPE, hwBoardType);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    return status;
}

static bool isValueOutOfRange(ETSOCVirtualRegisterIdx_t regIdx, uint32_t value)
{
    return ((value < spCmd[regIdx].min) || (value > spCmd[regIdx].max));
}

static bool isIndexInvalid(ETSOCVirtualRegisterIdx_t regIdx)
{
    return ((regIdx < 1) || (regIdx >= NUMBER_OF_REGISTERS)); //index 0 is reserved
}

static int ETSOCCommandExecute(spCommandIndex_t cmdIdx, bool isCmdRead, uint32_t data)
{
    int status = STATUS_SUCCESS;
    ETSOCVirtualRegisterIdx_t regIdx = getRegisterIndexFromSpCmdIndex(cmdIdx);

    if (regIdx == RESERVED)
    {
        interruptingPrintfOneLine("ETSOC command index %d is invalid.", cmdIdx);
        i2csSetClientError(cmdIdx, I2CS_REG_FORBID);
        return CMD_HANDLER_ERROR_INVALID_REG_IDX;
    }

    if (isCmdRead) //ETSOC read request
    {
        if (isAccessTypeWriteOnly(cmdIdx))
        {
            interruptingPrintfOneLine("Request read but ETSOC command %d is write only.", cmdIdx);
            i2csSetClientError(cmdIdx, I2CS_RD_FORBID);
            return CMD_HANDLER_ERROR_READ_FORBIDDEN;
        }

        //handle special commands like auto increment after read for updatePmbStats
        //or post read update for updateCmd and updateData.
        handlePostReadUpdate(cmdIdx);

        if (isCleanAfterReadRequired(cmdIdx))
        {
            status = setRegisterValue(regIdx, 0);
        }
    }
    else //ETSOC write request
    {
        if (isAccessTypeReadOnly(cmdIdx))
        {
            interruptingPrintfOneLine("Request write but ETSOC command %d is read only.", cmdIdx);
            i2csSetClientError(cmdIdx, I2CS_WR_FORBID);
            return CMD_HANDLER_ERROR_WRITE_FORBIDDEN;
        }

        if (!isValueOutOfRange(regIdx, data))
        {
            status = spCmd[regIdx].doWrCmd(regIdx, data);
            if (status != STATUS_SUCCESS)
            {
                interruptingPrintfOneLine("ETSOCCommandExecute: CMD %02X failed, Error: %d", regIdx, status);
            }
        }
        else
        {
            status = CMD_HANDLER_ERROR_VALUE_OUT_OF_RANGE;
            interruptingPrintfOneLine("Value %d for the register index %d is out of range.", data, regIdx);
            i2csSetClientError(cmdIdx, I2CS_VALUE_FORBID);
        }
    }

    return status;
}

int updatePmbStats(void)
{
    uint16_t data = 0;
    if (pmbStatsReadDevc < NCHAN)
    { // is in .item
        if (pmbStatsReadStat < NSTAT - 1)
        { // is in .arr.stats
            data = pmbStatsSnapshot.item[pmbStatsReadDevc][pmbStatsReadReg].arr.stats[pmbStatsReadStat];
            ++pmbStatsReadStat;
        }
        else
        {
            data = (uint16_t)((pmbStatsSnapshot.item[pmbStatsReadDevc][pmbStatsReadReg].arr.ave + (1 << 15)) >> 16);
            pmbStatsReadStat = 0;
            if (++pmbStatsReadReg == NREG)
            {
                pmbStatsReadReg = 0;
                ++pmbStatsReadDevc;
            }
        }
    }
    else
    {
        if (pmbStatsReadDevc < NCHAN + 1)
        {
            if (pmbStatsReadStat < 2)
            {
                u64U16_t *p = (pmbStatsReadStat == 0) ? &pmbStatsSnapshot.nSamples : &pmbStatsSnapshot.nFails;
                data = p->u16[pmbStatsReadReg];
                if (++pmbStatsReadReg == 4)
                {
                    pmbStatsReadReg = 0;
                    ++pmbStatsReadStat;
                }
            } // else don't do anything, data remains 0
        }     // else don't do anything, data remains 0
    }

    return setRegisterValue(PMB_STATS, data);
}

static void setConfigRegistersToInitValue(void)
{
    setRegisterValue(INTERRUPT_CONF, spCmd[INTERRUPT_CONF].init);
    setRegisterValue(GPIO_CONF, spCmd[GPIO_CONF].init);
    setRegisterValue(WATCHDOG_CONF, spCmd[WATCHDOG_CONF].init);
}
