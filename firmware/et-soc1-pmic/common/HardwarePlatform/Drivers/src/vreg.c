/***********************************************************************
 *
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *
 ************************************************************************/

/***********************************************************************/
/*! \file vreg.c
    \brief Vreg.
*/

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "CLITask.h"
#include "boardchipinfo.h"
#include "chips.h"
#include "cli.h"
#include "gpio.h"
#include "i2cm2.h"
#define VREG_INSTANTIATE
#include "IoTask.h"
#include "chipio.h"
#include "etsoc_cmd_handler_task.h"
#include "vreg.h"
#include "hw_encoding.h"

#define TPS53681_REG_VID_BASE_mV  245 //for DAC step 5mv
#define TPS53681_REG_VID_DELTA_mV 5   //for DAC step 5mv

static uint16_t regTps53681ToMv(uint16_t code)
{
    if (code == 0)
    {
        return 0;
    }
    else
    {
        return TPS53681_REG_VID_BASE_mV + (uint16_t)((uint32_t)code * TPS53681_REG_VID_DELTA_mV);
    }
}

static uint16_t mVtoRegTps53681(uint32_t mV)
{
    if (mV == 0)
    {
        return 0;
    }
    else
    {
        return (uint16_t)((mV - TPS53681_REG_VID_BASE_mV) / TPS53681_REG_VID_DELTA_mV);
    }
}

static uint32_t regFs1406ToMv(uint32_t regVal, voutInfo_t const *p)
{
    return (p->baseuV + (((int16_t)(regVal - (p->baseNomCode)) * p->stepnV) / 1000 + 500L)) / 1000L;
}

static uint32_t mVtoRegFS1406(uint32_t mV, voutInfo_t const *p)
{
    int32_t t = mV * 1000000L - p->baseuV * 1000;
    // integer division truncates towards 0, not in a consistent direction
    t = t >= 0 ? ((t + p->stepnV / 2) / p->stepnV) : -(((-t) + p->stepnV / 2) / p->stepnV);
    return p->baseNomCode + t;
}

int32_t rawToL11FmtX1k(uint16_t raw)
{
    int16_t volatile mant =
        (raw &
            0x7FF); // due to the fact all numbers shall be positive. This is to workaround an aggregated negative current issue
    int8_t volatile exp = (int8_t)(((int16_t)raw) >> 11);
    int64_t volatile t = ((int32_t)mant) << (16 + exp); // exp usually negative
    t *= 1000;
    t >>= 16;
    return (int32_t)t;
}

uint32_t rawToL16FmtX1k(uint16_t raw)
{
    return (raw * 1000UL + 2048UL) >> 12;
}

uint16_t rawToMvL16(uint16_t raw)
{
    return (uint16_t)rawToL16FmtX1k(raw);
}

static uint16_t mVToRawL16(uint32_t mV)
{
    return (uint16_t)(((mV << 12) + 500UL) / 1000UL);
}

uint16_t rawToMvL16n9(uint16_t raw)
{
    return (raw * 1000UL + (1UL << 8)) >> 9;
}

static uint16_t mVToRawL16n9(uint32_t mV)
{
    return (uint16_t)(((mV << 9) + 500UL) / 1000UL);
}

uint16_t multiRawToMv(uint16_t regVal, voutInfo_t const *p)
{
    switch (p->chipType)
    {
        case ECT_TPS53681_VID:
            return regTps53681ToMv(regVal);
        case ECT_FS1406:
            return (uint16_t)regFs1406ToMv(regVal, p);
        case ECT_L16:
            return rawToMvL16(regVal);
        case ECT_L16n9:
            return rawToMvL16n9(regVal);
        default:
            return 0;
    }
    return 0;
}

uint16_t multiMvToRaw(uint16_t mV, voutInfo_t const *p)
{
    switch (p->chipType)
    {
        case ECT_TPS53681_VID:
            return mVtoRegTps53681(mV);
        case ECT_FS1406:
            return (uint16_t)mVtoRegFS1406(mV, p);
        case ECT_L16:
            return mVToRawL16(mV);
        case ECT_L16n9:
            return mVToRawL16n9(mV);
        default:
            return 0;
    }
    return 0;
}

