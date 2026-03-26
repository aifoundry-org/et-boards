/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file i2cm.h
    \brief I2C Master
*/

#ifndef __I2CM_H__
#define __I2CM_H__

#include "samd20.h"
#include "i2cm2.h"

#define NUM_I2CM 2

typedef struct {
    uint8_t addr, reg, lng;
    void *pVal;
} t_I2cmScanItem;

#define NREG 7
typedef struct {
    t_I2cmScanItem items[NREG];
} t_pmbScanItem;

typedef struct {
    t_pmbScanItem const **scanList;
    uint8_t *rvList;
    uint8_t *nScan;
} t_I2cmScanBlock;

void I2cm2Task(void *pvParameters);
void i2cmScanBlock(t_I2cmScanBlock const *_scanList);

static inline void i2cm_wait_for_sync(Sercom *dev)
{
    while (dev->I2CM.STATUS.reg & SERCOM_I2CM_STATUS_SYNCBUSY)
    {
    };
}

int i2cmInit(void);

#endif // __I2CM_H__
