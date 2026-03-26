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

#ifndef __FEATURES_H__
#define __FEATURES_H__

#include <stdbool.h>

/***********************************************************************
 * GLOBAL Macros:  Defininition of global macros
***********************************************************************/
// Serial port outputs end of line as CRLF if defined, LF otherwise
#define FEATURE_UART_CRLF

#define USE_CONSOLE

#define USE_I2CM1
#define USE_I2CM2

#define USE_I2CS

#define USE_NVM

#define USE_ADC
#define USE_TC0
#define USE_TC2
#define USE_TC3
#define USE_TC4
#define USE_EIC_INTERRUPT

//#define USE_DAC
//#define USE_AC

/***********************************************************************
 * GLOBAL data types:      Defininition of global data types
***********************************************************************/
typedef enum {
    POWER_GOOD_ON_IOXPANDER_PRESENT,
    POWER_GOOD_3P3V_SIGNAL_PRESENT,
    POWER_GOOD_12V_SIGNAL_PRESENT,

    FEATURE_PER_HW_ENCODING_MAX_COUNT,

} featureId_t;

/***********************************************************************
 * GLOBAL Functions:   Declaration of global functions
***********************************************************************/
bool isFeaturePerHwEncodingEnabled(featureId_t featureId);

#endif // __FEATURES_H__