i2cmErr_t reguRegRead(uint8_t chipaddr, uint8_t regNum, uint8_t lng, uint16_t *pVal)
{
    i2cmErr_t rv = 0;
    uint8_t regval8;
    uint16_t regval16;
    switch (lng)
    {
        case 1:
            rv = i2cm_read8(chipaddr, regNum, &regval8);
            *pVal = regval8;
            break;
        case 2:
            rv = i2cm_read16(chipaddr, regNum, &regval16);
            *pVal = regval16;
            break;
        case 9:
            rv = i2cm_read8(chipaddr, regNum, &regval8);
            *pVal = ((uint16_t)regval8) << 8;
            rv = rv ? rv : i2cm_read8(chipaddr, regNum + 1, &regval8);
            *pVal |= regval8;
            break;
        default:
            assertMsg(0, "bad case");
    }
    return rv;
}

i2cmErr_t reguRegWrite(uint8_t chipaddr, uint8_t regNum, uint8_t lng, uint16_t val)
{
    i2cmErr_t rv = 0;
    switch (lng)
    {
        case 1:
            rv = i2cm_write8(chipaddr, regNum, (uint8_t)val);
            break;
        case 2:
            rv = i2cm_write16(chipaddr, regNum, val);
            break;
        case 9:
            rv = i2cm_write8(chipaddr, regNum, (uint8_t)(val >> 8));
            rv = rv ? rv : i2cm_write8(chipaddr, regNum + 1, (uint8_t)val);
            break;
        default:
            assertMsg(0, "bad case");
    }
    return rv;
}

void cmdListv(void)
{
    voutInfo_t const *p;
    interruptingPrintBegin();
    interruptingPrintf("    Some regulators cannot produce listed command voltages.\n");
    interruptingPrintf("    Curr value is nearest possible value in those cases.\n");
    interruptingPrintf("    All voltages in millivolts\n");
    interruptingPrintf("%-5s  %-20s %4s %4s %4s %4s %5s\n", "ID", "Description", "CurV", "Nom", "Min", "Max", "Step");
    for (vout_t *pv = vout; (p = pv->voutInfo); ++pv)
    {
        t_structChipInfo const *pChip = p->voltageReguId == NO_I2C ? NULL : &structChipInfo[p->voltageReguId];
        uint16_t regVal = 0, mV;
        bool enabled = !pv->voutDisabled;

        if (pChip)
        {
            uint8_t chipaddr = pChip->addr7;
            i2cmErr_t rv = reguRegRead(chipaddr, p->reg[0], p->i2clng, &regVal);
            if (rv == I2CM_OK)
            {
                mV = multiRawToMv(regVal, p);
            }
            else
            {
                mV = 0xFFFF;
            }
        }
        else
        {
            mV = (uint16_t)(regFs1406ToMv(0, p));
        }

        uint16_t delta = getRegulatorCodeConversionDelta(p->etsocVirtualRegIdx);
        interruptingPrintf("%-5s  %-20s %4d %4d %4d %4d %2d.%02d\n", p->id, p->desc, enabled ? mV : 0,
            getDefaultRegulatorMilliVoltValue(p->etsocVirtualRegIdx),
            getMinRegulatorMilliVoltValue(p->etsocVirtualRegIdx), getMaxRegulatorMilliVoltValue(p->etsocVirtualRegIdx),
            delta / 1000, (delta % 1000) / 10);
    }
    if (!globals.commonData.powerState.hwUp)
    {
        interruptingPrintf("\n!POWER IS OFF!\n");
    }
    interruptingPrintNL();
    printPmbStats();
    analogData_t const *pad = &globals.commonData.analogData;
    uint32_t mv = pad->vmon12vMV;
    uint32_t ma = pad->imon12vMA;
    uint32_t mw = pad->instmW;
    uint32_t mwAv = (uint32_t)(pad->avemW >> 32);

    interruptingPrintf("Inputs: Volts: %2d.%03dV, Amps: %2d.%03dA, Watts: %2d.%03dW, Average Watts: %2d.%03dW\n",
        mv / 1000, mv % 1000, ma / 1000, ma % 1000, mw / 1000, mw % 1000, mwAv / 1000, mwAv % 1000);
    interruptingPrintEnd();
}

