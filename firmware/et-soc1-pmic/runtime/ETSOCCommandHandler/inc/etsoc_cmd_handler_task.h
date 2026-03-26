/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file etsoc_cmd_handler_task.h
    \brief PMIC control from ET-SOC.
*/
/***********************************************************************/

#ifndef __ETSOCCMDHANDLERTASK_H__
#define __ETSOCCMDHANDLERTASK_H__

#include <stdbool.h>
#include "utils.h"
#include "globals.h"

//TODO SW-16870: All SP-PMIC protocol related macros and data types will be moved to pmic_hal shared file

/***********************************************************************
 * Macros:        Defininition of global macros
***********************************************************************/
#define POWER_ALARM_DEFAULT_VAL 0xFF //Max val 63.75W
#define TEMP_ALARM_DEFAULT_VAL  0x41
#define TEMP_ALARM_MIN_VAL      0x37
#define TEMP_ALARM_MAX_VAL      0x55
#define INT_CONFIG_DEFAULT_VAL  0x00
#define WATCHDOG_CONFIG_MAX_VAL 0x3F // only 6 out of 8 bits are used
#define SYS_TEMP_MIN_VAL        0x00
#define SYS_TEMP_MAX_VAL        0x69
#define REGU_FAULT_MAX_VAL      0x7FFFFFFF // only 31 out of 32 bits are used
#define REGU_COMM_FAIL_MAX_VAL  0x7FFFFF   // only 23 out of 32 bits are used
#define RST_CTRL_MAX_VAL        0x01       // only 1 bit used

#define FW_UPDATE_CMD_SP_WR_MASK 0x1F // bits 0-4 are RW, bits 5-7 are RO

#define LOW_VOLTAGE_ALARM_SET_POINT_mV 285

//Power alarm set point:
//bits 2-7 represent whole part in range 0W - 63W
//bits 0-1 represent decimal part in 0.25W steps
//Same scaling is used for power alarm delta value
#define POWER_ALARM_SET_POINT_STEP_mW      250U
#define POWER_ALARM_SET_POINT_MAX_VALUE_mW (POWER_ALARM_SET_POINT_STEP_mW * 0xff)

#define POWER_ALARM_SET_POINT_BASE         15
#define INTR_OVER_TEMP_VALUE_BIT_OFFSET    8
#define INTR_OVER_TEMP_VALUE_BIT_MASK      (~(0xff << INTR_OVER_TEMP_VALUE_BIT_OFFSET)) //0xffff00ff
#define INTR_OVER_POWER_VALUE_BIT_OFFSET   16
#define INTR_OVER_POWER_VALUE_BIT_MASK     (~(0xff << INTR_OVER_POWER_VALUE_BIT_OFFSET)) //0xff00ffff
#define INTR_MNN_DROOP_VALUE_BIT_OFFSET    24
#define INTR_MNN_DROOP_VALUE_BIT_MASK      (~(0xff << INTR_MNN_DROOP_VALUE_BIT_OFFSET)) //0x00ffffff

#define CMD_COMM_FAIL_DETAILS_REG_ADDR_POS     24
#define CMD_COMM_FAIL_DETAILS_REG_ADDR_MAX_VAL 0xff
#define CMD_COMM_FAIL_DETAILS_REG_ADDR_MASK \
    (~(CMD_COMM_FAIL_DETAILS_REG_ADDR_MAX_VAL << CMD_COMM_FAIL_DETAILS_REG_ADDR_POS))
#define CMD_COMM_FAIL_DETAILS_CLIENT_ERR_POS     2
#define CMD_COMM_FAIL_DETAILS_CLIENT_ERR_MAX_VAL 0x7
#define CMD_COMM_FAIL_DETAILS_CLIENT_ERR_MASK \
    (~(CMD_COMM_FAIL_DETAILS_CLIENT_ERR_MAX_VAL << CMD_COMM_FAIL_DETAILS_CLIENT_ERR_POS))
#define CMD_COMM_FAIL_DETAILS_MULTI_ERR_POS 1
#define CMD_COMM_FAIL_DETAILS_VALID_POS     0

#define REG_COMM_FAIL_DETAILS_REG_ADDR_POS     16
#define REG_COMM_FAIL_DETAILS_REG_ADDR_MAX_VAL 0x7f
#define REG_COMM_FAIL_DETAILS_REG_ADDR_MASK \
    (~(REG_COMM_FAIL_DETAILS_REG_ADDR_MAX_VAL << REG_COMM_FAIL_DETAILS_REG_ADDR_POS))
