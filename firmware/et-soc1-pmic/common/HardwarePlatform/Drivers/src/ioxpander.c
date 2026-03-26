/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file ioxpander.c
    \brief IO expander.
*/
/***********************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "ioxpander.h"
#include "gpio.h"

#include "i2cm2.h"
#include "PCA9575.h"
#include "boardchipinfo.h"
#include "globals.h"

/***********************************************************************
 * Macros:  Defininition of local macros
***********************************************************************/
#define PINS_PER_IOXP_PORT (8)
#define NUM_IOXP           (2)

#define RESET_ASSERTION   (-1)
#define RESET_CYCLE       (0)
#define RESET_DEASSERTION (1)

/***********************************************************************
 * Data types:      Defininition of data types
***********************************************************************/
typedef struct {
    uint8_t addr;
    uint8_t val;
} initVal_t;

/***********************************************************************
 * GLOBAL variables:  Defininition of global variables and constants
***********************************************************************/
static uint8_t const addresses[NUM_IOXP] = { ADR_PCA9575_0, ADR_PCA9575_1 };

static uint8_t ioxpInputState[NUM_IOXP];
static uint8_t ioxpOutputState[NUM_IOXP];

gpioId_t ioxpResetPinId[NUM_IOXP] = { IOXP_0_RST_OUT, IOXP_1_RST_OUT };

/***********************************************************************
 * Functions:   Declaration of local functions
***********************************************************************/
static void ioxpander_reset(int inout);

/***********************************************************************
 * GLOBAL Functions:   Defininition of global functions
***********************************************************************/
bool ioxpander_init(void)
{
    static initVal_t const initVals[NUM_IOXP][4] = {
        {                            // IOXP0
            { PCA9575__OUT1, 0x00 }, // port 1 outputs are 0 - should already be 0 but aren't
            { PCA9575__CFG1, 0x00 }, // port 1 pins are all outputs
            { PCA9575__MSK0, 0x80 }, // enable all port 0 interrupts except unused p0_7
            { 0xFF } },
        {                            // IOXP1
            { PCA9575__OUT1, 0x00 }, // port 1 outputs are 0 - should already be 0 but aren't
            { PCA9575__CFG1, 0x00 }, // port 1 pins are all outputs
            { PCA9575__MSK0, 0xC0 }, // enable all port 0 interrupts except SRAM_EN_SENSE and unused p0_7
            { 0xFF } },
    };

    ioxpander_reset(RESET_DEASSERTION);

    memset(ioxpOutputState, 0, sizeof(ioxpOutputState));

    for (uint8_t iIoxp = 0; iIoxp < NUM_IOXP; ++iIoxp)
    {
        uint8_t addr = addresses[iIoxp];

        uint8_t v = 0;
        if (i2cm_read8(addr, PCA9575__OUT1, &v) != I2CM_OK)
        {
            return 0;
        }
        ioxpOutputState[iIoxp] = v;

        for (initVal_t const *q = initVals[iIoxp]; q->addr != 0xFF; ++q)
        {
            if (i2cm_write8(addr, q->addr, q->val) != I2CM_OK)
            {
                return 0;
            }
        }
        v = 0;
        if (i2cm_read8(addr, PCA9575__IN0, &v) != I2CM_OK)
        {
            return 0;
        }
        ioxpInputState[iIoxp] = v;
    }
    return 1;
}

uint8_t ioxpander_read_in_state(uint8_t iIoxp)
{
    IOXPREADSEM_TAKE
    uint8_t addr = addresses[iIoxp];
    uint8_t v = 0;
    i2cm_read8(addr, PCA9575__IN0, &v);
    ioxpInputState[iIoxp] = v;
    IOXPREADSEM_GIVE
    return v;
}

void ioxpander_update_all_input_state(void)
{
    for (uint8_t iIoxp = 0; iIoxp < NUM_IOXP; iIoxp++)
    {
        ioxpander_read_in_state(iIoxp);
    }
}

uint8_t ioxpander_get_in_saved_state(uint8_t iIoxp)
{
    IOXPREADSEM_TAKE
    uint8_t v = ioxpInputState[iIoxp];
    IOXPREADSEM_GIVE
    return v;
}

int ioxpander_get_base_pin(uint8_t iIoxp, uint8_t *ioxpBasePin)
{
    int status = STATUS_SUCCESS;
    if (iIoxp == 0)
    {
        *ioxpBasePin = EXPANDER_0;
    }
    else if (iIoxp == 1)
    {
        *ioxpBasePin = EXPANDER_1;
    }
    else
    {
        status = IOXP_ERROR_UNSUPPORTED_INDEX;
    }
    return status;
}

void ioxpander_set_out(uint8_t pin, bool state)
{
    pin -= EXPANDER_0;                                     // ioxp group relative pin code
    uint8_t iIoxp = pin / (NUM_IOXP * PINS_PER_IOXP_PORT); // ioxp 0 or 1
    pin -= iIoxp * NUM_IOXP * PINS_PER_IOXP_PORT;          // pin on ioxp
    assertMsg(iIoxp < NUM_IOXP, "bad iIoxp");
    assertMsg(pin >= PINS_PER_IOXP_PORT, "not output"); // port 0 is inputs, port 1 is outputs
    pin -= PINS_PER_IOXP_PORT;                          // pin on ioxp port 1
    uint8_t addr = addresses[iIoxp];                    // ioxp i2c address
    uint8_t mask = 1 << (pin % PINS_PER_IOXP_PORT);     // bit mask for pin on 8 bit port
    uint8_t *pv = &ioxpOutputState[iIoxp];              // address of local copy of port state
    uint8_t v = *pv;
    v = state ? v | mask : v & ~mask;    // set or clear the single bit
    *pv = v;                             // save local state
    i2cm_write8(addr, PCA9575__OUT1, v); // set ioxpander output
}

bool ioxpander_get_out(uint8_t pin)
{
    pin -= EXPANDER_0;
    uint8_t iIoxp = pin / (NUM_IOXP * PINS_PER_IOXP_PORT);
    pin -= iIoxp * NUM_IOXP * PINS_PER_IOXP_PORT;
    pin -= PINS_PER_IOXP_PORT;
    return (ioxpOutputState[iIoxp] >> pin) & 0x01;
}

bool ioxpander_get_in_saved(uint8_t pin)
{
    IOXPREADSEM_TAKE
    uint8_t iIoxp = (pin >= GPIO1_P0_0);
    uint8_t ioxpPin = pin - (iIoxp ? GPIO1_P0_0 : GPIO0_P0_0);
    bool v = ((ioxpInputState[iIoxp] >> ioxpPin) & 0x01) != 0;
    IOXPREADSEM_GIVE
    return v;
}

/***********************************************************************
 * Functions:   Definition of local functions
***********************************************************************/
static void ioxpander_reset(int inout)
{
    taskENTER_CRITICAL();
    for (uint8_t i = 0; i < NUM_IOXP; ++i)
    {
        if (inout <= 0)
        {
            gpio_set_out_low_pin_id(ioxpResetPinId[i]);
        }
        if (inout >= 0)
        {
            gpio_set_out_high_pin_id(ioxpResetPinId[i]);
        }
    }
    taskEXIT_CRITICAL();
}