// Helper function to read voutInfo and print the result
static uint16_t readVoutInfo(const vout_t *pv)
{
    voutInfo_t const *p = pv->voutInfo;
    t_structChipInfo const *pChip = p->voltageReguId == NO_I2C ? NULL : &structChipInfo[p->voltageReguId];
    i2cmErr_t rv;
    uint16_t regVal = 0;
    uint16_t mV = 0;

    interruptingPrintf("%-5s  %-16s : ", p->id, p->desc);

    if (pChip)
    {
        for (uint8_t i = 0; i < countof(p->reg); ++i)
        {
            if (p->reg[i] == NO_REG)
            {
                continue;
            }
            rv = reguRegRead(pChip->addr7, p->reg[i], p->i2clng, &regVal);
            if (rv == I2CM_OK)
            {
                mV = multiRawToMv(regVal, p);
                interruptingPrintf(i == 0 ? "%4d mV" : "  (%4d mV)", mV);
            }
            else
            {
                i2cPErrorOL(rv, "read reg");
            }
        }
    }
    else
    {
        interruptingPrintf("  FIXED");
    }

    if (pv->voutDisabled)
    {
        interruptingPrintf(" OFF");
    }

    interruptingPrintf("\n");

    return mV;
}

uint16_t cmdReadv(const char *id)
{
    vout_t *pv = vout;
    bool all = 1;
    uint16_t mV = 0;

    if (id)
    {
        pv = findId(id);
        if (!pv)
        {
            interruptingPrintfOneLine("Id not found. Use l command to see Ids\n");
            return 0;
        }
        all = 0;
    }

    interruptingPrintBegin();

    while (1)
    {
        mV = readVoutInfo(pv);
        pv++;
        if (!all || !(pv->voutInfo))
            break;
    }

    if (!globals.commonData.powerState.hwUp)
    {
        interruptingPrintf("\n!POWER IS OFF!\n");
    }

    interruptingPrintEnd();

    return mV;
}

bool cmdSetv(const char *id, const char *mvstr, const powerState_t *pps)
{
    vout_t *pv = NULL;
    voutInfo_t const *p = NULL;
    uint32_t mV = -1;
    bool ok = false;
    bool enableRegulator = false;

    interruptingPrintBegin();
    do
    {
        if (!id)
        {
            interruptingPrintf("Id required. Use l command to see Ids.\n");
            break;
        }
        pv = findId(id);
        if (!pv)
        {
            interruptingPrintf("Id not found.  Use l command to see Ids\n");
            break;
        }
        p = pv->voutInfo;

        if (strcmp(mvstr, "OFF") == 0)
        {
            enableRegulator = true;
            if (pps->hwUp)
                gpio_set_out_low_pin_id(p->enablePinId);
            pv->voutDisabled = 1;
            anyVoutDisabled = 1;
            if (strcmp("MNN", p->id) == 0)
                pv[-1].voutDisabled = 1;
            if (strcmp("NOC", p->id) == 0)
                pv[1].voutDisabled = 1;
            break;
        }
        else if (strcmp(mvstr, "ON") == 0)
        {
            enableRegulator = true;
            if (pps->hwUp)
                gpio_set_out_high_pin_id(p->enablePinId);
            pv->voutDisabled = 0;
            if (strcmp("MNN", p->id) == 0)
                pv[-1].voutDisabled = 0;
            if (strcmp("NOC", p->id) == 0)
                pv[1].voutDisabled = 0;
            anyVoutDisabled = 0;
            for (vout_t *pv2 = vout; pv2->voutInfo; ++pv2)
                anyVoutDisabled |= pv2->voutDisabled;
            break;
        }
        else
        {
            if (p->voltageReguId == NO_I2C)
            {
                interruptingPrintf("%s cannot be changed.\n", p->id);
                break;
            }

            mV = atodec(mvstr, &ok);
            if (!ok)
            {
                interruptingPrintf("Bad/missing mV\n");
                break;
            }
            else if (mV < getMinRegulatorMilliVoltValue(p->etsocVirtualRegIdx) ||
                     mV > getMaxRegulatorMilliVoltValue(p->etsocVirtualRegIdx))
            {
                interruptingPrintf("mV is out of bounds.\n");
                ok = false;
                break;
            }
        }

        ok = true;

    } while (0);
    interruptingPrintEnd();

    if (ok && !enableRegulator)
    {
        uint8_t hexCode = 0xff;
        if (milliVoltToHexCode(p->etsocVirtualRegIdx, mV, &hexCode) != STATUS_SUCCESS)
        {
            printfFromInterrupt("Error converting mV to hex code: regIdx 0x%02X, mV 0x%02X", p->etsocVirtualRegIdx, mV);
            return false;
        }

        int status = writeRegulatorReg(p->etsocVirtualRegIdx, hexCode);
        if (status != STATUS_SUCCESS)
        {
            ok = false;
        }
    }

    return ok;
}

