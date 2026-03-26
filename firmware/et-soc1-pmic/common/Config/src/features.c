/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file features.h
    \brief Enables or disables various features for specific board types
*/
/***********************************************************************/

#include <stdbool.h>
#include "features.h"
#include "hw_encoding.h"

/***********************************************************************
 * Data types:      Defininition of local data types
***********************************************************************/
typedef struct {
    featureId_t featureId;
    bool enabledPerHwEncoding[HW_ENCODING_VERSION_MAX_COUNT];
} featureEnabledPerHwEncoding_t;

// clang-format off
static const featureEnabledPerHwEncoding_t featureEnabledPerHwEncoding[FEATURE_PER_HW_ENCODING_MAX_COUNT] = {
                                        // feature id                          { HW_ENCODING_RESERVE_0, HW_ENCODING_PCIE_1088, HW_ENCODING_RESERVE_2, HW_ENCODING_PCIE_V3_2, HW_ENCODING_RESERVE_4, HW_ENCODING_PCIE_V3_3, HW_ENCODING_BUB_REV_2, HW_ENCODING_VERSION_7 ...
                                        //                                     { BUB v2,                Penguin RevB,          unused,                PCIE v3.2,             unused,                PCIE v3.3,             BUB v2,                unused ...
    [POWER_GOOD_ON_IOXPANDER_PRESENT]   = { POWER_GOOD_ON_IOXPANDER_PRESENT,   { true,                  true,                  false,                 false,                 false,                 false,                 true  }},//            false ... }},
    [POWER_GOOD_3P3V_SIGNAL_PRESENT]    = { POWER_GOOD_3P3V_SIGNAL_PRESENT,    { false,                 true,                  false,                 true,                  false,                 true,                  false }},//            false ... }},
    [POWER_GOOD_12V_SIGNAL_PRESENT]     = { POWER_GOOD_12V_SIGNAL_PRESENT,     { false,                 true,                  false,                 true,                  false,                 true,                  false }},//            false ... }},
};
// clang-format on

/***********************************************************************
 * GLOBAL Functions:   Defininition of global functions
***********************************************************************/
bool isFeaturePerHwEncodingEnabled(featureId_t featureId)
{
    bool featureEnabled = false;
    if (featureId >= 0 && featureId < FEATURE_PER_HW_ENCODING_MAX_COUNT)
    {
        featureEnabled =
            featureEnabledPerHwEncoding[featureId].enabledPerHwEncoding[Hw_Encoding_Get_Hw_Version_Encoding()];
    }
    return featureEnabled;
}
