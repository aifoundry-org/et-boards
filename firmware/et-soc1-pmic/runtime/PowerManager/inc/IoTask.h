/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file IOTask.h
    \brief IO Task.
*/

#ifndef IOTASK_H
#define IOTASK_H

#include "globals.h"
#include "power_manager_event.h"

/***********************************************************************
 * GLOBAL Functions:   Declaration of global functions
***********************************************************************/

void powerManagerInitialize(void);

void IOTask(void *pvParameters);
void fastTask(void *pvParameters);

void gpioUp(powerState_t *pps);
void gpioDown(powerState_t *pps);

void cmdPowerOn(powerState_t *pps);
void cmdPowerOff(powerState_t *pps);
void cmdResetSoc(void);
void cmdSpiPwr(powerState_t *pps);
bool checkRegu(void);
int dumpFS1406Regu(void);
void setRegChk(const char *p1, powerState_t *pps);
bool checkVoltageRegisters(void);
bool checkFS1406Comm(void);

bool enqueueCommandFasttask(powerManagerEventType_t eventType,
    BaseType_t *pxHigherPriorityTaskWoken); //TODO: will be removed when Logging task is implemented

uint32_t getPerstDownCount(void);
void incrementPerstDownCount(void);
bool getPowerState(void);

#endif /* IOTASK_H */
