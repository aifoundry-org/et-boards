/***********************************************************************
 *
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *
 ************************************************************************/

/***********************************************************************/
/*! \file chipio.c
    \brief Chip macro related functions
*/

#include <stdbool.h>
#include <stdint.h>

#include "hw_encoding.h"
#include "MAX6660.h"
#include "TPS53681.h"
#include "TPSM8D6C24.h"

void chipInit(t_cprf *pcprf)
{
    pcprf->okSoFar = 1;
    pcprf->errId = 0;

    pcprf->postI2cWriteDelay = 0; // mSec
    pcprf->interCheckDelay = 0;

    TPS53681_init(pcprf);
    switch (Hw_Encoding_Get_Hw_Version_Encoding())
    {
        case HW_ENCODING_PCIE_1088:
            TPSM8D6C24_init(pcprf);
            break;
        default:
            LTM4680_init(pcprf);
            break;
    }
}
