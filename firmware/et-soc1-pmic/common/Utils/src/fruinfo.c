/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file fruinfo.c
    \brief FRU information storage related functions
*/

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "board_defs.h"
#include "FreeRTOS.h"
#include "projdefs.h"
#include "semphr.h"

#include "config.h"
#include "fruinfo.h"
#include "nvm.h"
#include "image_checksum.h"
#include "etsoc_cmd_handler_task.h"
#include "cli.h"
#include "CLITask.h"

#if FRUSIZE > FLASH_ROW_SIZE
#error Current implementation supports only up to 256 byte (1 row) FRU info
#endif

#define UINT32_SIZE 4
#if FRUSIZE & (UINT32_SIZE - 1)
#error FRU information storage size has to be multiple of uint32_t
#endif
#define FRU_WSIZE (FRUSIZE / sizeof(uint32_t))

#if FRUOFFSET & (FLASH_ROW_SIZE - 1)
#error FRU information storage has to be row-aligned
#endif

static union {
    uint8_t byte[FRUSIZE];
    uint32_t word[FRU_WSIZE];
} fruinfo;

/* FRU information storage write / read functions */
static int programFRU(void)
{
    int count;
    const uint32_t *psrc;
    uint32_t *pdest;
    uint32_t *ppage;
    uint32_t fru_wsize;
    int status = STATUS_SUCCESS;

    fru_wsize = FRU_WSIZE;
    psrc = fruinfo.word;
    pdest = (uint32_t *)FRUOFFSET;
    if (nvmEraseRow(pdest, true) != STATUS_SUCCESS)
    {
        interruptingPrintf("row erase oops @ %08X\n", (uint32_t)pdest);
        return GENERAL_ERROR;
    }

    while (fru_wsize)
    {
        /* prepare the page buffer */
        if (fru_wsize >= WORD_COUNT_PER_PAGE)
        {
            count = WORD_COUNT_PER_PAGE;
        }
        else
        {
            count = fru_wsize;
        }
        fru_wsize -= count;

        ppage = pdest;
        while (count--)
        {
            *pdest++ = *psrc++;
        }

        /* program the page */
        if (nvmExecuteWriteUserPage() != STATUS_SUCCESS)
        {
            interruptingPrintf("write oops @ %08X\n", (uint32_t)ppage);
        }
    }

    fru_wsize = FRU_WSIZE;
    psrc = fruinfo.word;
    pdest = (uint32_t *)FRUOFFSET;
    do
    {
        if (*pdest++ != *psrc++)
        {
            interruptingPrintf(
                "%02X: write %08X read %08X\n", (WORD_COUNT_PER_ROW - fru_wsize) * UINT32_SIZE, psrc[-1], pdest[-1]);
            status = GENERAL_ERROR;
        }
    } while (--fru_wsize);

    return status;
}

static void load_fruinfo(void)
{
    int fru_wsize = FRU_WSIZE;
    const uint32_t *psrc = (uint32_t *)(FRUOFFSET);
    uint32_t *pdest = fruinfo.word;

    while (fru_wsize--)
    {
        *pdest++ = *psrc++;
    }
}

#define CLI_WRITE_FLAG 0xA5

/* FRU information intepretation functions */
static bool fru_area_checksum(uint32_t offset)
{
    uint32_t length;

    if (offset > FRUSIZE - 8) /* area must be longer than 8 bytes */
    {
        interruptingPrintf(" invalid offset\n");
        return false;
    }

    if (1 != fruinfo.byte[offset])
    {
        interruptingPrintf(" invalid version\n");
        return false;
    }

    length = fruinfo.byte[offset + 1] * 8;
    if (offset + length > FRUSIZE)
    {
        interruptingPrintf(" invalid length\n");
        return false;
    }

    if (u8Checksum(&fruinfo.byte[offset], length))
    {
        interruptingPrintf(" corrupted area\n");
        return false;
    }

    return true;
}

static bool interruptingPrintFru(const uint8_t **ppFru, const char *desc, bool skip)
{
    char buf[64] = { 0 };
    uint8_t *ptr = (uint8_t *)buf;
    const uint8_t *pFru = *ppFru;
    uint8_t len = *pFru & 0x3F;
    if ((pFru + len) >= (fruinfo.byte + FRUSIZE - 3))
    {
        interruptingPrintf("  %s:\tcorrupted\n", desc);
        return false;
    }

    if (1 == len)
    {
        interruptingPrintf("  %s:\tend of fields\n", desc);
        return false;
    }

    if (skip)
    {
        pFru += ++len;
    }
    else
    {
        if (0xC0 == (*pFru++ & 0xC0))
        {
            while (len--)
            {
                *ptr++ = *pFru++;
            }
            interruptingPrintf("  %s:\t%s\n", desc, buf);
        }
        else
        {
            pFru += len;
            interruptingPrintf("  %s:\tunsupported type\n", desc);
        }
    }
    *ppFru = pFru;
    return true;
}