#define REG_COMM_FAIL_DETAILS_CMD_ADDR_POS     8
#define REG_COMM_FAIL_DETAILS_CMD_ADDR_MAX_VAL 0xff
#define REG_COMM_FAIL_DETAILS_CMD_ADDR_MASK \
    (~(REG_COMM_FAIL_DETAILS_CMD_ADDR_MAX_VAL << REG_COMM_FAIL_DETAILS_CMD_ADDR_POS))
#define REG_COMM_FAIL_DETAILS_ERR_POS       5
#define REG_COMM_FAIL_DETAILS_ERR_MAX_VAL   0x7
#define REG_COMM_FAIL_DETAILS_ERR_MASK      (~(REG_COMM_FAIL_DETAILS_ERR_MAX_VAL << REG_COMM_FAIL_DETAILS_ERR_POS))
#define REG_COMM_FAIL_DETAILS_MULTI_ERR_POS 1
#define REG_COMM_FAIL_DETAILS_VALID_POS     0

#define NUMBER_OF_MINION_VOLTAGE_GROUPS 17

#define BOARD_TYPE_BIT_POS             0
#define BOARD_DESIGN_REV_BIT_POS       8
#define BOARD_MODIFICATION_REV_BIT_POS 16
#define BOARD_UID_BIT_POS              24

// Default, min, and max values as defined per communication protocol.
// Voltages are converted to hex codes according to regulators' documentation.

#define MV2HEX(mV, base, delta)   ((uint8_t)((((mV)-base) * 1000UL + (delta / 2)) / delta))
#define HEX2MV(code, base, delta) (base + (uint16_t)((delta * code + 500UL) / 1000UL))

#define QLP_BASE_mV        (250)
#define QLP_DELTA_uV       (10000)
#define QLP_DEFAULT_VAL_mV (640) // 0x27
#define QLP_MIN_VAL_mV     (580) // 0x1C
#define QLP_MAX_VAL_mV     (660) // 0x29
#define QLP_DEFAULT_VAL    MV2HEX(QLP_DEFAULT_VAL_mV, QLP_BASE_mV, QLP_DELTA_uV)
#define QLP_MIN_VAL        MV2HEX(QLP_MIN_VAL_mV, QLP_BASE_mV, QLP_DELTA_uV)
#define QLP_MAX_VAL        MV2HEX(QLP_MAX_VAL_mV, QLP_BASE_mV, QLP_DELTA_uV)

#define SRM_BASE_mV        (250)
#define SRM_DELTA_uV       (5000)
#define SRM_DEFAULT_VAL_mV (700) // 0x5A
#define SRM_MIN_VAL_mV     (660) // 0x52
#define SRM_MAX_VAL_mV     (850) // 0x78
#define SRM_DEFAULT_VAL    MV2HEX(SRM_DEFAULT_VAL_mV, SRM_BASE_mV, SRM_DELTA_uV)
#define SRM_MIN_VAL        MV2HEX(SRM_MIN_VAL_mV, SRM_BASE_mV, SRM_DELTA_uV)
#define SRM_MAX_VAL        MV2HEX(SRM_MAX_VAL_mV, SRM_BASE_mV, SRM_DELTA_uV)

#define DDR_BASE_mV        (250)
#define DDR_DELTA_uV       (5000)
#define DDR_DEFAULT_VAL_mV (800) // 0x6E
#define DDR_MIN_VAL_mV     (700) // 0x5A
#define DDR_MAX_VAL_mV     (870) // 0x7C
#define DDR_DEFAULT_VAL    MV2HEX(DDR_DEFAULT_VAL_mV, DDR_BASE_mV, DDR_DELTA_uV)
#define DDR_MIN_VAL        MV2HEX(DDR_MIN_VAL_mV, DDR_BASE_mV, DDR_DELTA_uV)
#define DDR_MAX_VAL        MV2HEX(DDR_MAX_VAL_mV, DDR_BASE_mV, DDR_DELTA_uV)

#define DDQ_BASE_mV        (250)
#define DDQ_DELTA_uV       (10000)
#define DDQ_DEFAULT_VAL_mV (1100) // 0x55
#define DDQ_MIN_VAL_mV     (1000) // 0x4B
#define DDQ_MAX_VAL_mV     (1120) // 0x57
#define DDQ_DEFAULT_VAL    MV2HEX(DDQ_DEFAULT_VAL_mV, DDQ_BASE_mV, DDQ_DELTA_uV)
#define DDQ_MIN_VAL        MV2HEX(DDQ_MIN_VAL_mV, DDQ_BASE_mV, DDQ_DELTA_uV)
#define DDQ_MAX_VAL        MV2HEX(DDQ_MAX_VAL_mV, DDQ_BASE_mV, DDQ_DELTA_uV)

