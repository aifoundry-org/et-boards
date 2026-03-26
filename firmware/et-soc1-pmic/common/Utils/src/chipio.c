/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file chipio.c
    \brief Chip IO related functions
*/

#include <stdbool.h>
#include <stdint.h>
#include "board_defs.h"
#include "gpio.h"
#include "globals.h"
#include "utils.h"
#include "i2cm2.h"
#include "chipio.h"
#include "chips.h"
#include "cli.h"
#include "scripts.h"
#include "boardchipinfo.h"

#define READ_REG_8(addr, reg, val, rv)         \
    i2cm_read8(addr, 0x20, &reg20);            \
    rv = i2cm_read8(addr, reg, val);           \
    if (i2cm_read8(addr, reg, val) != I2CM_OK) \
    {                                          \
        i2cPError(rv, "read error reg" #reg);  \
        continue;                              \
    }

#define WRITE_REG_8(addr, reg, val, rv)        \
    rv = i2cm_write8(addr, reg, val);          \
    if (rv != I2CM_OK)                         \
    {                                          \
        i2cPError(rv, "write error reg" #reg); \
        continue;                              \
    }

static char const i2cerr[] = "i2cerr";

void cmdI2cScan(void)
{
    uint8_t addr;
    interruptingPrintBegin();
    interruptingPrintf("Found:");
    int col = 6;
    bool found = 0;
    static int const MAX_COL = 40;
    addr = 0x00;
    do
    {
        OSDELAY_MS(1)
        i2cmErr_t rv = i2cm_probe(addr);
        if (rv == I2CM_OK)
        {
            if ((col + 3) > MAX_COL)
            {
                interruptingPrintNL();
                col = 0;
            }
            interruptingPrintf(" %02X", addr);
            col += 3;
            found = 1;
        }
        else if (rv != I2CM_NO_ACK)
        {
            if ((col + 6) > MAX_COL)
            {
                interruptingPrintNL();
                col = 0;
            }
            interruptingPrintf(" %02X-e%d", addr, rv);
            col += 6;
            found = 1;
        }
        ++addr;
    } while (addr != 0xFF);
    if (!found)
        interruptingPrintf("None");
    interruptingPrintNL();
    interruptingPrintEnd();
}

int hwSetRegVal(uint8_t chipid, uint16_t regnum, uint8_t regProtcl, uint8_t reglen, uint64_t regval, t_cprf *pcprf)
{
    if (!pcprf->okSoFar)
    {
        dummy(); // place to set breakpoint
        return UTILS_GENERAL_ERROR;
    }
    pcprf->errId += 1;

    uint8_t chipaddr = structChipInfo[chipid].addr7;
    uint8_t protcl = xlatRTtoProp[regProtcl];
    if (!(protcl & eP_Write))
    {
        pcprf->okSoFar = 0;
        pcprf->errMsg = "not writable";
        return UTILS_ERROR_REG_NOT_WRIETABLE;
    }

    if (regval >= (1ULL << 8 * reglen))
    {
        pcprf->okSoFar = 0;
        pcprf->errMsg = "value too big";
        return UTILS_ERROR_INAVLID_VALUE;
    }
    pcprf->value = regval;
    pcprf->validvalue = 1;

    if (protcl & eP_SepReg)
        regnum = regnum >> 8; // this is a write, use upper byte

    pcprf->errMsg = i2cerr;
    if (!(protcl & eP_Blk))
    {
        if (reglen == 0)
        {
            pcprf->okSoFar = I2CM_OK == i2cm_write0(chipaddr, (uint8_t)regnum);
            OSHWDELAY_MS(pcprf->postI2cWriteDelay)
            return STATUS_SUCCESS;
        }
        if (reglen == 1)
        {
            pcprf->okSoFar = I2CM_OK == i2cm_write8(chipaddr, (uint8_t)regnum, (uint8_t)regval);
            OSHWDELAY_MS(pcprf->postI2cWriteDelay)
            return STATUS_SUCCESS;
        }
        if (reglen == 2)
        {
            pcprf->okSoFar = I2CM_OK == i2cm_write16(chipaddr, (uint8_t)regnum, (uint16_t)regval);
            OSHWDELAY_MS(pcprf->postI2cWriteDelay)
            return STATUS_SUCCESS;
        }
        pcprf->okSoFar = 0;
        pcprf->errMsg = "reg protcl not implemented";
    }
    else
    { // block write
        pcprf->okSoFar = I2CM_OK == i2cm_write_blk_word(chipaddr, (uint8_t)regnum, regval, reglen);
        OSHWDELAY_MS(pcprf->postI2cWriteDelay)
    }

    return STATUS_SUCCESS;
}

int hwGetRegVal(uint8_t chipid, uint16_t regnum, uint8_t regProtcl, uint8_t reglen, t_cprf *pcprf)
{
    if (!pcprf->okSoFar)
        return UTILS_GENERAL_ERROR;
    pcprf->errId += 1;

    uint8_t chipaddr = structChipInfo[chipid].addr7;
    uint8_t protcl = xlatRTtoProp[regProtcl];
    if (!(protcl & eP_Read))
    {
        pcprf->okSoFar = 0;
        pcprf->errMsg = "not readable";
        return UTILS_ERROR_REG_NOT_READABLE;
    }

    if (protcl & eP_SepReg)
        regnum = regnum & 0xFF; // this is a read, use lower byte

    pcprf->errMsg = i2cerr;

    pcprf->validvalue = 1;
    if (!(protcl & eP_Blk))
    {
        // there are no 0 length reads
        if (reglen == 1)
        {
            uint8_t regval = 0;
            pcprf->okSoFar = I2CM_OK == i2cm_read8(chipaddr, (uint8_t)regnum, &regval);
            pcprf->value = regval;
            return STATUS_SUCCESS;
        }
        if (reglen == 2)
        {
            uint16_t regval = 0;
            pcprf->okSoFar = I2CM_OK == i2cm_read16(chipaddr, (uint8_t)regnum, &regval);
            pcprf->value = regval;
            return STATUS_SUCCESS;
        }
        pcprf->okSoFar = 0;
        pcprf->errMsg = "reg protcl not implemented";
    }
    else
    {
        uint64_t regval = 0;
        pcprf->okSoFar = I2CM_OK == i2cm_read_blk_word(chipaddr, (uint8_t)regnum, &regval, reglen);
    }

    return STATUS_SUCCESS;
}

int hwChkRegVal(uint8_t chipid, uint16_t regnum, uint8_t regProtcl, uint8_t reglen, uint64_t val, t_cprf *pcprf)
{
    uint64_t regval64 = 0;
    if (!pcprf->okSoFar)
        return UTILS_GENERAL_ERROR;
    pcprf->errId += 1;

    uint8_t chipaddr = structChipInfo[chipid].addr7;
    uint8_t protcl = xlatRTtoProp[regProtcl];
    if (!(protcl & eP_Read))
    {
        pcprf->okSoFar = 0;
        pcprf->errMsg = "not readable";
        return UTILS_ERROR_REG_NOT_READABLE;
    }

    if (protcl & eP_SepReg)
        regnum = regnum & 0xFF; // this is a read, use lower byte

    pcprf->errMsg = i2cerr;

    pcprf->validvalue = 1;
    do
    {
        if (!(protcl & eP_Blk))
        {
            // there are no 0 length reads
            if (reglen == 1)
            {
                uint8_t regval = 0;
                pcprf->okSoFar = I2CM_OK == i2cm_read8(chipaddr, (uint8_t)regnum, &regval);
                regval64 = regval;
                break;
            }
            if (reglen == 2)
            {
                uint16_t regval = 0;
                pcprf->okSoFar = I2CM_OK == i2cm_read16(chipaddr, (uint8_t)regnum, &regval);
                regval64 = regval;
                break;
            }
            pcprf->okSoFar = 0;
            pcprf->errMsg = "reg protcl not implemented";
        }
        else
        {
            pcprf->okSoFar = I2CM_OK == i2cm_read_blk_word(chipaddr, (uint8_t)regnum, &regval64, reglen);
        }
    } while (0);
    if (pcprf->okSoFar && regval64 != val)
    {
#if 0
        interruptingPrintfOneLine( "Chk %3d %02X,%02X: %0*llX -- %0*llX", pcprf->errId, chipaddr, regnum,  2*reglen, val,  2*reglen, regval64 );
#endif
        pcprf->okSoFar = 0;
        pcprf->errMsg = "read check mismatch";
        return UTILS_ERROR_REG_VAL_MISMATCH;
    }

    return STATUS_SUCCESS;
}

int hwSetChkRegVal(uint8_t chipid, uint16_t regnum, uint8_t regProtcl, uint8_t reglen, uint64_t regval, t_cprf *pcprf)
{
    int status = STATUS_SUCCESS;
    status = hwSetRegVal(chipid, regnum, regProtcl, reglen, regval, pcprf);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }
    if (pcprf->okSoFar)
        pcprf->errId -= 1;
    OSHWDELAY_MS(pcprf->interCheckDelay)
    return hwChkRegVal(chipid, regnum, regProtcl, reglen, regval, pcprf);
}

int hwPause(uint32_t ms, const t_cprf *pcprf)
{
    if (!pcprf->okSoFar)
        return UTILS_GENERAL_ERROR;
    OSHWDELAY_MS(ms)

    return STATUS_SUCCESS;
}

void cmdSetGpio(const char *p, const char *p2)
{
    uint8_t ab = 0;
    uint8_t info = 0;
    uint8_t ll = 0;
    uint8_t ul = 0;
    int n = 0;
    bool ok;

    interruptingPrintBegin();
    do
    {
        if (!p)
        {
            interruptingPrintf("GPIO name required\n");
            break;
        }
        if (p[0] == 'P')
        {
            ++p;
            ab = (uint8_t)(*p++ - 'A');
            n = atodec(p, &ok);
            if ((ab & 0xFE) != 0 || !ok || n < 0 || n >= 32)
            {
                interruptingPrintf("Bad GPIO name\n");
                break;
            }
            ul = ll = (uint8_t)(n + (ab << 5)); // PIN_PA00 = 0, PIN_PB31 = 63
            if (p2)
            {
                if (strcmp(p2, "OUT") == 0)
                    gpio_set_dir_out(ll);
                else if (strcmp(p2, "IN") == 0)
                    gpio_set_dir_in(ll);
                else if (strcmp(p2, "0") == 0)
                    gpio_set_out(ll, 0);
                else if (strcmp(p2, "1") == 0)
                    gpio_set_out(ll, 1);
                else if (p2[0] == '+')
                    info = 2;
                else
                {
                    interruptingPrintf("Bad GPIO param\n");
                    break;
                }
            }
            else
                info = 1; // 2nd param not given
        }
        else if (strcmp(p, "ALL") == 0)
        {
            info = 2;
            ll = 0;
            ul = 63;
        }
        for (uint8_t ngpio = ll; ngpio <= ul; ++ngpio)
        {
            uint8_t pin = (uint8_t)GPIO_PIN(ngpio);
            const PortGroup *pg = &PORT->Group[GPIO_PORT(ngpio)];
            if (ll != ul)
                interruptingPrintf("\nP%c%02d:\n", 'A' + GPIO_PORT(ngpio), pin);
            uint8_t dir = (uint8_t)gpio_read_dir(ngpio) + 2 * pg->PINCFG[pin].bit.INEN;
            static char const *dirs[] = { "NEITHER", "OUT", "IN", "OUT /W READBACK" };
            interruptingPrintf(dir < 3 ? "Dir %s, value %d\n" : "Dir %s, out %d, in %d\n", dirs[dir],
                dir & 1 ? gpio_get_out(ngpio) : gpio_get_in(ngpio), gpio_get_in(ngpio));
            if (info > 1)
            {
                interruptingPrintf("%-8s %X\n", "OUT:", gpio_get_out(ngpio));
                interruptingPrintf("%-8s %X\n", "IN:", gpio_get_in(ngpio));
                PORT_PINCFG_Type cfg = pg->PINCFG[pin];
                interruptingPrintf(cfg.bit.PMUXEN ? "%-8s '%c'\n" : "%-8s N.A. (%c)\n",
                    "PMUX:", 'A' + ((pg->PMUX[pin / 2].reg >> (4 * (pin & 0x01))) & 0x0F));
                interruptingPrintf("%-8s DRVSTR %d, PULLEN %d, INEN %d, PMUXEN %d\n", "CFG:", cfg.bit.DRVSTR,
                    cfg.bit.PULLEN, cfg.bit.INEN, cfg.bit.PMUXEN);
            }
        }
    } while (0);
    interruptingPrintEnd();
}

void cmdSetChip(const char *p, t_cprf *pcprf)
{
    uint16_t i;
    int n;
    interruptingPrintBegin();
    do
    {
        if (!p)
        {
            interruptingPrintf("Choices are:\n");
            for (i = 0; i < MAX_CHIPS; ++i)
                interruptingPrintf("%s\n", structChipInfo[i].chipName);
            break;
        }
        n = 0;
        i = 0xFFFF;
        for (uint16_t j = 0; j < MAX_CHIPS; ++j)
        {
            for (int k = 0; k <= (int)strlen(structChipInfo[j].chipName) - 3; ++k)
            { //try inner substring down to 3 characters
                if (strncmp(p, structChipInfo[j].chipName + k, strlen(p)) == 0)
                {
                    i = j;
                    n += 1;
                }
            }
        }
        if (n == 0)
        {
            interruptingPrintf("Bad chip name\n");
        }
        else if (n > 1)
        {
            interruptingPrintf("Ambiguous chip name\n");
            i = 0xFFFF;
        }
        else
        {
            if (pcprf->chip != i)
            {
                pcprf->page = pcprf->reg = pcprf->field = 0xFFFF;
                pcprf->validvalue = 0;
            }
            pcprf->chip = i;
        }
        i = pcprf->chip;
        if (i < MAX_CHIPS)
            interruptingPrintf("Chip %s selected\n", structChipInfo[i].chipName);
        else
            interruptingPrintf("No chip selected\n");
    } while (0);
    interruptingPrintEnd();
}

static void csrGetReg(char const *p, t_structChipInfo const *pChip, t_cprf *pcprf);
static void csrWriteReg(char const *p2, t_structRegInfo const *pReg, const t_cprf *pcprf);
static void csrGetField(t_structChipInfo const *pChip, t_structRegInfo const *pReg, t_cprf *pcprf);

void cmdSetReg(const char *p, const char *p2, t_cprf *pcprf)
{
    int i;
    t_structChipInfo const *pChip = pcprf->chip < MAX_CHIPS ? &structChipInfo[pcprf->chip] : NULL;
    t_structRegInfo const *pStructRegInfo;

    interruptingPrintBegin();
    do
    {
        if (!pChip)
        {
            interruptingPrintf("Chip not selected yet\n");
            break;
        }
        pStructRegInfo = pChip->pStructRegInfo;
        if (p)
        {
            csrGetReg(p, pChip, pcprf);
        }
        i = pcprf->reg;
        if (i < 0xFFFF)
        {
            if (p2)
            {
                csrWriteReg(p2, &pStructRegInfo[i], pcprf);
            }
            csrGetField(pChip, &pStructRegInfo[i], pcprf);
        }
        else
            interruptingPrintf("No register selected\n");
    } while (0);
    interruptingPrintEnd();
}

// cmdSetReg helper function
static void csrGetReg(char const *p, t_structChipInfo const *pChip, t_cprf *pcprf)
{
    uint16_t i;
    int n;
    char tbuf[32];
    char const *ps;
    t_structRegInfo const *pStructRegInfo = pChip->pStructRegInfo;

    interruptingPrintf("matching reg names:\n");
    n = 0;
    i = 0xFFFF;
    for (uint16_t j = 0; j < pChip->numRegIdx; ++j)
    {
        ps = pStructRegInfo[j].regName;
        assertMsg(strlen(ps) + 3 <= sizeof(tbuf), NULL);
        strcpy(tbuf, "^");
        strcat(tbuf, ps);
        strcat(tbuf, "$");
        for (int k = 0; k <= (int)strlen(tbuf) - 3; ++k)
        { //try inner substring down to 3 characters
            if (strncmp(p, tbuf + k, strlen(p)) == 0)
            {
                i = j;
                n += 1;
                interruptingPrintf("  %s\n", pStructRegInfo[j].regName);
            }
        }
    }
    if (n == 0)
    {
        interruptingPrintf("No matching name\n");
    }
    else if (n > 1)
    {
        interruptingPrintf("Ambiguous reg name\n");
        i = 0xFFFF;
    }
    if (pcprf->reg != i)
    {
        pcprf->field = 0xFFFF;
    }

    pcprf->reg = i;
}

// cmdSetReg helper function
static void csrWriteReg(char const *p2, t_structRegInfo const *pReg, const t_cprf *pcprf)
{
    bool ok;
    i2cmErr_t rv;
    uint8_t chipaddr;
    uint8_t regProps;
    uint8_t reglen;
    uint16_t v = 0;
    if (p2)
    {
        v = (uint16_t)atohex(p2, &ok);
        if (!ok)
        {
            interruptingPrintf("Bad value\n");
            return;
        }
    }
    regProps = xlatRTtoProp[pReg->epType];
    reglen = (regProps & eP_8Bit) ? 1 : (regProps & eP_16Bit) ? 2 : 0;
    if (v >= (1UL << 8 * reglen))
    {
        interruptingPrintf("value exceeds register size\n");
        return;
    }
    chipaddr = structChipInfo[pcprf->chip].addr7;
    if (!(regProps & eP_Write))
    {
        interruptingPrintf("not writable.\n");
        return;
    }
    if (reglen == 0)
    {
        interruptingPrintf("bad reglen\n");
        return;
    }
    rv = i2cm_write8or16(chipaddr, pReg->regnum, v, reglen);
    i2cPError(rv, "write reg");
}

// cmdSetReg helper function
static void csrGetField(t_structChipInfo const *pChip, t_structRegInfo const *pReg, t_cprf *pcprf)
{
    t_structFieldInfo const *pStructFieldInfo = pChip->pStructFieldInfo;
    t_structFieldInfo const *pField;

    interruptingPrintf("Chip %s, Reg %s (offset %02X) selected\n", pChip->chipName, pReg->regName, pReg->regnum);
    if (pReg->numFieldIdx == 1)
    {
        interruptingPrintf("Only one field: %s - Now selected\n", pStructFieldInfo[pReg->firstFieldIdx].fieldName);
        pcprf->field = pReg->firstFieldIdx;
    }
    else
    {
        pcprf->field = 0xFFFF;
        pcprf->validvalue = 0;
        interruptingPrintf("Fields are:\n");
        for (int i = pReg->firstFieldIdx; i < pReg->firstFieldIdx + pReg->numFieldIdx; ++i)
        {
            pField = &pStructFieldInfo[i];
            interruptingPrintf("  %s[%d:%d]\n", pField->fieldName, pField->hiBit, pField->loBit);
        }
    }
}

void cmdReadRegister(t_cprf *pcprf)
{
    t_structChipInfo const *pChip = pcprf->chip < MAX_CHIPS ? &structChipInfo[pcprf->chip] : NULL;
    t_structRegInfo const *pStructRegInfo;
    t_structRegInfo const *pReg = NULL;
    uint8_t chipaddr;
    uint8_t regProps;
    uint8_t reglen;
    i2cmErr_t rv;
    uint16_t val;

    interruptingPrintBegin();
    do
    {
        if (pChip)
        {
            pStructRegInfo = pChip->pStructRegInfo;
            if (pcprf->reg < 0xFFFF)
            {
                pReg = &pStructRegInfo[pcprf->reg];
            }
        }
        if (!pReg)
        {
            interruptingPrintf("Register not selected yet\n");
            break;
        }
        chipaddr = structChipInfo[pcprf->chip].addr7;
        regProps = xlatRTtoProp[pReg->epType];
        reglen = (regProps & eP_8Bit) ? 1 : (regProps & eP_16Bit) ? 2 : 0;
        if (!(regProps & eP_Read))
        {
            interruptingPrintf(" %s %s cannot be read.  Setting current value to 0.\n", pChip->chipName, pReg->regName);
            pcprf->value = 0;
            break;
        }
        if (reglen == 0)
        {
            interruptingPrintf(" %s %s Not implemented.  Skipping\n", pChip->chipName, pReg->regName);
            break;
        }
        rv = i2cm_read8or16(chipaddr, pReg->regnum, &val, reglen);
        i2cPError(rv, "read reg");
        pcprf->value = val;
        pcprf->validvalue = 1;
        interruptingPrintf("%s %s Local value set to %0*X", pChip->chipName, pReg->regName, 2 * reglen, val);
    } while (0);
    interruptingPrintEnd();
}

void cmdSetRegisterValue(const char *p, t_cprf *pcprf)
{
    t_structChipInfo const *pChip = pcprf->chip < MAX_CHIPS ? &structChipInfo[pcprf->chip] : NULL;
    t_structRegInfo const *pStructRegInfo;
    t_structRegInfo const *pReg = NULL;
    uint8_t regProps;
    uint8_t reglen;
    bool ok;
    uint32_t v;

    interruptingPrintBegin();
    do
    {
        if (pChip)
        {
            pStructRegInfo = pChip->pStructRegInfo;
            if (pcprf->reg < 0xFFFF)
            {
                pReg = &pStructRegInfo[pcprf->reg];
            }
        }
        if (!pReg)
        {
            interruptingPrintf("Register not selected yet\n");
            break;
        }
        if (!p)
        {
            interruptingPrintf("Value required\n");
            break;
        }
        v = atohex(p, &ok);
        if (!ok)
        {
            interruptingPrintf("Bad hex value\n");
            break;
        }
        regProps = xlatRTtoProp[pReg->epType];
        reglen = (regProps & eP_8Bit) ? 1 : (regProps & eP_16Bit) ? 2 : 0;
        if (v >= (1UL << 8 * reglen))
        {
            interruptingPrintf("value exceeds register size\n");
            break;
        }
        pcprf->value = v;
        pcprf->validvalue = 1;
        interruptingPrintf("%s %s Local value set to %0*lX", pChip->chipName, pReg->regName, 2 * reglen, v);
    } while (0);
    interruptingPrintEnd();
}

void cmdSetField(const char *p, t_cprf *pcprf)
{
    int i;
    int n;
    t_structChipInfo const *pChip = pcprf->chip < MAX_CHIPS ? &structChipInfo[pcprf->chip] : NULL;
    t_structRegInfo const *pStructRegInfo;
    t_structRegInfo const *pReg = NULL;
    t_structFieldInfo const *pStructFieldInfo;
    t_structFieldInfo const *pField;
    char tbuf[64];
    char const *ps;

    interruptingPrintBegin();
    do
    {
        if (pChip)
        {
            pStructRegInfo = pChip->pStructRegInfo;
            if (pcprf->reg < 0xFFFF)
            {
                pReg = &pStructRegInfo[pcprf->reg];
            }
        }
        if (!pReg)
        {
            interruptingPrintf("Register not selected yet\n");
            break;
        }
        pStructFieldInfo = pChip->pStructFieldInfo;
        if (!p)
        {
            interruptingPrintf("Fields:\n");
            for (int j = pReg->firstFieldIdx; j < pReg->firstFieldIdx + pReg->numFieldIdx; ++j)
            {
                pField = &pStructFieldInfo[j];
                uint8_t width = pField->hiBit + 1 - pField->loBit;
                uint64_t val = (pcprf->value >> pField->loBit) & ((1 << width) - 1);
                memset(tbuf, '.', sizeof(tbuf));
                sprintf(tbuf, "%s[%d:%d]", pField->fieldName, pField->hiBit, pField->loBit);
                tbuf[strlen(tbuf)] = ' ';
                char *q = tbuf + 32;
                for (int8_t m = (width - 1) / 4; m >= 0; --m)
                {
                    sprintf(q++, "%01lX", (uint32_t)((val >> (4 * m)) & 0xF));
                }
                interruptingPrintf("%s\n", tbuf);
            }
        }
        else
        {
            interruptingPrintf("matching reg names:\n");
            n = 0;
            i = 0xFFFF;
            for (int j = pReg->firstFieldIdx; j < pReg->firstFieldIdx + pReg->numFieldIdx; ++j)
            {
                ps = pStructFieldInfo[j].fieldName;
                assertMsg(strlen(ps) + 3 <= sizeof(tbuf), NULL);
                strcpy(tbuf, "^");
                strcat(tbuf, ps);
                strcat(tbuf, "$");
                for (int k = 0; k <= (int)strlen(tbuf) - 3; ++k)
                { //try inner substring down to 3 characters
                    if (strncmp(p, tbuf + k, strlen(p)) == 0)
                    {
                        i = j;
                        n += 1;
                        interruptingPrintf("  %s\n", pStructFieldInfo[j].fieldName);
                    }
                }
            }
            if (n == 0)
            {
                interruptingPrintf("No matching name\n");
            }
            else if (n > 1)
            {
                interruptingPrintf("Ambiguous field name\n");
                i = 0xFFFF;
            }
            pcprf->field = (uint16_t)i;
        }
        pField = &pStructFieldInfo[pcprf->field];
        if (pcprf->field < 0xFFFF)
        {
            interruptingPrintf("Chip %s, Reg %s, Field %s[%d:%d] selected\n", pChip->chipName, pReg->regName,
                pField->fieldName, pField->hiBit, pField->loBit);
        }
        else
            interruptingPrintf("No field selected\n");
    } while (0);
    interruptingPrintEnd();
}

void cmdModifyField(const char *p, t_cprf *pcprf)
{
    uint32_t v;
    bool ok;

    interruptingPrintBegin();
    do
    {
        if (!p)
        {
            interruptingPrintf("value required\n");
            break;
        }
        v = atohex(p, &ok);
        if (!ok)
        {
            interruptingPrintf("bad hex value\n");
            break;
        }
        cmdModifyFieldCommon(v, pcprf);
    } while (0);
    interruptingPrintEnd();
}

void cmdModifyFieldDec(const char *p, t_cprf *pcprf)
{
    uint32_t v;
    bool ok;

    interruptingPrintBegin();
    do
    {
        if (!p)
        {
            interruptingPrintf("value required\n");
            break;
        }
        v = atodec(p, &ok);
        if (!ok)
        {
            interruptingPrintf("bad decimal value\n");
            break;
        }
        cmdModifyFieldCommon(v, pcprf);
    } while (0);
    interruptingPrintEnd();
}

void cmdModifyFieldCommon(uint32_t v, t_cprf *pcprf)
{
    t_structChipInfo const *pChip = pcprf->chip < MAX_CHIPS ? &structChipInfo[pcprf->chip] : NULL;
    t_structRegInfo const *pStructRegInfo;
    t_structRegInfo const *pReg = NULL;
    t_structFieldInfo const *pStructFieldInfo;
    t_structFieldInfo const *pField = NULL;
    uint32_t nmask;
    uint32_t oldvalue;
    uint32_t shv;
    uint8_t regProps;
    uint8_t reglen;

    interruptingPrintBegin();
    do
    {
        if (pChip)
        {
            pStructRegInfo = pChip->pStructRegInfo;
            if (pcprf->reg < 0xFFFF)
            {
                pReg = &pStructRegInfo[pcprf->reg];
            }
            pStructRegInfo = pChip->pStructRegInfo;
            pStructFieldInfo = pChip->pStructFieldInfo;
            if (pcprf->field < 0xFFFF)
            {
                pField = &pStructFieldInfo[pcprf->field];
            }
        }
        if (!pField)
        {
            interruptingPrintf("Field not selected yet\n");
            break;
        }
        if (!pcprf->validvalue)
        {
            interruptingPrintf("Local value not set\n");
            break;
        }
        regProps = xlatRTtoProp[pReg->epType];
        reglen = (regProps & eP_8Bit) ? 1 : (regProps & eP_16Bit) ? 2 : 0;
        nmask = ~((1 << (pField->hiBit + 1)) - (1 << pField->loBit));
        shv = v << pField->loBit;
        if (shv & nmask)
        {
            interruptingPrintf("value %lX is too large for field size", v);
            break;
        }
        oldvalue = (uint32_t)pcprf->value;
        pcprf->value = (oldvalue & nmask) | shv;
        interruptingPrintf("Local value modified %*lX --> %*llX", 2 * reglen, oldvalue, 2 * reglen, pcprf->value);
    } while (0);
    interruptingPrintEnd();
}

void cmdWriteoutRegister(const char *p, const t_cprf *pcprf)
{
    bool readback = p && p[0] == 'R';
    t_structChipInfo const *pChip = pcprf->chip < MAX_CHIPS ? &structChipInfo[pcprf->chip] : NULL;
    t_structRegInfo const *pStructRegInfo;
    t_structRegInfo const *pReg = NULL;
    uint8_t regProps;
    uint8_t reglen;
    uint8_t chipaddr;
    i2cmErr_t rv;

    interruptingPrintBegin();
    do
    {
        if (pChip)
        {
            pStructRegInfo = pChip->pStructRegInfo;
            if (pcprf->reg < 0xFFFF)
            {
                pReg = &pStructRegInfo[pcprf->reg];
            }
            pStructRegInfo = pChip->pStructRegInfo;
        }
        if (!pcprf->validvalue)
        {
            interruptingPrintf("Local value not set\n");
            break;
        }
        chipaddr = structChipInfo[pcprf->chip].addr7;
        regProps = xlatRTtoProp[pReg->epType];
        reglen = (regProps & eP_8Bit) ? 1 : (regProps & eP_16Bit) ? 2 : 0;
        if (!(regProps & eP_Write))
        {
            interruptingPrintf(" %s %s is not writable.\n", pChip->chipName, pReg->regName);
            break;
        }
        if (reglen == 0)
        {
            i2cm_write0(chipaddr, pReg->regnum);
            break;
        }
        rv = i2cm_write8or16(chipaddr, pReg->regnum, (uint16_t)pcprf->value, reglen);
        i2cPError(rv, "write reg");
        if (readback)
        {
            uint16_t val = 0;
            rv = i2cm_read8or16(chipaddr, pReg->regnum, &val, reglen);
            i2cPError(rv, "read reg");
            interruptingPrintf("%s %s Wrote %0*llX, read back %0*X", pChip->chipName, pReg->regName, 2 * reglen,
                pcprf->value, 2 * reglen, val);
        }
    } while (0);
    interruptingPrintEnd();
}

void runScript(const char *p, t_cprf *pcprf, powerState_t *pps)
{
    scriptStruct_t *psl;
    int i;
    bool ok;

    interruptingPrintBegin();
    do
    {
        if (!p)
        {
            for (i = 1, psl = scriptList; psl->pScript; ++i, ++psl)
            {
                interruptingPrintf("%d. %s\n", i, psl->name);
            }
            break;
        }
        i = atodec(p, &ok) - 1;
        if (!ok || (uint32_t)i >= (uint32_t)MAX_SCRIPTLIST)
        {
            interruptingPrintf("bad scriptnum\n");
            break;
        }
        psl = &scriptList[i];
        execScript(psl, pcprf, pps);
    } while (0);
    interruptingPrintEnd();
}

void execScript(scriptStruct_t *psl, t_cprf *pcprf, powerState_t *pps)
{
#if 0
    uint8_t dummy;
    char tbuf[32];
    for( char const * const * pp = psl->pScript; *pp; ++pp ) {
        strcpy( tbuf, *pp );
        detailMenu( tbuf, &dummy, pcprf, pps );
    }
#endif
}

#define FIXTIMINGBUG
void doI2cWrite(const char *p, char **plinep)
{
    uint8_t tx[37];
    uint32_t ntx = 0;
    i2cmErr_t rv;
    bool ok = 0;

    interruptingPrintBegin();
    do
    {
        while (1)
        {
            if (!p)
            {
                break;
            }
            uint32_t v = atohex(p, &ok);
            if (!ok || v > 0xFF)
            {
                interruptingPrintf("bad byte: %s\n", p);
                break;
            }
            if (ntx == 0 && v > 0xFF)
            {
                interruptingPrintf("bad address %s", p);
                break;
            }
            if (ntx >= sizeof(tx))
            {
                interruptingPrintf("too many bytes\n");
                break;
            }
            tx[ntx++] = (uint8_t)v;
            p = nextToken(plinep);
        }
        if (ntx < 2)
        {
            interruptingPrintf("too few bytes\n");
            break;
        }
        rv = i2cm_write(tx[0], tx + 1, ntx - 1);
        i2cPError(rv, "write reg");
    } while (0);
    interruptingPrintEnd();
}

void doI2cRead(char *p1, const char *p2, const char *p3, bool repeat)
{
    uint8_t rx[34];
    i2cmErr_t rv = 0;
    bool ok = 0;
    uint32_t i;
    uint32_t v;
    uint8_t addr = 0;
    uint8_t reg;
    bool pagelessMode;

    interruptingPrintBegin();
    do
    {
        v = atohex(p1, &ok);
        if (!ok || v > 0x1FF)
        {
            interruptingPrintf("bad address: %s\n", p1);
            break;
        }
        addr = (uint8_t)v;
        pagelessMode = (v >> 8) != 0;

        if (!p2)
        {
            rv = i2cm_probe(addr);
            interruptingPrintf("%s", i2cm_get_error_message(rv));
            break;
        }
        v = atohex(p2, &ok);
        if (!ok || v > 0xFF)
        {
            interruptingPrintf("bad reg\n");
            break;
        }
        reg = (uint8_t)v;
        v = atohex(p3, &ok);
        if (!ok || v > sizeof(rx))
        {
            interruptingPrintf("bad num bytes to read\n");
            break;
        }
        for (i = 0; i < (repeat ? 1000 : 1); ++i)
        {
            if (i > 0)
                OSDELAY_MS(10)
            rv = !pagelessMode ? i2cm_read(addr, &reg, 1, rx, v) : i2cm_readPageless(addr, &reg, 1, rx, v);
        }
        i2cPError(rv, "read reg");
        if (rv == I2CM_OK)
        {
            for (i = 0; i < v; ++i)
            {
                if (!(i % 16))
                {
                    interruptingPrintf("\n%02X: ", i);
                }
                interruptingPrintf(" %02X", rx[i]);
            }

            interruptingPrintf("\n");
        }
    } while (0);
    interruptingPrintEnd();
}

void doI2cWriteRead(const char *p1, const char *p2, char **plinep)
{
    uint8_t tx[21];
    uint8_t rx[34];
    i2cmErr_t rv = 0;
    bool ok;
    uint32_t i;
    uint32_t v;
    uint8_t addr;
    uint8_t rsz;
    uint32_t ntx = 0;
    bool pagelessMode;

    interruptingPrintBegin();
    do
    {
        v = atohex(p1, &ok);
        if (!ok || v > 0x1FF)
        {
            interruptingPrintf("bad address: %s\n", p1);
            break;
        }
        addr = (uint8_t)v;
        pagelessMode = (v >> 8) != 0;

        if (!p2)
        {
            rv = i2cm_probe(addr);
            interruptingPrintf("%s", i2cm_get_error_message(rv));
            break;
        }
        v = atohex(p2, &ok);
        if (!ok || v > sizeof(rx))
        {
            interruptingPrintf("too many data to read\n");
            break;
        }
        rsz = (uint8_t)v;

        while (1)
        {
            p1 = nextToken(plinep);
            if (!p1)
            {
                break;
            }
            v = atohex(p1, &ok);
            if (!ok || v > 0xFF)
            {
                interruptingPrintf("bad data: %s\n", p1);
                break;
            }
            if (ntx >= sizeof(tx))
            { // for EEPROM, 16 data bytes + 2 address offset
                ok = false;
                interruptingPrintf("too many data to write\n");
                break;
            }
            tx[ntx++] = (uint8_t)v;
        }
        if (!ok)
            break;
        if (ntx < 1)
        {
            interruptingPrintf("too few data\n");
            break;
        }

        rv = pagelessMode ? i2cm_readPageless(addr, tx, ntx, rx, rsz) : i2cm_read(addr, tx, ntx, rx, rsz);
        i2cPError(rv, "i2c master write-read");
        if (rv == I2CM_OK)
        {
            for (i = 0; i < rsz; ++i)
            {
                if (!(i % 16))
                {
                    interruptingPrintf("\n%02X: ", i);
                }
                interruptingPrintf(" %02X", rx[i]);
            }

            interruptingPrintf("\n");
        }
    } while (0);
    interruptingPrintEnd();
}

void cmdFS(char *p1, const char *p2)
{
    interruptingPrintBegin();
    do
    {
        uint8_t addr;
        uint8_t addr1 = 0x08;
        uint8_t addr2 = 0x0B;
        bool ok = 0;
        bool doit = 0;
        bool undoit = 0;
        i2cmErr_t rv;

        if (p1)
        {
            doit = (p1 && (strcmp(p1, "PROGRAM") == 0));
            undoit = (p1 && (strcmp(p1, "REVERTTEST") == 0));
        }

        if (!(doit || undoit))
        {
            addr = atohex(p1, &ok);
            if (!ok || addr < 0x08 || 0x0B < addr)
            {
                interruptingPrintf("bad addr: %s\n", p1);
                break;
            }
            addr1 = addr2 = addr;
            doit = (p2 && (strcmp(p2, "PROGRAM") == 0));
            undoit = (p2 && (strcmp(p2, "REVERTTEST") == 0));
        }

        for (addr = addr1; addr <= addr2; ++addr)
        {
            interruptingPrintf("Doing addr %02X\n", addr);

            uint8_t reg1A = 0;
            uint8_t reg20 = 0;
            READ_REG_8(addr, 0x1A, &reg1A, rv)
            READ_REG_8(addr, 0x1A, &reg20, rv)

            char const *fmt = NULL;
            ok = 1;
            switch (reg20)
            {
                case 0x09:
                    fmt = "3 prog remaining\n";
                    break;
                case 0x12:
                    fmt = "2 remaining\n";
                    break;
                case 0x1B:
                    fmt = "1 remaining\n";
                    break;
                case 0x24:
                    fmt = "0 remaining\n";
                    ok = 0;
                    break;
                default:
                    ok = 0;
                    fmt = "unknown code %02X\n";
            }
            interruptingPrintf(fmt, reg20);

            static char const *vname[] = { "1.8", "3.3" };
            interruptingPrintf("set for %sV\n", vname[(reg1A & 0x02) != 0]);

            if (!ok)
            {
                interruptingPrintf("Can't program\n");
                continue;
            }

            if (!(doit || undoit))
                continue;

            if ((reg1A & 0x02) != 0)
            {
                if (undoit)
                {
                    interruptingPrintf("Already 3.3V\n");
                    continue;
                }
                reg1A = 0x00;
            }
            else
            {
                if (doit)
                {
                    interruptingPrintf("Already 1.8V\n");
                    continue;
                }
                reg1A = 0x02;
            }

            WRITE_REG_8(addr, 0x1A, reg1A, rv)

            uint8_t val;
            rv = i2cm_read8(addr, 0x1D, &val);
            if (rv != I2CM_OK)
            {
                i2cPError(rv, "read error reg1D");
                continue;
            }

            interruptingPrintf("Programming - wait\n");
            OSDELAY_MS(3000)
            val &= 0xFC;

            WRITE_REG_8(addr, 0x1D, reg1A, rv)
            OSDELAY_MS(3000)
            val |= 0x02;
            WRITE_REG_8(addr, 0x1D, reg1A, rv)
            interruptingPrintf("%02X done.\n", addr);
        } // end for

        if (doit || undoit)
        {
            interruptingPrintf(
                "Requested programming (if any) done.  Power cycle and check before removing I2C pullups.\n");
        }
        break;
    } while (0);
    if (!p1)
    {
        interruptingPrintNL();
        interruptingPrintf("fs                     -> check all 4 FS regulators\n");
        interruptingPrintf("fs [8|9|A|B]           -> check one FS regulator\n");
        interruptingPrintf("fs program             -> program all 4 FS regulators\n");
        interruptingPrintf("fs [8|9|A|B] program   -> program one FS regulator\n");
    }
    interruptingPrintNL();
    interruptingPrintf("Programming will fail without 7.5V supply and I2C pullup.\n");

    interruptingPrintEnd();
}