static bool is_valid_fruinfo(void)
{
    bool ret = true;
    uint32_t offset;
    const uint8_t *pFru = NULL;

    /* common header */
    if (u8Checksum(fruinfo.byte, 8))
    {
        interruptingPrintf("Corrupted Common Header\n");
        ret = false;
    }
    if (fruinfo.byte[0] != 1)
    {
        interruptingPrintf("Invalid FRU version\n");
        ret = false;
    }
    if (fruinfo.byte[1])
    {
        interruptingPrintf("Unknown Internal Use area\n");
    }
    if (fruinfo.byte[2])
    {
        interruptingPrintf("Unknown Chassis Info area\n");
    }
    if (fruinfo.byte[5])
    {
        interruptingPrintf("Unknown MultiRecord area\n");
    }
    if (fruinfo.byte[6])
    {
        interruptingPrintf("Invalid padding\n");
        ret = false;
    }

    /* board area */
    interruptingPrintf("Board:\n");
    offset = fruinfo.byte[3] * 8;
    if (fru_area_checksum(offset))
    {
        pFru = &fruinfo.byte[offset + 6];
        if (!interruptingPrintFru(&pFru, "Mfgr", false) || !interruptingPrintFru(&pFru, "Name", false) ||
            !interruptingPrintFru(&pFru, "S/N", false) || !interruptingPrintFru(&pFru, "P/N", false))
        {
            ret = false;
        }
    }
    else
    {
        ret = false;
    }

    /* product info area */
    interruptingPrintf("Product:\n");
    offset = fruinfo.byte[4] * 8;
    if (fru_area_checksum(offset))
    {
        pFru = &fruinfo.byte[offset + 3];
        if (!interruptingPrintFru(&pFru, "Mfgr", false) || !interruptingPrintFru(&pFru, "Name", false) ||
            !interruptingPrintFru(&pFru, "P/N", false) || !interruptingPrintFru(&pFru, "Ver", true) ||
            !interruptingPrintFru(&pFru, "S/N", false) || !interruptingPrintFru(&pFru, "ATag", false))
        {
            ret = false;
        }
    }
    else
    {
        ret = false;
    }

    return ret;
}

/* CLI interface command functions */
void cmdFruRead(void)
{
    int len = FRUSIZE;
    int i = 0;
    const uint8_t *p = fruinfo.byte;

    if (0x01 != *p)
        load_fruinfo();

    interruptingPrintBegin();
    interruptingPrintf("FRU Data:");
    while (len--)
    {
        if (!(i % 16))
        {
            interruptingPrintf("\n%02X: ", i);
        }
        interruptingPrintf(" %02X", *p++);
        i++;
    }

    interruptingPrintf("\n");
    interruptingPrintEnd();
}

void cmdFruWrite(const char *p, char **plinep)
{
    uint32_t data;
    uint32_t offset;
    uint8_t *ptr;
    const uint8_t *pend = fruinfo.byte + FRUSIZE;
    uint16_t eof = 0;
    bool ok = 0;

    if (!p)
        return;

    interruptingPrintBegin();
    do
    {
        offset = atohex(p, &ok);
        if (!ok || offset >= FRUSIZE)
        {
            interruptingPrintf("bad offset: %s\n", p);
            ok = 0;
            break;
        }
        else if ((FRUSIZE - 1) == offset)
        {
            ptr = (uint8_t *)&eof;
            pend = ptr + sizeof(eof);
        }
        else
        {
            ptr = &fruinfo.byte[offset];
        }

        while ((p = nextToken(plinep)))
        {
            data = atohex(p, &ok);
            if (!ok || data > 0xFF)
            {
                interruptingPrintf("bad data %s\n", p);
                ok = 0;
            }
            if (ptr >= pend)
            {
                interruptingPrintf("too many bytes\n");
                ok = 0;
            }
            if (!ok)
                break;
            *ptr++ = (uint8_t)data;
        }

        fruinfo.byte[0] = CLI_WRITE_FLAG; /* invalidate fruinfo */
        setRegisterValue(FRU_OPS_CMD, FRU_OPS_CLI_OVERRIDE);
    } while (0);

    if (!ok || ((FRUSIZE - 1) != offset))
    {
        goto EXIT_CMD_FRU_WRITE;
    }
    else if (eof <= 0xFF)
    {
        fruinfo.byte[FRUSIZE - 1] = (uint8_t)eof;
        goto EXIT_CMD_FRU_WRITE;
    }
    else if (0xFFFF == eof)
    {
        if (CLI_WRITE_FLAG == fruinfo.byte[0])
        {
            fruinfo.byte[0] = 0x01;
            if (!is_valid_fruinfo())
            {
                interruptingPrintf("start over please!\n");
                load_fruinfo();
            }
            else
            {
                programFRU();
            }
        }
    }
    else
    {
        interruptingPrintf("Invalid command. Start over please!\n");
        load_fruinfo();
    }
    setRegisterValue(FRU_OPS_CMD, FRU_OPS_CLI_IDLE);

EXIT_CMD_FRU_WRITE:
    interruptingPrintEnd();
}