#define PCL_BASE_mV        (600)
#define PCL_DELTA_uV       (6250)
#define PCL_DEFAULT_VAL_mV (775)    // 0x1C
#define PCL_MIN_VAL_mV     (731.25) // 0x15
#define PCL_MAX_VAL_mV     (815)    // 0x22
#define PCL_DEFAULT_VAL    MV2HEX(PCL_DEFAULT_VAL_mV, PCL_BASE_mV, PCL_DELTA_uV)
#define PCL_MIN_VAL        MV2HEX(PCL_MIN_VAL_mV, PCL_BASE_mV, PCL_DELTA_uV)
#define PCL_MAX_VAL        MV2HEX(PCL_MAX_VAL_mV, PCL_BASE_mV, PCL_DELTA_uV)

#define PCI_BASE_mV        (600)
#define PCI_DELTA_uV       (12500)
#define PCI_DEFAULT_VAL_mV (1500) // 0x48
#define PCI_MIN_VAL_mV     (1400) // 0x40
#define PCI_MAX_VAL_mV     (1525) // 0x4A
#define PCI_DEFAULT_VAL    MV2HEX(PCI_DEFAULT_VAL_mV, PCI_BASE_mV, PCI_DELTA_uV)
#define PCI_MIN_VAL        MV2HEX(PCI_MIN_VAL_mV, PCI_BASE_mV, PCI_DELTA_uV)
#define PCI_MAX_VAL        MV2HEX(PCI_MAX_VAL_mV, PCI_BASE_mV, PCI_DELTA_uV)

#define MXN_BASE_mV        (250)
#define MXN_DELTA_uV       (5000)
#define MXN_DEFAULT_VAL_mV (600) // 0x46
#define MXN_MIN_VAL_mV     (600) // 0x46
#define MXN_MAX_VAL_mV     (870) // 0x7C
#define MXN_DEFAULT_VAL    MV2HEX(MXN_DEFAULT_VAL_mV, MXN_BASE_mV, MXN_DELTA_uV)
#define MXN_MIN_VAL        MV2HEX(MXN_MIN_VAL_mV, MXN_BASE_mV, MXN_DELTA_uV)
#define MXN_MAX_VAL        MV2HEX(MXN_MAX_VAL_mV, MXN_BASE_mV, MXN_DELTA_uV)

#define NOC_BASE_mV        (250)
#define NOC_DELTA_uV       (5000)
#define NOC_DEFAULT_VAL_mV (450) // 0x28
#define NOC_MIN_VAL_mV     (400) // 0x1E
#define NOC_MAX_VAL_mV     (600) // 0x46
#define NOC_DEFAULT_VAL    MV2HEX(NOC_DEFAULT_VAL_mV, NOC_BASE_mV, NOC_DELTA_uV)
#define NOC_MIN_VAL        MV2HEX(NOC_MIN_VAL_mV, NOC_BASE_mV, NOC_DELTA_uV)
#define NOC_MAX_VAL        MV2HEX(NOC_MAX_VAL_mV, NOC_BASE_mV, NOC_DELTA_uV)

#define MNN_BASE_mV        (250)
#define MNN_DELTA_uV       (5000)
#define MNN_DEFAULT_VAL_mV (400) // 0x1E
#define MNN_MIN_VAL_mV     (400) // 0x1E
#define MNN_MAX_VAL_mV     (620) // 0x4A
#define MNN_DEFAULT_VAL    MV2HEX(MNN_DEFAULT_VAL_mV, MNN_BASE_mV, MNN_DELTA_uV)
#define MNN_MIN_VAL        MV2HEX(MNN_MIN_VAL_mV, MNN_BASE_mV, MNN_DELTA_uV)
#define MNN_MAX_VAL        MV2HEX(MNN_MAX_VAL_mV, MNN_BASE_mV, MNN_DELTA_uV)

