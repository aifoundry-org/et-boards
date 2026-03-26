/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file image_checksum.h
    \brief Functions related to image checksum calculation which are used during bootup and in fw update.
*/

#ifndef IMAGE_CHECKSUM_H
#define IMAGE_CHECKSUM_H

#include <stdint.h>

int calculateChecksum(const uint32_t *fwData, uint32_t fwDataSize, uint32_t *cksum);
uint8_t u8Checksum(const uint8_t *buf, size_t size);

#endif //IMAGE_CHECKSUM_H