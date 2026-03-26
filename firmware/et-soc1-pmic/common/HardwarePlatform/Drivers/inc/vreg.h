/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file vreg.h
    \brief Vreg.
*/

#ifndef VREG_H
#define VREG_H

#include "pmbstats.h"

enum { ECT_TPS53681_VID, ECT_FS1406, ECT_L16, ECT_L16n9 };

#define NO_I2C   0xFF
#define NO_REG   0xFF
#define LNG_MASK 0x07

typedef struct {
    char const *id;
    char const *desc;
    uint8_t voltageReguId;
    uint8_t chipType;
    uint8_t reg[2];
    uint8_t i2clng;
    ETSOCVirtualRegisterIdx_t etsocVirtualRegIdx;
    uint8_t enablePinId;
    uint8_t baseNomCode;
    int32_t stepnV;
    int32_t baseuV;
} voutInfo_t;

typedef struct {
    voutInfo_t const *voutInfo;
    uint16_t voutRegCurVal;
    bool voutDisabled;
} vout_t;

extern voutInfo_t const voutInfoTbl[];
extern vout_t vout[];
extern bool anyVoutDisabled;

int32_t rawToL11FmtX1k(uint16_t raw);
uint32_t rawToL16FmtX1k(uint16_t raw);
uint16_t rawToMvL16(uint16_t raw);
uint16_t rawToMvL16n9(uint16_t raw);
uint16_t multiRawToMv(uint16_t regVal, voutInfo_t const *p);
uint16_t multiMvToRaw(uint16_t mV, voutInfo_t const *p);

i2cmErr_t reguRegRead(uint8_t chipaddr, uint8_t regNum, uint8_t lng, uint16_t *pVal);
i2cmErr_t reguRegWrite(uint8_t chipaddr, uint8_t regNum, uint8_t lng, uint16_t val);
bool *findVoutDisableByPinId(gpioId_t pinId);
vout_t *findId(char const *id);
vout_t *findReguInfoByRegIdx(ETSOCVirtualRegisterIdx_t idx);

//#define params3( nomCode, step, dflt) ( 40 + ( (250 + nomCode*5)*1000 - dflt +step/2)/step ), step, dflt

// FS1406
#define RTOP            4.02
#define RFACTOR(rBot)   ((RTOP + rBot) / rBot)
#define STEP(rBot)      ((uint32_t)(.955 * RFACTOR(rBot) * 5000000))        // nV
#define DFLT(rBot)      ((uint32_t)(.955 * RFACTOR(rBot) * 600000 + 27000)) // uV