/***********************************************************************
 * GLOBAL Data types:      Defininition of global data types
***********************************************************************/
typedef enum {
    reservedSpCmd = 0x00,
    firmwareVersionSpCmd = 0x01,
    boardTypeSpCmd = 0x02,
    voltageInSpCmd = 0x03,
    powerInSpCmd = 0x04,
    averagePowerSpCmd = 0x05,
    systemTempSpCmd = 0x06,
    powerAlarmSpCmd = 0x07,
    tempAlarmSpCmd = 0x08,
    intConfigSpCmd = 0x09,
    intCauseSpCmd = 0x0A,
    regulatorFaultSpCmd = 0x0B,
    reguCommFailSpCmd = 0x0C,
    cmdCommFailSpCmd = 0x0D,
    watchdogConfigSpCmd = 0x0E,
    watchdogResetSpCmd = 0x0F,
    resetCauseSpCmd = 0x10,
    resetCommandSpCmd = 0x11,
    resetControlSpCmd = 0x12,
    GPIOConfigSpCmd = 0x13,
    GPIOControlSpCmd = 0x14,
    pmbStatsSpCmd = 0x15,
    updateCmdSpCmd = 0x16,
    updateDataSpCmd = 0x17,
    DDQLPVoltageSpCmd = 0x18,
    L2CacheVoltageSpCmd = 0x19,
    DDRVoltageSpCmd = 0x1A,
    DDQVoltageSpCmd = 0x1B,
    PCIeLogicVoltageSpCmd = 0x1C,
    PCIeVoltageSpCmd = 0x1D,
    maxionVoltageSpCmd = 0x1E,
    NOCVoltageSpCmd = 0x1F,
    allMinionVoltageSpCmd = 0x20,
    minionG1VoltageSpCmd = 0x21,
    minionG2VoltageSpCmd = 0x22,
    minionG3VoltageSpCmd = 0x23,
    minionG4VoltageSpCmd = 0x24,
    minionG5VoltageSpCmd = 0x25,
    minionG6VoltageSpCmd = 0x26,
    minionG7VoltageSpCmd = 0x27,
    minionG8VoltageSpCmd = 0x28,
    minionG9VoltageSpCmd = 0x29,
    minionG10VoltageSpCmd = 0x2A,
    minionG11VoltageSpCmd = 0x2B,
    minionG12VoltageSpCmd = 0x2C,
    minionG13VoltageSpCmd = 0x2D,
    minionG14VoltageSpCmd = 0x2E,
    minionG15VoltageSpCmd = 0x2F,
    minionG16VoltageSpCmd = 0x30,
    minionG17VoltageSpCmd = 0x31,
    fruOpsCmdSpCmd = 0x32,
    fruOpsDataSpCmd = 0x33,
    numberOfSpCommands, // must be the last
} spCommandIndex_t;

typedef enum {
    INTR_cmd_ov_temp = 0,
    INTR_cmd_ov_pwr = 1,
    INTR_cmd_pwr_fail = 2,
    INTR_cmd_minion_droop = 3,
    INTR_cmd_reserved = 4,
    INTR_cmd_msg_error = 5,
    INTR_cmd_reg_comm_fail = 6,
    INTR_cmd_reg_fault = 7,

    INTR_cmd_invalid = 8,
} interruptController_t;

enum {
    RST_cause_no_rst = 0x00,
    RST_cause_pwr_on = 0x01,
    RST_cause_warm_pwr_on = 0x02,
    RST_cause_sw_reset = 0x03,
    RST_cause_pmic_reset = 0x04,
    RST_cause_perst_reset = 0x05,
    RST_cause_wdt_reset = 0x06,
    RST_cause_cru_sys_reset = 0x07,
    RST_cause_sw_pwrcycle = 0x08,
    RST_cause_pmic_pwrcycle = 0x09,
    RST_cause_regfault_pwrcycle = 0x0A,
    RST_cause_ovrpower_pwrcycle = 0x0B,
    RST_cause_ovrtempr_pwrcycle = 0x0C
};

enum {
    RST_cmd_none = 0x00,
    RST_cmd_perst = 0x01,
    RST_cmd_soc = 0x02,
    RST_cmd_pwrCycle = 0x03,
    RST_cmd_shutdown = 0x04,
    RST_cmd_current = 0x05,
    RST_cmd_golden = 0x06
};

