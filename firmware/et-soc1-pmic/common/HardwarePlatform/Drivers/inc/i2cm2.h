/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file i2cm2.h
    \brief I2C Master
*/


#ifndef I2CM2_H
#define I2CM2_H

#include <compiler.h>
#include "etsoc_cmd_handler_task.h" //todo SW-16870: to be replaced with pmic_hal shared file

i2cmErr_t i2cm_read1w(uint8_t addr7, uint8_t reg,  uint8_t *rx, uint32_t rxlen);
i2cmErr_t i2cm_write0( uint8_t addr7, uint8_t reg );
i2cmErr_t i2cm_write8( uint8_t addr7, uint8_t reg, uint8_t data );
i2cmErr_t i2cm_write16( uint8_t addr7, uint8_t reg, uint16_t data );
i2cmErr_t i2cm_write32( uint8_t addr7, uint8_t reg, uint32_t data );

i2cmErr_t i2cm_write8or16( uint8_t addr7, uint8_t reg, uint16_t data, uint32_t nBytes );
i2cmErr_t i2cm_read8( uint8_t addr7, uint8_t reg, uint8_t * pdata );
i2cmErr_t i2cm_read16( uint8_t addr7, uint8_t reg, uint16_t * pdata );
i2cmErr_t i2cm_read8or16( uint8_t addr7, uint8_t reg, uint16_t * pdata, uint32_t nBytes );
i2cmErr_t i2cm_write_blk_word( uint8_t addr7, uint8_t reg, uint64_t data, uint8_t lng );
i2cmErr_t i2cm_read_blk_word( uint8_t addr7, uint8_t reg, uint64_t * pdata, uint8_t lng );
i2cmErr_t i2cm_write(uint8_t addr7, uint8_t *tx, uint32_t txlen);
i2cmErr_t i2cm_readPageless(uint8_t address, uint8_t *tx,  uint32_t txlen, uint8_t *rx, uint32_t rxlen);
i2cmErr_t i2cm_read(uint8_t addr7, uint8_t *tx,  uint32_t txlen, uint8_t *rx, uint32_t rxlen);
i2cmErr_t i2cm_probe(uint8_t addr7);
i2cmErr_t i2cm_get_STATUS(void);
const char *i2cm_get_error_message(i2cmErr_t err);


#endif /* I2CM2_H */
