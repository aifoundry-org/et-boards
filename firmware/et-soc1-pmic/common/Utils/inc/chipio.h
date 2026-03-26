/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file chipio.h
    \brief Chip IO related.
*/

#ifndef CHIPIO_H
#define CHIPIO_H

#include "utils.h"
#include "scripts.h"
#include "iostructs.h"

void runScript(const char *p, t_cprf *pcprf, powerState_t *pps);

void chipInit(t_cprf *pcprf);
void pmbReadback(uint8_t addr7);

void cmdI2cScan(void);
int hwSetRegVal(uint8_t chipid, uint16_t regnum, uint8_t regProtcl, uint8_t reglen, uint64_t regval, t_cprf *pcprf);
int hwSetChkRegVal(uint8_t chipid, uint16_t regnum, uint8_t regProtcl, uint8_t reglen, uint64_t regval, t_cprf *pcprf);
int hwGetRegVal(uint8_t chipid, uint16_t regnum, uint8_t regProtcl, uint8_t reglen, t_cprf *pcprf);
int hwChkRegVal(uint8_t chipid, uint16_t regnum, uint8_t regProtcl, uint8_t reglen, uint64_t val, t_cprf *pcprf);
int hwPause(uint32_t ms, const t_cprf *pcprf);

void cmdSetGpio(const char *, const char *);
void cmdSetChip(const char *, t_cprf *);
void cmdSetReg(const char *, const char *, t_cprf *);
void cmdReadRegister(t_cprf *);
void cmdSetRegisterValue(const char *, t_cprf *);
void cmdSetField(const char *, t_cprf *);
void cmdModifyField(const char *, t_cprf *);
void cmdModifyFieldDec(const char *, t_cprf *);
void cmdModifyFieldCommon(uint32_t, t_cprf *);
void cmdModifyFieldCommon(uint32_t, t_cprf *);
void cmdWriteoutRegister(const char *, const t_cprf *);
void doI2cWrite(const char *p, char **plinep);
void doI2cRead(char *p1, const char *p2, const char *p3, bool repeat);
void doI2cWriteRead(const char *p1, const char *p2, char **plinep);
void execScript(scriptStruct_t *psl, t_cprf *pcprf, powerState_t *);
void cmdT(char const *p1);
int cmdU(char const *p1);
void cmdTimedisp(char const *p1);
void dumpFan(void);
void cmdFS(char *p1, const char *p2);

#endif /* CHIPIO_H */
