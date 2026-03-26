/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file fruinfo.h
    \brief FRU information storage related.
*/

#ifndef FRUINFO_H
#define FRUINFO_H

#include "utils.h"
#include "scripts.h"
#include "iostructs.h"
#include "globals.h"

void post_fruinfo(void);
void cmdFruRead(void);
void cmdFruWrite(const char *p, char **plinep);
void cmdFruPrint(void);

typedef enum {
    FRU_OPS_CLI_IDLE,
    FRU_OPS_CLI_OVERRIDE,
    FRU_OPS_SET_OFFSET,
    FRU_OPS_GET_OFFSET,
    FRU_OPS_READ,
    FRU_OPS_WRITE,
    FRU_OPS_WRITE_COMMIT,

    FRU_OPS_INVALID,
} fru_ops_t;

int fruOpsCmdSet(ETSOCVirtualRegisterIdx_t regIdx, uint32_t new_ops);
int fruOpsDataWrite(uint32_t data);
int fruOpsDataRead(void);

#endif /* FRUINFO_H */