typedef enum {
    I2CH_UNEXPECTED_STOP_BIT = 8,               /* Unexpected stop condition */
    I2CH_UNEXPECTED_NOT_STOP_BIT = 9,           /* Unexpected not stop condition */
    I2CH_RD_UNEXPECTED_ACK_BIT = 10,            /* Read unexpected ACK */
    I2CH_UNEXPECTED_AMATCH_BIT = 11,            /* Unexpected acknowledge match */
    I2CH_RPTD_START_IS_MASTER_WRITE_BIT = 12,   /* Repeated start condition is a master write */
    I2CH_RPTD_START_AFTER_MASTER_READ_BIT = 13, /* Repeated start condition after a master read */
    I2CH_RD_XTRA_DATA_BIT = 14,                 /* Read extra data */
    I2CH_WR_XTRA_DATA_BIT = 15,                 /* Write extra data */
    I2CH_OTHER_INT_FLAG_BIT = 16,               /* Other interrupt flag set - unexpected AMATCH, DRDY, STOP*/
    I2CH_PMIC_ASSERT_FAIL_BIT = 23,             /* PMIC assert failure */
} i2c_slave_handlerErrorCode;

typedef enum {
    I2CS_PMIC_OK = 0x0,                  /* No error */
    I2CS_PMIC_HERR = 0x1,                /* i2c error code */
    I2CS_PMIC_NOT_READY_VIOLATION = 0x2, /* PMIC not ready violation */
    I2CS_REG_FORBID = 0x3,               /* Register access forbidden */
    I2CS_RD_FORBID = 0x4,                /* Read access forbidden */
    I2CS_WR_FORBID = 0x5,                /* Write access forbidden */
    I2CS_VALUE_FORBID = 0x6,             /* Value forbidden */
} i2c_slave_clientErrorCode;

typedef enum {
    I2CM_OK = 0x0,
    I2CM_BUSFAULT = 0x1,
    I2CM_NO_ACK = 0x2,
    I2CM_TIMEOUT = 0x3,
    I2CM_BAD_MSG = 0x4,
    I2CM_LOST_ARB = 0x5,
} i2cmErr_t;

/***********************************************************************
 * GLOBAL Functions:   Declaration of global functions
***********************************************************************/
void etsocCmdHandlerInitialize(void);
void etsocCmdHandlerTask(void *pvParameters);
int milliVoltToHexCode(ETSOCVirtualRegisterIdx_t regIdx, uint32_t mV, uint8_t *p_hexCode);
int writeRegulatorReg(ETSOCVirtualRegisterIdx_t regIdx, uint32_t data);
int setAllRegulatorsToDefaultVoltage(void);
uint32_t getDefaultRegulatorMilliVoltValue(ETSOCVirtualRegisterIdx_t regIdx);
uint32_t getMinRegulatorMilliVoltValue(ETSOCVirtualRegisterIdx_t regIdx);
uint32_t getMaxRegulatorMilliVoltValue(ETSOCVirtualRegisterIdx_t regIdx);
uint16_t getRegulatorCodeConversionDelta(ETSOCVirtualRegisterIdx_t regIdx);
int setRegisterValue(ETSOCVirtualRegisterIdx_t regIdx, uint32_t value);
int getRegisterValue(ETSOCVirtualRegisterIdx_t regIdx, uint32_t *data);
void *getRegisterAddress(ETSOCVirtualRegisterIdx_t regIdx);
ETSOCVirtualRegisterIdx_t getRegisterIndexFromSpCmdIndex(spCommandIndex_t cmdIdx);
bool getSpCmdReadData(spCommandIndex_t cmdIdx, uint32_t *data, uint32_t *numberOfBytes);
int getPowerAlarmSetPoint(uint32_t *data);
bool isInterruptEnabled(interruptController_t intrType);
int setInterruptCause(interruptController_t intrType, uint32_t data);
void triggerETSOCInterrupt(void);
void configureETSOCInterruptsToDefault(void);
void setResetCauseCruSysReset(void);

void i2csSetHandlerError(uint8_t regAddr, i2c_slave_handlerErrorCode code);
void i2csSetClientError(uint8_t regAddr, i2c_slave_clientErrorCode code);
void setRegulatorCommFailError(uint8_t regulatorAddr, uint8_t cmdRegAddr, i2cmErr_t code);

// Used in CLITask, to be removed or become local eventually
bool isAccessTypeReadOnly(spCommandIndex_t cmdIdx);
bool isAccessTypeWriteOnly(spCommandIndex_t cmdIdx);
bool isCleanAfterReadRequired(spCommandIndex_t cmdIdx);
uint32_t getNumberOfBytes(spCommandIndex_t cmdIdx);
const char *getCmdName(spCommandIndex_t cmdIdx);
void handlePostReadUpdate(spCommandIndex_t cmdIdx);

#endif // __ETSOCCMDHANDLERTASK_H__
