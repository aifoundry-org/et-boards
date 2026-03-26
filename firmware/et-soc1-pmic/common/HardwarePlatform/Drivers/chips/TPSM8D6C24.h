/***********************************************************************
 *
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *
 ************************************************************************/
#ifndef TPSM8D6C24_H
#define TPSM8D6C24_H
#include <stdint.h>
#include "chips.h"
#include "chipio.h"
#include "chipmacro.h"

#define TPSM8D6C24__MFR_ID                           0x99
#define TPSM8D6C24__MFR_ID__PROT                     eRT_RBLK
#define TPSM8D6C24__MFR_ID__BYTES                    4
#define TPSM8D6C24__MFR_MODEL                        0x9A
#define TPSM8D6C24__MFR_MODEL__PROT                  eRT_RBLK
#define TPSM8D6C24__MFR_MODEL__BYTES                 4
#define TPSM8D6C24__MFR_COMPENSATION_CONFIG          0xB1
#define TPSM8D6C24__MFR_COMPENSATION_CONFIG__PROT    eRT_RWBLK
#define TPSM8D6C24__MFR_COMPENSATION_CONFIG__BYTES   6
#define TPSM8D6C24__MFR_POWER_STAGE_CONFIG           0xB5
#define TPSM8D6C24__MFR_POWER_STAGE_CONFIG__PROT     eRT_RWBLK
#define TPSM8D6C24__MFR_POWER_STAGE_CONFIG__BYTES    2
#define TPSM8D6C24__MFR_TELEMETRY_CONFIG             0xD0
#define TPSM8D6C24__MFR_TELEMETRY_CONFIG__PROT       eRT_RWBLK
#define TPSM8D6C24__MFR_TELEMETRY_CONFIG__BYTES      7
#define TPSM8D6C24__MFR_READ_ALL                     0xDA
#define TPSM8D6C24__MFR_READ_ALL__PROT               eRT_RBLK
#define TPSM8D6C24__MFR_READ_ALL__BYTES              15
#define TPSM8D6C24__MFR_STATUS_ALL                   0xDB
#define TPSM8D6C24__MFR_STATUS_ALL__PROT             eRT_RBLK
#define TPSM8D6C24__MFR_STATUS_ALL__BYTES            8
#define TPSM8D6C24__MFR_SYNC_CONFIG                  0xE4
#define TPSM8D6C24__MFR_SYNC_CONFIG__PROT            eRT_RW8
#define TPSM8D6C24__MFR_SYNC_CONFIG__BYTES           1
#define TPSM8D6C24__MFR_STACK_CONFIG                 0xEC
#define TPSM8D6C24__MFR_STACK_CONFIG__PROT           eRT_RW16
#define TPSM8D6C24__MFR_STACK_CONFIG__BYTES          2
#define TPSM8D6C24__MFR_MISC_OPTIONS                 0xED
#define TPSM8D6C24__MFR_MISC_OPTIONS__PROT           eRT_RW16
#define TPSM8D6C24__MFR_MISC_OPTIONS__BYTES          2
#define TPSM8D6C24__MFR_PIN_DETECT_OVERRIDE          0xEE
#define TPSM8D6C24__MFR_PIN_DETECT_OVERRIDE__PROT    eRT_RW16
#define TPSM8D6C24__MFR_PIN_DETECT_OVERRIDE__BYTES   2
#define TPSM8D6C24__MFR_LOOP_FOLLOWER_ADDRESS        0xEF
#define TPSM8D6C24__MFR_LOOP_FOLLOWER_ADDRESS__PROT  eRT_RW8
#define TPSM8D6C24__MFR_LOOP_FOLLOWER_ADDRESS__BYTES 1
#define TPSM8D6C24__MFR_NVM_CHECKSUM                 0xF0
#define TPSM8D6C24__MFR_NVM_CHECKSUM__PROT           eRT_R16
#define TPSM8D6C24__MFR_NVM_CHECKSUM__BYTES          2
#define TPSM8D6C24__MFR_SIMULATE_FAULTS              0xF1
#define TPSM8D6C24__MFR_SIMULATE_FAULTS__PROT        eRT_RW16
#define TPSM8D6C24__MFR_SIMULATE_FAULTS__BYTES       2
#define TPSM8D6C24__MFR_FUSION_ID0                   0xFC
#define TPSM8D6C24__MFR_FUSION_ID0__PROT             eRT_RW16
#define TPSM8D6C24__MFR_FUSION_ID0__BYTES            2
#define TPSM8D6C24__MFR_FUSION_ID1                   0xFD
#define TPSM8D6C24__MFR_FUSION_ID1__PROT             eRT_RBLK
#define TPSM8D6C24__MFR_FUSION_ID1__BYTES            7

extern t_structFieldInfo const structFieldInfo_TPSM8D6C24[];

#ifdef INSTANTIATE_DEVINFO

t_structFieldInfo const structFieldInfo_TPSM8D6C24[] = {};

#endif /* INSTANTIATE_DEVINFO */

void TPSM8D6C24_init(t_cprf *pcprf);

#endif /* TPSM8D6C24_H */