bool cmdIntSetv(const char *id, const uint16_t mV)
{
    vout_t const *pv = NULL;
    voutInfo_t const *p = NULL;
    bool ok = false;

    pv = findId(id);
    if (!pv)
    {
        interruptingPrintfOneLine("Id (%s) not found\n", id);
        return false;
    }

    p = pv->voutInfo;

    if (mV < getMinRegulatorMilliVoltValue(p->etsocVirtualRegIdx) ||
        mV > getMaxRegulatorMilliVoltValue(p->etsocVirtualRegIdx))
    {
        interruptingPrintfOneLine("mV is out of bounds.\n");
        return false;
    }

    uint8_t hexCode = 0xff;
    if (milliVoltToHexCode(p->etsocVirtualRegIdx, mV, &hexCode) != STATUS_SUCCESS)
    {
        printfFromInterrupt("Error converting mV to hex code: regIdx 0x%02X, mV 0x%02X", p->etsocVirtualRegIdx, mV);
        return false;
    }

    int status = writeRegulatorReg(p->etsocVirtualRegIdx, hexCode);

    if (status != STATUS_SUCCESS)
    {
        ok = false;
    }
    return ok;
}

enum {
    E_RF_V_I_VIN = 1,        // readV, readI, readVin
    E_RF_MORE_UNIT_REGS = 2, // more units regs
    E_RF_ALL_UNIT_REGS = 3,  // all units regs
    E_RF_ALL_REGS = 4,       // all regs
    E_RF_ALL_REGS_RAW = 5,   // all regs
};

#define ASSERT_CMD(condition, errorMessage)   \
    do                                        \
    {                                         \
        if ((condition))                      \
        {                                     \
            interruptingPrintf(errorMessage); \
            interruptingPrintEnd();           \
            return false;                     \
        }                                     \
    } while (0);

