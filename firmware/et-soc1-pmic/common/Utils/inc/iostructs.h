/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file iostructs.h
    \brief IO related data types.
*/

#ifndef IOSTRUCTS_H
#define IOSTRUCTS_H

#include <stdbool.h>

typedef struct {
    bool hwUp;
    bool reqPwrUp;
    bool deferPerstUp;
    bool socPerstToggled;
    bool skipReguCheck;
    bool waiting12V;
} powerState_t;

typedef struct {
    uint32_t vmon12vMV;
    uint32_t imon12vMA;
    uint32_t oldT;
    uint32_t instmW;
    int64_t avemW;
    bool initV;
} analogData_t;

typedef struct {
    powerState_t powerState;
    analogData_t analogData;
} commonData_t;

typedef struct {
    uint64_t value;
    uint16_t chip;
    uint16_t page;
    uint16_t reg;
    uint16_t field;
    uint16_t postI2cWriteDelay;
    uint16_t interCheckDelay;
    uint16_t errId;
    uint8_t protcl;
    bool validvalue;
    bool okSoFar;
    char const *errMsg;
} t_cprf;

#endif /* IOSTRUCTS_H */
