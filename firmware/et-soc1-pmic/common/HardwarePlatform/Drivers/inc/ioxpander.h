/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file ioxpander.h
    \brief IO expander related.
*/
 
#ifndef __IOXPANDER_H__
#define __IOXPANDER_H__

#include <stdint.h>

/***********************************************************************
 * GLOBAL Functions:   Defininition of global functions
***********************************************************************/
bool ioxpander_init(void);
uint8_t ioxpander_read_in_state(uint8_t iIoxp);
void ioxpander_update_all_input_state(void);
uint8_t ioxpander_get_in_saved_state(uint8_t iIoxp);
int ioxpander_get_base_pin(uint8_t iIoxp, uint8_t *ioxpBasePin);

void ioxpander_set_out(uint8_t pin, bool state); // sets output state
bool ioxpander_get_out(uint8_t pin);             // reads output state (set by above function)
bool ioxpander_get_in_saved(uint8_t pin);

#endif /*__IOXPANDER_H__*/