bool cmdRPmb(const char *id, const char *flagStr)
{
    uint8_t flag;
    uint8_t chipaddr = 0;
    i2cmErr_t rv;
    uint8_t isSram = 0;
    bool ok = false;
    bool status = false;
    interruptingPrintBegin();

    ASSERT_CMD((id == NULL), "Id required. Use l command to see Ids.\n")

    if (strncmp(id, "SRM", 3) == 0)
    {
        if (HW_ENCODING_PCIE_1088 == Hw_Encoding_Get_Hw_Version_Encoding())
        {
            chipaddr = (id[3] == '0') ? structChipInfo[eID_TPSM8D6C24p0].addr7 :
                                        (id[3] == '1') ? structChipInfo[eID_TPSM8D6C24p1].addr7 : 0;
        }
        else
        {
            chipaddr = (id[3] == '0') ? structChipInfo[eID_LTM4680p0].addr7 :
                                        (id[3] == '1') ? structChipInfo[eID_LTM4680p1].addr7 : 0;
        }

        isSram = 1;
    }

    if (chipaddr == 0)
    {
        voutInfo_t const *p;
        vout_t *pv = findId(id);
        ASSERT_CMD((id == NULL), "Id required. Use l command to see Ids.\n")

        p = pv->voutInfo;
        chipaddr = structChipInfo[p->voltageReguId].addr7;
        ASSERT_CMD((p->voltageReguId == eID_TPS53681_MNN && p->voltageReguId == eID_TPS53681_NOC),
            "Not PMB chip.  Only NOC, MNN, SRM0 and SRM1 supported\n")
    }

    flag = atodec(flagStr, &ok);
    if (!ok || flag < E_RF_V_I_VIN || flag > E_RF_ALL_REGS_RAW)
    {
        interruptingPrintf(
            "Flag 1: readV, readI, readVin\n     2: more units regs\n     3: all units regs\n     4: all "
            "regs\n     5: all regs raw\n");
        flag = E_RF_V_I_VIN;
    }

    // clang-format off
    static uint32_t const flag1regs[2][8] = { { 0, 0x0002, 0, 0, 0x1900, 0, 0x00100000, }, { 0, 0x0002, 0, 0, 0x1900, 0, 0x00100000, } };
    static uint32_t const flag2regs[2][8] = { { 0, 0, 0, 0, 0x00C03B00, 0, 0x00100000, }, { 0, 0, 0, 0, 0x00C03B00, 0, 0x00100000, } };
    static uint32_t const vidL16Fmt[2][8] = { { 0, 0x0872, 0x0011, 0, 0x0800, 0, 0x08000000, }, { 0, 0x872, 0x1D, 0, 0x800, 0x20, 0x20000000, 0 } };
    static uint32_t const l11Fmt[2][8] = { { 0, 0x03280780, 0x2A228440, 0x0801, 0x00C03300, 0, 0x00100000, }, { 0, 0x680080, 0x212A8440, 0x877, 0xE07300, 0, 0xDC800000, 0x2900112 } };
    static uint32_t const unitVolts[2][8] = { { 0, 0x00200FF2, 0x02200011, 0, 0x0900, 0x08100000, 0x08100000, }, { 0, 0x600072, 0x120001D, 0, 0x900, 0x20, 0x60000000, } };
    static uint32_t const unitAmps[2][8] = { { 0, 0x03000000, 0x28000440, 0, 0x1200, }, { 0, 0, 0x20000440, 0, 0x1200, 0, 0x800000, 0x12 } };
    static uint32_t const unitWatts[2][8] = { { 0, 0, 0, 0x0800, 0x00C00000, }, { 0, 0, 0, 0, 0xC00000, } };
    static uint32_t const unitDegrees[2][8] = { { 0, 0, 0x00028000, 0x2000, 0x2000, }, { 0, 0, 0xA8000, 0, 0x6000, 0, 0x80000000, 0x2100000 } };
    static uint32_t const unitSeconds[2][8] = { { 0, 0, 0, 0x0001, }, { 0, 0, 0, 0x77, 0, 0, 0x18000000, 0 } };
    static uint32_t const unitkHz[2][8] = { { 0, 0x00080000, }, { 0, 0x80000, 0, 0, 0x200000, } };
    static uint32_t const unitmOhm[2][8] = { { 0, }, { 0, 0, 0, 0, 0, 0, 0x4000000, 0x800100 } };
    static uint32_t const unitVpmS[2][8] = { { 0, }, { 0, 0x80, } };
    // clang-format on

    t_structRegInfo const *pr = PMBus_RegInfo;
    uint8_t reg = 0;
    while (1)
    {
        do
        {
            reg = pr->regnum;
            if (isSram && (pr->regnum >= 0xD0 && pr->regnum <= 0xEC))
                continue;
            uint8_t props = xlatRTtoProp[pr->epType];
            if (!((props & (eP_8Bit | eP_16Bit)) && (props & eP_Read)))
                continue;

            uint8_t i = reg >> 5;
            uint32_t m = 1 << (reg & 0x1F);
            char const *units = (unitVolts[isSram][i] & m) ?
                                    "Volts" :
                                    (unitAmps[isSram][i] & m) ?
                                    "Amps" :
                                    (unitWatts[isSram][i] & m) ?
                                    "Watts" :
                                    (unitDegrees[isSram][i] & m) ?
                                    "Deg. C" :
                                    (unitSeconds[isSram][i] & m) ?
                                    "mSec" :
                                    (unitkHz[isSram][i] & m) ?
                                    "kHz" :
                                    (unitmOhm[isSram][i] & m) ? "mOhm" : (unitVpmS[isSram][i] & m) ? "V/mS" : NULL;
            ok = 0;
            switch (flag)
            {
                case E_RF_V_I_VIN:
                    ok = (flag1regs[isSram][i] & m) != 0;
                    break;
                case E_RF_MORE_UNIT_REGS:
                    ok = (flag2regs[isSram][i] & m) != 0;
                    break;
                case E_RF_ALL_UNIT_REGS:
                    ok = units != NULL;
                    break;
                default: /* case E_RF_ALL_REGS and E_RF_ALL_REGS_RAW */
                    ok = 1;
                    break;
            }
            if (!ok)
                break;

            uint16_t val;
            if (props & eP_8Bit)
            {
                uint8_t val8;
                rv = i2cm_read8(chipaddr, reg, &val8);
                val = val8;
            }
            else
                rv = i2cm_read16(chipaddr, reg, &val);

            if (rv == I2CM_OK)
            {
                if (flag < 5)
                {
                    interruptingPrintf("%-27s r=%02X, ", pr->regName, reg);
                    interruptingPrintf((props & eP_8Bit) ? "v=  %02X" : "v=%04X", val);
                }
                else
                {
                    interruptingPrintf("%02X %04X\n", reg, val);
                }
            }
            else
            {
                i2cPError(rv, "read reg");
                break;
            }
            if (flag == 5)
                continue;
            int32_t u = 999;
            uint32_t mu = 999;
            bool hasFmt = 0;
            if (vidL16Fmt[isSram][i] & m)
            {
                uint32_t t = !isSram ?
                                 (uint32_t)(val * 5 + 250) :
                                 (HW_ENCODING_PCIE_1088 == Hw_Encoding_Get_Hw_Version_Encoding()) ? rawToMvL16n9(val) :
                                                                                                    rawToL16FmtX1k(val);
                mu = t % 1000;
                u = t / 1000;
                hasFmt = 1;
            }
            else if (l11Fmt[isSram][i] & m)
            {
                int32_t t = rawToL11FmtX1k(val);
                bool s = t < 0;
                if (s)
                    t = -t;
                mu = t % 1000;
                u = t / 1000;
                if (s)
                    u = -u;
                hasFmt = 1;
            }
            if (hasFmt)
                interruptingPrintf(" %3d.%03u %s", u, mu, units);
            interruptingPrintNL();
        } while (0);
        /**
        The if statement should be as below. However, since TPSM8D6C24__MFR_FUSION_ID1 happens to be the same
        as LTM4680__MFR_RESET, it is considered as an error and the statement is then simplified.
        if (reg ==
            (isSram ? ((HW_ENCODING_PCIE_1088 == Hw_Encoding_Get_Hw_Version_Encoding()) ? TPSM8D6C24__MFR_FUSION_ID1 :
                                                                                          LTM4680__MFR_RESET) :
                      TPS53681__MFR_SPECIFIC_42))
            break;
        */
        if (reg == (isSram ? LTM4680__MFR_RESET : TPS53681__MFR_SPECIFIC_42))
            break;
        ++pr;
    }
    status = true;
    interruptingPrintEnd();

    return status;
}

