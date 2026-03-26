/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file TPS53681.c
    \brief C file for TPS53681 driver
*/
/***********************************************************************/

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "TPS53681.h"
#include "pmbus.h"

void TPS53681_init(t_cprf *pcprf)
{
    SETREGVAL_PMBUS(TPS53681, CLEAR_FAULT, 0) // 0 is dummy byte, this write has no data
    SETREGVAL_PMBUS(TPS53681, ON_OFF_CONFIG, 0x17)
    SETREGVAL_PMBUS(TPS53681, PHASE, 0x80)
    SETREGVAL_PMBUS(TPS53681, OT_FAULT_RESPONSE, 0x80)
    SETREGVAL_PMBUS(TPS53681, PHASE, 0x80)
    SETREGVAL_PMBUS(TPS53681, OPERATION, 0x80) // enable

    SETREGVAL_MFR(TPS53681, MFR_SPECIFIC_11, 0x1F, _MNN) // Boot VID : 400 mV
    SETREGVAL_MFR(TPS53681, MFR_SPECIFIC_11, 0x29, _NOC) // BOOT VID : 450 mV
}