void cmdFruPrint(void)
{
    if (0x01 != fruinfo.byte[0])
        load_fruinfo();

    interruptingPrintBegin();
    if (!is_valid_fruinfo())
    {
        interruptingPrintf("FRU data corrupted!\n");
    }
    interruptingPrintEnd();
}

/* SoC interface command functions */
static uint8_t fruOpsOffset;
int fruOpsCmdSet(ETSOCVirtualRegisterIdx_t regIdx, uint32_t new_ops)
{
    int status;
    uint32_t current;

    if (new_ops >= FRU_OPS_INVALID)
    {
        return GENERAL_ERROR;
    }

    status = getRegisterValue(regIdx, &current);
    if (STATUS_SUCCESS != status)
    {
        return status;
    }
    if (FRU_OPS_CLI_OVERRIDE == current)
    {
        /* CLI fru-write takes the precedence */
        return GENERAL_ERROR;
    }

    switch (new_ops)
    {
        case FRU_OPS_CLI_OVERRIDE:
        case FRU_OPS_CLI_IDLE:
            /* ops from CLI only */
            status = GENERAL_ERROR;
            break;
        case FRU_OPS_GET_OFFSET:
            setRegisterValue(FRU_OPS_DATA, fruOpsOffset);
            break;
        case FRU_OPS_READ:
            if (FRU_OPS_READ != current)
            {
                load_fruinfo();
            }
            setRegisterValue(FRU_OPS_DATA, fruinfo.byte[fruOpsOffset++]);
            break;
        case FRU_OPS_WRITE:
            if ((FRU_OPS_SET_OFFSET != current) && (FRU_OPS_WRITE != current))
            {
                status = GENERAL_ERROR;
            }
            break;
        case FRU_OPS_WRITE_COMMIT:
            if (FRU_OPS_WRITE == current)
            {
                if (!is_valid_fruinfo())
                {
                    interruptingPrintf("Changes discarded!\n");
                    load_fruinfo();
                    status = GENERAL_ERROR;
                }
                else
                {
                    interruptingPrintf("FRU data updated!\n");
                    status = programFRU();
                }
            }
            else if (FRU_OPS_WRITE_COMMIT != current)
            {
                status = GENERAL_ERROR;
            }
            fruOpsOffset = 0;
            break;
        default:
            break;
    }

    if (STATUS_SUCCESS == status)
    {
        setRegisterValue(regIdx, new_ops);
    }

    return status;
}

int fruOpsDataWrite(uint32_t data)
{
    int status;
    uint32_t ops;

    status = getRegisterValue(FRU_OPS_CMD, &ops);
    if (STATUS_SUCCESS == status)
    {
        switch (ops)
        {
            case FRU_OPS_SET_OFFSET:
                if (data >= FRUSIZE)
                {
                    status = GENERAL_ERROR;
                }
                else
                {
                    fruOpsOffset = (uint8_t)data;
                }
                break;
            case FRU_OPS_WRITE:
                if (fruOpsOffset >= FRUSIZE)
                {
                    status = GENERAL_ERROR;
                }
                else
                {
                    fruinfo.byte[fruOpsOffset++] = (uint8_t)data;
                }
                break;
            default:
                status = GENERAL_ERROR;
                break;
        }
    }

    return status;
}

int fruOpsDataRead(void)
{
    int status;
    uint32_t ops;

    status = getRegisterValue(FRU_OPS_CMD, &ops);
    if (STATUS_SUCCESS == status)
    {
        switch (ops)
        {
            case FRU_OPS_GET_OFFSET:
                break;
            case FRU_OPS_READ:
                if (fruOpsOffset >= FRUSIZE)
                {
                    status = GENERAL_ERROR;
                }
                else
                {
                    status = setRegisterValue(FRU_OPS_DATA, fruinfo.byte[fruOpsOffset++]);
                }
                break;
            default:
                status = GENERAL_ERROR;
                break;
        }
    }

    return status;
}
