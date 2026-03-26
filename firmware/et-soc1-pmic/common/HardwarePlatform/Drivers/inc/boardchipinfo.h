/***********************************************************************
 *
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *
 ************************************************************************/

/***********************************************************************/
/*! \file boardchipinfo.h
    \brief TBD
*/
/***********************************************************************/

#ifndef BOARDCHIPINFO_H
#define BOARDCHIPINFO_H

#include "FS1406.h"
#include "LTM4680.h"
#include "TPSM8D6C24.h"
#include "MAX6660.h"
#include "TPS53681.h"
#include "pmbus.h"
#include "PCA9575.h"

typedef struct {
    char const *chipName;
    uint16_t numRegIdx;
    uint8_t addr7;
    t_structRegInfo const *pStructRegInfo;
    t_structFieldInfo const *pStructFieldInfo;
} t_structChipInfo;

extern t_structChipInfo const structChipInfo[];

typedef struct {
    uint8_t addr, realAddr, pageReg, pageVal;
    uint8_t *pCurPageVal;
} t_pageXlate;

extern const t_pageXlate pageXlate[];
extern uint8_t curPageVal[];

enum {
    DDR,
    MXN,
    QLP,
    Q,
    eID_FS1406_LOGIC,
    eID_FS1406_PCI,
    eID_FS1406_1P8V,
    eID_LTM4680,
    eID_LTM4680p0,
    eID_LTM4680p1,
    eID_TPSM8D6C24,
    eID_TPSM8D6C24p0,
    eID_TPSM8D6C24p1,
    eID_TPS53681,
    eID_TPS53681_MNN,
    eID_TPS53681_NOC,
    eID_MAX6660,
    eID_PCA9575_0,
    eID_PCA9575_1,
    MAX_CHIPS
};

enum {
    ADR_DDR = 0x08,
    ADR_MXN = 0x09,
    ADR_QLP = 0x0A,
    ADR_Q = 0x0B,
    ADR_LOGIC = 0x88,
    ADR_PCI = 0x89,
    ADR_1P8V = 0x8A,
    ADR_SRAM = 0xC1,
    ADR_SRAMP0 = 0xC2,
    ADR_SRAMP1 = 0xC3,
    ADR_SRAM_TPSM8D6C24 = 0x94,
    ADR_SRAMP0_TPSM8D6C24 = 0x95,
    ADR_SRAMP1_TPSM8D6C24 = 0x96,
    ADR_TPS53681 = 0x58,
    ADR_MNN = 0x59,
    ADR_NOC = 0x5A,
    ADR_MAX6660 = 0x98,
    ADR_PCA9575_0 = 0x20,
    ADR_PCA9575_1 = 0xA0,
};

#ifdef INSTANTIATE_DEVINFO
// clang-format off
t_structChipInfo const structChipInfo[MAX_CHIPS] = {
    [DDR]              = { "FS1406_DDR",         MAXREG_FS1406, ADR_DDR,               structRegInfo_FS1406, PMBus1_2_FieldInfo },
    [MXN]              = { "FS1406_MXN",         MAXREG_FS1406, ADR_MXN,               structRegInfo_FS1406, PMBus1_2_FieldInfo },
    [QLP]              = { "FS1406_QLP",         MAXREG_FS1406, ADR_QLP,               structRegInfo_FS1406, PMBus1_2_FieldInfo },
    [Q]                = { "FS1406_Q",           MAXREG_FS1406, ADR_Q,                 structRegInfo_FS1406, PMBus1_2_FieldInfo },
    [eID_FS1406_LOGIC] = { "FS1406_LOGIC",       MAXREG_FS1406, ADR_LOGIC,             structRegInfo_FS1406, PMBus1_2_FieldInfo },
    [eID_FS1406_PCI]   = { "FS1406_PCI",         MAXREG_FS1406, ADR_PCI,               structRegInfo_FS1406, PMBus1_2_FieldInfo },
    [eID_FS1406_1P8V]  = { "FS1406_1P8V",        MAXREG_FS1406, ADR_1P8V,              structRegInfo_FS1406, PMBus1_2_FieldInfo },
    [eID_LTM4680]      = { "LTM4680_SRAMALL",    PMBus_MAXREG,  ADR_SRAM,              PMBus_RegInfo,        PMBus1_2_FieldInfo },
    [eID_LTM4680p0]    = { "LTM4680_SRAMP0",     PMBus_MAXREG,  ADR_SRAMP0,            PMBus_RegInfo,        PMBus1_2_FieldInfo },
    [eID_LTM4680p1]    = { "LTM4680_SRAMP1",     PMBus_MAXREG,  ADR_SRAMP1,            PMBus_RegInfo,        PMBus1_2_FieldInfo },
    [eID_TPSM8D6C24]   = { "TPSM8D6C24_SRAMALL", PMBus_MAXREG,  ADR_SRAM_TPSM8D6C24,   PMBus_RegInfo,        PMBus1_3_FieldInfo },
    [eID_TPSM8D6C24p0] = { "TPSM8D6C24_SRAMP0",  PMBus_MAXREG,  ADR_SRAMP0_TPSM8D6C24, PMBus_RegInfo,        PMBus1_3_FieldInfo },
    [eID_TPSM8D6C24p1] = { "TPSM8D6C24_SRAMP1",  PMBus_MAXREG,  ADR_SRAMP1_TPSM8D6C24, PMBus_RegInfo,        PMBus1_3_FieldInfo },
    [eID_TPS53681]     = { "TPS53681PALL",       PMBus_MAXREG,  ADR_TPS53681,          PMBus_RegInfo,        PMBus1_3_FieldInfo },
    [eID_TPS53681_MNN] = { "TPS53681_MNN",       PMBus_MAXREG,  ADR_MNN,               PMBus_RegInfo,        PMBus1_3_FieldInfo },
    [eID_TPS53681_NOC] = { "TPS53681_NOC",       PMBus_MAXREG,  ADR_NOC,               PMBus_RegInfo,        PMBus1_3_FieldInfo },
};
// clang-format on

// clang-format off
t_pageXlate const pageXlate[] = {
    { 0x58, 0x58, 0x00, 0xFF, curPageVal + 0 }, // both
    { 0x59, 0x58, 0x00, 0x00, curPageVal + 0 }, // MNN
    { 0x5A, 0x58, 0x00, 0x01, curPageVal + 0 }, // NOC
    { 0xC1, 0xC1, 0x00, 0xFF, curPageVal + 1 }, // both
    { 0xC2, 0xC1, 0x00, 0x00, curPageVal + 1 }, // chan0
    { 0xC3, 0xC1, 0x00, 0x01, curPageVal + 1 }, // chan1
    { 0x94, 0x94, 0x04, 0xFF, curPageVal + 1 }, // both
    { 0x95, 0x94, 0x04, 0x00, curPageVal + 1 }, // SRAM P0
    { 0x96, 0x94, 0x04, 0x01, curPageVal + 1 }, // SRAM P1
    { 0 }
};
// clang-format on

uint8_t curPageVal[2] = { 0x80, 0x80 }; // init to never used page number

#endif /* INSTANTIATE_DEVINFO */

#endif /* BOARDCHIPINFO_H */
