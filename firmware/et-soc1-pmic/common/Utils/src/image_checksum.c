/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file image_checksum.c
    \brief Functions related to image checksum calculation which are used during bootup and in fw update.
*/

#include <stdint.h>
#include <stddef.h>
#include "image_checksum.h"
#include "error_codes.h"

int calculateChecksum(const uint32_t *fwData, uint32_t fwDataSize, uint32_t *cksum)
{
    uint32_t ckSum = 0;

    if (cksum == NULL)
    {
        return INVALID_ARGUMENT;
    }

    for (; (int32_t)fwDataSize > 0; fwDataSize -= sizeof(uint32_t))
    {
        ckSum += *fwData++;
    }

    *cksum = ckSum;
    return STATUS_SUCCESS;
}

uint8_t u8Checksum(const uint8_t *buf, size_t size)
{
    uint8_t sum = 0;
    while (size--)
    {
        sum += *buf++;
    }
    return sum;
}
