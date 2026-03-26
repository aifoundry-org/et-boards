/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file TPSM8D6C24.c
    \brief C file for TPSM8D6C24 driver
*/
/***********************************************************************/

#include "boardchipinfo.h"
#include "pmbus.h"
#include "TPSM8D6C24.h"

void TPSM8D6C24_init(t_cprf *pcprf)
{
    SETREGVAL_PMBUS(TPSM8D6C24, OPERATION, 0x00) // disable  0x01

    // set default values in case they aren't correct
    SETREGVAL_PMBUS(TPSM8D6C24, VIN_ON, 0xD130)               // 4.75V 	0x35
    SETREGVAL_PMBUS(TPSM8D6C24, VIN_OFF, 0xD120)              // 4.5V 	0x36
    SETREGVAL_PMBUS(TPSM8D6C24, VOUT_OV_FAULT_RESPONSE, 0xB8) // shutdown 0x41
    SETREGVAL_PMBUS(TPSM8D6C24, VOUT_UV_FAULT_RESPONSE, 0xB8) // shutdown 0x45
    SETREGVAL_PMBUS(TPSM8D6C24, IOUT_OC_FAULT_LIMIT, 0xF8D0)  // 52A per phase 0x46
    SETREGVAL_PMBUS(TPSM8D6C24, IOUT_OC_FAULT_RESPONSE, 0xB8) // shutdown 0x47
    SETREGVAL_PMBUS(TPSM8D6C24, IOUT_OC_WARN_LIMIT, 0xF8A0)   // 40A per phase 0x4A
    SETREGVAL_PMBUS(TPSM8D6C24, OT_FAULT_LIMIT, 0xF200)       // 128 degC 0x4F
    SETREGVAL_PMBUS(TPSM8D6C24, OT_FAULT_RESPONSE, 0xB8)      // shutdown 0x50
    SETREGVAL_PMBUS(TPSM8D6C24, OT_WARN_LIMIT, 0xF200)        // 128 degC 0x51
    SETREGVAL_PMBUS(TPSM8D6C24, VIN_OV_FAULT_LIMIT, 0xD3E0)   // 15.5V 	0x55
    SETREGVAL_PMBUS(TPSM8D6C24, VIN_OV_FAULT_RESPONSE, 0xB8)  // shutdown 0x56
    SETREGVAL_PMBUS(TPSM8D6C24, VIN_UV_WARN_LIMIT, 0xD2B0)    // 10.75V 	0x58
    SETREGVAL_PMBUS(TPSM8D6C24, TON_DELAY, 0x8000)            // 0 ms 	0x60
    SETREGVAL_PMBUS(TPSM8D6C24, TON_MAX_FAULT_RESPONSE, 0xB8) // shutdown 0x63
    SETREGVAL_PMBUS(TPSM8D6C24, TOFF_DELAY, 0x8000)           // 0 ms 	0x64
    SETREGVAL_PMBUS(TPSM8D6C24, TOFF_FALL, 0xC300)            // 3 ms 	0x65
    SETREGVAL_PMBUS(TPSM8D6C24, CLEAR_FAULT, 0)               // 0x03  0 is dummy byte, this write has no data
    SETREGVAL_PMBUS(TPSM8D6C24, TON_MAX_FAULT_LIMIT, 0xD280)  // 0x62  10 ms
    SETREGVAL_PMBUS(TPSM8D6C24, TON_RISE, 0xBE00)             // 0x61  3ms
    SETREGVAL_PMBUS(TPSM8D6C24, VOUT_MAX, 0x0280)             // 0x24  1.250 V
    SETREGVAL_PMBUS(TPSM8D6C24, VOUT_MIN, 0x0100)             // 0x2B  0.5V
    SETREGVAL_PMBUS(TPSM8D6C24, FREQUENCY_SWITCH, 0x0226)     // 0x33  550kHz
    SETCHKREGVAL_PMBUS(TPSM8D6C24, VOUT_COMMAND, 0x180)       //  0x21 0.750 V

    SETREGVAL_PMBUS(TPSM8D6C24, OPERATION, 0x80) // 0x01  reenable
}