vout_t *findId(char const *id)
{
    voutInfo_t const *p;
    for (vout_t *pv = vout; (p = pv->voutInfo); ++pv)
    {
        if (strcmp(id, p->id) == 0)
        {
            return pv;
        }
    }
    return NULL;
}

vout_t *findReguInfoByRegIdx(ETSOCVirtualRegisterIdx_t idx)
{
    voutInfo_t const *p;
    for (vout_t *pv = vout; (p = pv->voutInfo); ++pv)
    {
        if (p->etsocVirtualRegIdx == idx)
        {
            return pv;
        }
    }
    return NULL;
}

bool *findVoutDisableByPinId(gpioId_t pinId)
{
    voutInfo_t const *p;
    vout_t *pv;
    for (pv = vout; (p = pv->voutInfo); ++pv)
    {
        if (p->enablePinId == pinId)
            return &(pv->voutDisabled);
    }
    return NULL;
}

bool checkVoltageRegisters(void)
{
    bool ok = 1;
    bool prtd = 0;
    voutInfo_t const *p;
    vout_t *pv;

    for (pv = vout; ok && (pv->voutInfo != NULL); ++pv)
    {
        p = pv->voutInfo;
        if (p->voltageReguId == NO_I2C)
        {
            continue;
        }
        t_structChipInfo const *pChip = &structChipInfo[p->voltageReguId];
        uint8_t chipaddr = pChip->addr7;
        i2cmErr_t rv = 0;
        uint16_t regVal = 0;
        uint8_t iReg;

        for (iReg = 0; ok && iReg < 2; ++iReg)
        {
            uint8_t regNum = p->reg[iReg];
            if (regNum == NO_REG)
            {
                continue;
            }
            rv = reguRegRead(chipaddr, p->reg[0], p->i2clng, &regVal);
            if (rv != I2CM_OK)
            {
                ok = 0;
                i2cPErrorGrouped(&prtd, rv, "checkVoltageRegisters: ");
                interruptingPrintfGrouped(&prtd, "%s: read Reg %02X\n", pChip->chipName, regNum);
                continue;
            }

            uint16_t curRegVal = pv->voutRegCurVal;
            if (regVal != curRegVal)
            {
                ok = 0;
                if (prtd)
                    interruptingPrintfGrouped(&prtd, "checkVoltageRegisters: register value error\n");
                interruptingPrintfGrouped(&prtd, "%s: Reg %02X %s, value %02X, should be %02X\n", pChip->chipName,
                    regNum, p->desc, regVal, curRegVal);
                rv = reguRegWrite(chipaddr, regNum, p->i2clng, curRegVal);
                i2cPErrorGrouped(&prtd, rv, "write reg");
            }
        }
    }
    interruptingPrintfGroupedEnd(&prtd);
    return ok;
}
