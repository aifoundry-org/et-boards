/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file pmbstats.h
    \brief Pmb stats.
*/

#ifndef __PMBSTATS_H__
#define __PMBSTATS_H__

#include "i2cm.h"

enum { echn_MNN, echn_NOC, echn_SRM, echn_SRM2 };
#define NCHAN     (echn_SRM + 1)
#define NSCANCHAN (NCHAN + 1)
#define NSTAT     4

typedef struct {
    uint16_t cur, min, max;
    uint32_t ave;
    uint16_t rawcur, rawmin, rawmax;
} pmbVars_t;

typedef struct {
    union {
        pmbVars_t vars;
        struct {
            uint16_t stats[3];
            uint32_t ave;
            uint16_t raw[3];
        } arr;
    };
} pmbStat_t;

typedef union {
    uint64_t u64;
    uint16_t u16[4];
} u64U16_t;

typedef struct {
    pmbStat_t item[NCHAN][NREG];
    u64U16_t nSamples;
    u64U16_t nFails;
} pmbStats_t;

extern pmbStats_t pmbStats;
int initPmbStats(void);
void printPmbStats(void);
int readPmbStats(void); // RAMFUNC;
int updatePmbStats(void);
uint64_t getPmbStatsNumOfFails(void);
int getPmbStatsMnnVout(void);
int getPmbStatsNocVout(void);
int getPmbStatsSrmVout(void);

extern char const *pmbId[NCHAN];
extern uint8_t const pmbRegs[NREG];
extern char const *pmbRegNames[NREG];
extern char const *pmbStatNames[NSTAT];

extern pmbStats_t pmbStatsSnapshot;
extern uint8_t pmbStatsReadDevc;
extern uint8_t pmbStatsReadReg;
extern uint8_t pmbStatsReadStat;

static inline void wrPmbStats2(uint8_t cmd)
{
    uint8_t s = (cmd >> 5) & 0x03;
    uint8_t r = (cmd >> 2) & 0x07;
    uint8_t d = (cmd >> 0) & 0x03;
    switch (cmd)
    {
        case 0x47:
            initPmbStats();
            // fall through
        case 0x43:
            IOSEM_TAKE
            memcpy(&pmbStatsSnapshot, &pmbStats, sizeof(pmbStats_t)); // temporarily block readPmbStats()
            IOSEM_GIVE
            pmbStatsReadDevc = 0;
            pmbStatsReadReg = 0;
            pmbStatsReadStat = 0;
            updatePmbStats();
            break;
        default:
            if (r > 6)
                break;
            if (d == 3)
            {
                if (s >= 2)
                    break;
                if (r >= 4)
                    break;
            }
            pmbStatsReadDevc = d;
            pmbStatsReadReg = r;
            pmbStatsReadStat = s;
            updatePmbStats();
            break;
    }
}

#ifdef VREG_INSTANTIATE
char const *pmbId[NCHAN] = { "MNN", "NOC", "SRM" };
uint8_t const pmbRegs[NREG] = { 0xFF, PMBUS__READ_IOUT, PMBUS__READ_POUT, PMBUS__READ_VIN, PMBUS__READ_IIN,
    PMBUS__READ_PIN, PMBUS__READ_TEMPERATURE_1 };
char const *pmbRegNames[NREG] = { "V_out", "A_out", "W_out", "V_in", "A_in", "W_in", "Deg_C" };
char const *pmbStatNames[NSTAT] = { "Cur", "Min", "Max", "Ave" };
#endif

#endif // __PMBSTATS_H__