enum {
    eIdx_QLP = 0,
    eIdx_SRM,
    eIdx_DDR,
    eIdx_DDQ,
    eIdx_PCL,
    eIdx_PCI,
    eIdx_MXN,
    eIdx_NOC,
    eIdx_MNN,
    eIdx_SRM_TPSM8D6C24,
    eIdx_MaxCount
};
#ifdef VREG_INSTANTIATE
// clang-format off
voutInfo_t const voutInfoTbl[] = { 
    [eIdx_QLP] =
    { 
        .id = "QLP",
        .desc = "V0P6 DDQLP",
        .voltageReguId = QLP,
        .chipType = ECT_FS1406,
        .reg = { FS1406__VOUT_HIGH, NO_REG },
        .i2clng = 9,
        .etsocVirtualRegIdx = DDQLP_VOLTAGE,
        .enablePinId = QLP_EN_OUT,
        .baseNomCode = 40,
        .stepnV = STEP(115),
        .baseuV = DFLT(115),
    }, // .620
    [eIdx_SRM] =
    {
        .id = "SRM",
        .desc = "SOC SRAM",
        .voltageReguId = eID_LTM4680,
        .chipType = ECT_L16,
        .reg = { PMBUS__VOUT_COMMAND, NO_REG },
        .i2clng = 2,
        .etsocVirtualRegIdx = L2_CACHE_VOLTAGE,
        .enablePinId = SRAM_EN_OUT,
    }, // .675
    [eIdx_SRM_TPSM8D6C24] =
    {
        .id = "SRM",
        .desc = "SOC SRAM",
        .voltageReguId = eID_TPSM8D6C24,
        .chipType = ECT_L16n9,
        .reg = { PMBUS__VOUT_COMMAND, NO_REG },
        .i2clng = 2,
        .etsocVirtualRegIdx = L2_CACHE_VOLTAGE,
        .enablePinId = SRAM_EN_OUT,
    }, // .675
    [eIdx_DDR] =
    { 
        .id = "DDR",
        .desc = "SOC DDR",
        .voltageReguId = DDR,
        .chipType = ECT_FS1406,
        .reg = { FS1406__VOUT_HIGH, NO_REG },
        .i2clng = 9,
        .etsocVirtualRegIdx = DDR_VOLTAGE,
        .enablePinId = DDR_EN_OUT,
        .baseNomCode = 40,
        .stepnV = STEP(9.2),
        .baseuV = DFLT(9.2), 
    }, // .850
    [eIdx_DDQ] =
    {
        .id = "DDQ",
        .desc = "V1P1 DDR VDDQ",
        .voltageReguId = Q,
        .chipType = ECT_FS1406,
        .reg = { FS1406__VOUT_HIGH, NO_REG },
        .i2clng = 9,
        .etsocVirtualRegIdx = DDQ_VOLTAGE,
        .enablePinId = Q_EN_OUT,
        .baseNomCode = 40,
        .stepnV = STEP(4.64),
        .baseuV = DFLT(4.64), 
    }, // 1.1
    [eIdx_PCL] =
    {
        .id = "PCL",
        .desc = "V0P75 PCIE LGC",
        .voltageReguId = eID_FS1406_LOGIC,
        .chipType = ECT_FS1406,
        .reg = { FS1406__VOUT_HIGH, NO_REG },
        .i2clng = 9,
        .etsocVirtualRegIdx = PCIE_LOGIC_VOLTAGE,
        .enablePinId = LOGIC_EN_OUT,
        .baseNomCode = 40,
        .stepnV = STEP(13.2),
        .baseuV = DFLT(13.2), 
    }, // .775
    [eIdx_PCI] =
    { 
        .id = "PCI",
        .desc = "V1P5 PCIE",
        .voltageReguId = eID_FS1406_PCI,
        .chipType = ECT_FS1406,
        .reg = { FS1406__VOUT_HIGH, NO_REG },
        .i2clng = 9,
        .etsocVirtualRegIdx = PCIE_VOLTAGE,
        .enablePinId = PCIE_EN_OUT,
        .baseNomCode = 40,
        .stepnV = STEP(2.55),
        .baseuV = DFLT(2.55), 
    }, // 1.5
    [eIdx_MXN] =
    {
        .id = "MXN",
        .desc = "SOC MAXION",
        .voltageReguId = MXN,
        .chipType = ECT_FS1406,
        .reg = { FS1406__VOUT_HIGH, NO_REG },
        .i2clng = 9,
        .etsocVirtualRegIdx = MAXION_VOLTAGE,
        .enablePinId = MXN_EN_OUT,
        .baseNomCode = 40,
        .stepnV = STEP(9.2),
        .baseuV = DFLT(9.2), 
    }, // .850
    [eIdx_NOC] =
    {
        .id = "NOC",
        .desc = "SOC NOC",
        .voltageReguId = eID_TPS53681_NOC,
        .chipType = ECT_TPS53681_VID,
        .reg = { PMBUS__VOUT_COMMAND, NO_REG },
        .i2clng = 2,
        .etsocVirtualRegIdx = NOC_VOLTAGE,
        .enablePinId = NOC_EN_OUT,
    }, // .400
    [eIdx_MNN] =
    {
        .id = "MNN",
        .desc = "SOC MNN",
        .voltageReguId = eID_TPS53681_MNN,
        .chipType = ECT_TPS53681_VID,
        .reg = { PMBUS__VOUT_COMMAND, NO_REG },
        .i2clng = 2,
        .etsocVirtualRegIdx = ALL_MINION_VOLTAGE,
        .enablePinId = MNN_EN_OUT,
    }, // .400
};
// clang-format on
bool anyVoutDisabled;
#endif

#endif /* VREG_H */
