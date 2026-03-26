/***********************************************************************
 *
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *
 ************************************************************************/

#ifndef PMBUS_H
#define PMBUS_H

#include "chips.h"
#include <stdint.h>

#define PMBUS__PAGE                                      0x00
#define PMBUS__PAGE__PROT                                eRT_RW8
#define PMBUS__PAGE__BYTES                               1
#define PMBUS__OPERATION                                 0x01
#define PMBUS__OPERATION__PROT                           eRT_RW8
#define PMBUS__OPERATION__BYTES                          1
#define PMBUS__ON_OFF_CONFIG                             0x02
#define PMBUS__ON_OFF_CONFIG__PROT                       eRT_RW8
#define PMBUS__ON_OFF_CONFIG__BYTES                      1
#define PMBUS__CLEAR_FAULT                               0x03
#define PMBUS__CLEAR_FAULT__PROT                         eRT_W0
#define PMBUS__CLEAR_FAULT__BYTES                        0
#define PMBUS__PHASE                                     0x04
#define PMBUS__PHASE__PROT                               eRT_RW8
#define PMBUS__PHASE__BYTES                              1
#define PMBUS__PAGE_PLUS_WRITE                           0x05
#define PMBUS__PAGE_PLUS_WRITE__PROT                     eRT_WBLK
#define PMBUS__PAGE_PLUS_WRITE__BYTES                    6
#define PMBUS__PAGE_PLUS_READ                            0x06
#define PMBUS__PAGE_PLUS_READ__PROT                      eRT_RWBLK
#define PMBUS__PAGE_PLUS_READ__BYTES                     3
#define PMBUS__WRITE_PROTECT                             0x10
#define PMBUS__WRITE_PROTECT__PROT                       eRT_RW8
#define PMBUS__WRITE_PROTECT__BYTES                      1
#define PMBUS__STORE_DEFAULT_ALL                         0x11
#define PMBUS__STORE_DEFAULT_ALL__PROT                   eRT_W0
#define PMBUS__STORE_DEFAULT_ALL__BYTES                  0
#define PMBUS__RESTORE_DEFAULT_ALL                       0x12
#define PMBUS__RESTORE_DEFAULT_ALL__PROT                 eRT_W0
#define PMBUS__RESTORE_DEFAULT_ALL__BYTES                0
#define PMBUS__STORE_USER_ALL                            0x15
#define PMBUS__STORE_USER_ALL__PROT                      eRT_W0
#define PMBUS__STORE_USER_ALL__BYTES                     0
#define PMBUS__RESTORE_USER_ALL                          0x16
#define PMBUS__RESTORE_USER_ALL__PROT                    eRT_W0
#define PMBUS__RESTORE_USER_ALL__BYTES                   0
#define PMBUS__CAPABILITY                                0x19
#define PMBUS__CAPABILITY__PROT                          eRT_R8
#define PMBUS__CAPABILITY__BYTES                         1
#define PMBUS__SMBALERT_MASK                             0x1B
#define PMBUS__SMBALERT_MASK__PROT                       eRT_SMBALERT_MASK
#define PMBUS__SMBALERT_MASK__BYTES                      1
#define PMBUS__VOUT_MODE                                 0x20
#define PMBUS__VOUT_MODE__PROT                           eRT_R8
#define PMBUS__VOUT_MODE__BYTES                          1
#define PMBUS__VOUT_COMMAND                              0x21
#define PMBUS__VOUT_COMMAND__PROT                        eRT_RW16
#define PMBUS__VOUT_COMMAND__BYTES                       2
#define PMBUS__VOUT_MAX                                  0x24
#define PMBUS__VOUT_MAX__PROT                            eRT_RW16
#define PMBUS__VOUT_MAX__BYTES                           2
#define PMBUS__VOUT_MARGIN_HIGH                          0x25
#define PMBUS__VOUT_MARGIN_HIGH__PROT                    eRT_RW16
#define PMBUS__VOUT_MARGIN_HIGH__BYTES                   2
#define PMBUS__VOUT_MARGIN_LOW                           0x26
#define PMBUS__VOUT_MARGIN_LOW__PROT                     eRT_RW16
#define PMBUS__VOUT_MARGIN_LOW__BYTES                    2
#define PMBUS__VOUT_TRANSITION_RATE                      0x27
#define PMBUS__VOUT_TRANSITION_RATE__PROT                eRT_RW16
#define PMBUS__VOUT_TRANSITION_RATE__BYTES               2
#define PMBUS__VOUT_DROOP                                0x28
#define PMBUS__VOUT_DROOP__PROT                          eRT_RW16
#define PMBUS__VOUT_DROOP__BYTES                         2
#define PMBUS__VOUT_SCALE_LOOP                           0x29
#define PMBUS__VOUT_SCALE_LOOP__PROT                     eRT_RW16
#define PMBUS__VOUT_SCALE_LOOP__BYTES                    2
#define PMBUS__VOUT_SCALE_MONITOR                        0x2A
#define PMBUS__VOUT_SCALE_MONITOR__PROT                  eRT_RW16
#define PMBUS__VOUT_SCALE_MONITOR__BYTES                 2
#define PMBUS__VOUT_MIN                                  0x2B
#define PMBUS__VOUT_MIN__PROT                            eRT_RW16
#define PMBUS__VOUT_MIN__BYTES                           2
#define PMBUS__FREQUENCY_SWITCH                          0x33
#define PMBUS__FREQUENCY_SWITCH__PROT                    eRT_RW16
#define PMBUS__FREQUENCY_SWITCH__BYTES                   2
#define PMBUS__VIN_ON                                    0x35
#define PMBUS__VIN_ON__PROT                              eRT_RW16
#define PMBUS__VIN_ON__BYTES                             2
#define PMBUS__VIN_OFF                                   0x36
#define PMBUS__VIN_OFF__PROT                             eRT_RW16
#define PMBUS__VIN_OFF__BYTES                            2
#define PMBUS__IOUT_CAL_GAIN                             0x38
#define PMBUS__IOUT_CAL_GAIN__PROT                       eRT_RW16
#define PMBUS__IOUT_CAL_GAIN__BYTES                      2
#define PMBUS__IOUT_CAL_OFFSET                           0x39
#define PMBUS__IOUT_CAL_OFFSET__PROT                     eRT_RW16
#define PMBUS__IOUT_CAL_OFFSET__BYTES                    2
#define PMBUS__VOUT_OV_FAULT_LIMIT                       0x40
#define PMBUS__VOUT_OV_FAULT_LIMIT__PROT                 eRT_RW16
#define PMBUS__VOUT_OV_FAULT_LIMIT__BYTES                2
#define PMBUS__VOUT_OV_FAULT_RESPONSE                    0x41
#define PMBUS__VOUT_OV_FAULT_RESPONSE__PROT              eRT_RW8
#define PMBUS__VOUT_OV_FAULT_RESPONSE__BYTES             1
#define PMBUS__VOUT_OV_WARN_LIMIT                        0x42
#define PMBUS__VOUT_OV_WARN_LIMIT__PROT                  eRT_RW16
#define PMBUS__VOUT_OV_WARN_LIMIT__BYTES                 2
#define PMBUS__VOUT_UV_WARN_LIMIT                        0x43
#define PMBUS__VOUT_UV_WARN_LIMIT__PROT                  eRT_RW16
#define PMBUS__VOUT_UV_WARN_LIMIT__BYTES                 2
#define PMBUS__VOUT_UV_FAULT_LIMIT                       0x44
#define PMBUS__VOUT_UV_FAULT_LIMIT__PROT                 eRT_RW16
#define PMBUS__VOUT_UV_FAULT_LIMIT__BYTES                2
#define PMBUS__VOUT_UV_FAULT_RESPONSE                    0x45
#define PMBUS__VOUT_UV_FAULT_RESPONSE__PROT              eRT_RW8
#define PMBUS__VOUT_UV_FAULT_RESPONSE__BYTES             1
#define PMBUS__IOUT_OC_FAULT_LIMIT                       0x46
#define PMBUS__IOUT_OC_FAULT_LIMIT__PROT                 eRT_RW16
#define PMBUS__IOUT_OC_FAULT_LIMIT__BYTES                2
#define PMBUS__IOUT_OC_FAULT_RESPONSE                    0x47
#define PMBUS__IOUT_OC_FAULT_RESPONSE__PROT              eRT_RW8
#define PMBUS__IOUT_OC_FAULT_RESPONSE__BYTES             1
#define PMBUS__IOUT_OC_WARN_LIMIT                        0x4A
#define PMBUS__IOUT_OC_WARN_LIMIT__PROT                  eRT_RW16
#define PMBUS__IOUT_OC_WARN_LIMIT__BYTES                 2
#define PMBUS__OT_FAULT_LIMIT                            0x4F
#define PMBUS__OT_FAULT_LIMIT__PROT                      eRT_RW16
#define PMBUS__OT_FAULT_LIMIT__BYTES                     2
#define PMBUS__OT_FAULT_RESPONSE                         0x50
#define PMBUS__OT_FAULT_RESPONSE__PROT                   eRT_RW8
#define PMBUS__OT_FAULT_RESPONSE__BYTES                  1
#define PMBUS__OT_WARN_LIMIT                             0x51
#define PMBUS__OT_WARN_LIMIT__PROT                       eRT_RW16
#define PMBUS__OT_WARN_LIMIT__BYTES                      2
#define PMBUS__UT_FAULT_LIMIT                            0x53
#define PMBUS__UT_FAULT_LIMIT__PROT                      eRT_RW16
#define PMBUS__UT_FAULT_LIMIT__BYTES                     2
#define PMBUS__UT_FAULT_RESPONSE                         0x54
#define PMBUS__UT_FAULT_RESPONSE__PROT                   eRT_RW8
#define PMBUS__UT_FAULT_RESPONSE__BYTES                  1
#define PMBUS__VIN_UV_WARN_LIMIT                         0x58
#define PMBUS__VIN_UV_WARN_LIMIT__PROT                   eRT_RW16
#define PMBUS__VIN_UV_WARN_LIMIT__BYTES                  2
#define PMBUS__VIN_OV_FAULT_LIMIT                        0x55
#define PMBUS__VIN_OV_FAULT_LIMIT__PROT                  eRT_RW16
#define PMBUS__VIN_OV_FAULT_LIMIT__BYTES                 2
#define PMBUS__VIN_OV_FAULT_RESPONSE                     0x56
#define PMBUS__VIN_OV_FAULT_RESPONSE__PROT               eRT_RW8
#define PMBUS__VIN_OV_FAULT_RESPONSE__BYTES              1
#define PMBUS__VIN_UV_FAULT_LIMIT                        0x59
#define PMBUS__VIN_UV_FAULT_LIMIT__PROT                  eRT_RW16
#define PMBUS__VIN_UV_FAULT_LIMIT__BYTES                 2
#define PMBUS__VIN_UV_FAULT_RESPONSE                     0x5A
#define PMBUS__VIN_UV_FAULT_RESPONSE__PROT               eRT_R8
#define PMBUS__VIN_UV_FAULT_RESPONSE__BYTES              1
#define PMBUS__IIN_OC_FAULT_LIMIT                        0x5B
#define PMBUS__IIN_OC_FAULT_LIMIT__PROT                  eRT_RW16
#define PMBUS__IIN_OC_FAULT_LIMIT__BYTES                 2
#define PMBUS__IIN_OC_FAULT_RESPONSE                     0x5C
#define PMBUS__IIN_OC_FAULT_RESPONSE__PROT               eRT_R8
#define PMBUS__IIN_OC_FAULT_RESPONSE__BYTES              1
#define PMBUS__IIN_OC_WARN_LIMIT                         0x5D
#define PMBUS__IIN_OC_WARN_LIMIT__PROT                   eRT_RW16
#define PMBUS__IIN_OC_WARN_LIMIT__BYTES                  2
#define PMBUS__TON_DELAY                                 0x60
#define PMBUS__TON_DELAY__PROT                           eRT_RW16
#define PMBUS__TON_DELAY__BYTES                          2
#define PMBUS__TON_RISE                                  0x61
#define PMBUS__TON_RISE__PROT                            eRT_RW16
#define PMBUS__TON_RISE__BYTES                           2
#define PMBUS__TON_MAX_FAULT_LIMIT                       0x62
#define PMBUS__TON_MAX_FAULT_LIMIT__PROT                 eRT_RW16
#define PMBUS__TON_MAX_FAULT_LIMIT__BYTES                2
#define PMBUS__TON_MAX_FAULT_RESPONSE                    0x63
#define PMBUS__TON_MAX_FAULT_RESPONSE__PROT              eRT_RW8
#define PMBUS__TON_MAX_FAULT_RESPONSE__BYTES             1
#define PMBUS__TOFF_DELAY                                0x64
#define PMBUS__TOFF_DELAY__PROT                          eRT_RW16
#define PMBUS__TOFF_DELAY__BYTES                         2
#define PMBUS__TOFF_FALL                                 0x65
#define PMBUS__TOFF_FALL__PROT                           eRT_RW16
#define PMBUS__TOFF_FALL__BYTES                          2
#define PMBUS__TOFF_MAX_WARN_LIMIT                       0x66
#define PMBUS__TOFF_MAX_WARN_LIMIT__PROT                 eRT_RW16
#define PMBUS__TOFF_MAX_WARN_LIMIT__BYTES                2
#define PMBUS__PIN_OP_WARN_LIMIT                         0x6B
#define PMBUS__PIN_OP_WARN_LIMIT__PROT                   eRT_RW16
#define PMBUS__PIN_OP_WARN_LIMIT__BYTES                  2
#define PMBUS__STATUS_BYTE                               0x78
#define PMBUS__STATUS_BYTE__PROT                         eRT_RW8
#define PMBUS__STATUS_BYTE__BYTES                        1
#define PMBUS__STATUS_WORD                               0x79
#define PMBUS__STATUS_WORD__PROT                         eRT_RW16
#define PMBUS__STATUS_WORD__BYTES                        2
#define PMBUS__STATUS_VOUT                               0x7A
#define PMBUS__STATUS_VOUT__PROT                         eRT_RW8
#define PMBUS__STATUS_VOUT__BYTES                        1
#define PMBUS__STATUS_IOUT                               0x7B
#define PMBUS__STATUS_IOUT__PROT                         eRT_RW8
#define PMBUS__STATUS_IOUT__BYTES                        1
#define PMBUS__STATUS_INPUT                              0x7C
#define PMBUS__STATUS_INPUT__PROT                        eRT_RW8
#define PMBUS__STATUS_INPUT__BYTES                       1
#define PMBUS__STATUS_TEMPERATURE                        0x7D
#define PMBUS__STATUS_TEMPERATURE__PROT                  eRT_RW8
#define PMBUS__STATUS_TEMPERATURE__BYTES                 1
#define PMBUS__STATUS_CML                                0x7E
#define PMBUS__STATUS_CML__PROT                          eRT_RW8
#define PMBUS__STATUS_CML__BYTES                         1
#define PMBUS__STATUS_MFR_SPECIFIC                       0x80
#define PMBUS__STATUS_MFR_SPECIFIC__PROT                 eRT_RW8
#define PMBUS__STATUS_MFR_SPECIFIC__BYTES                1
#define PMBUS__READ_VIN                                  0x88
#define PMBUS__READ_VIN__PROT                            eRT_R16
#define PMBUS__READ_VIN__BYTES                           2
#define PMBUS__READ_IIN                                  0x89
#define PMBUS__READ_IIN__PROT                            eRT_R16
#define PMBUS__READ_IIN__BYTES                           2
#define PMBUS__READ_VOUT                                 0x8B
#define PMBUS__READ_VOUT__PROT                           eRT_R16
#define PMBUS__READ_VOUT__BYTES                          2
#define PMBUS__READ_IOUT                                 0x8C
#define PMBUS__READ_IOUT__PROT                           eRT_R16
#define PMBUS__READ_IOUT__BYTES                          2
#define PMBUS__READ_TEMPERATURE_1                        0x8D
#define PMBUS__READ_TEMPERATURE_1__PROT                  eRT_R16
#define PMBUS__READ_TEMPERATURE_1__BYTES                 2
#define PMBUS__READ_TEMPERATURE_2                        0x8E
#define PMBUS__READ_TEMPERATURE_2__PROT                  eRT_R16
#define PMBUS__READ_TEMPERATURE_2__BYTES                 2
#define PMBUS__READ_POUT                                 0x96
#define PMBUS__READ_POUT__PROT                           eRT_R16
#define PMBUS__READ_POUT__BYTES                          2
#define PMBUS__READ_PIN                                  0x97
#define PMBUS__READ_PIN__PROT                            eRT_R16
#define PMBUS__READ_PIN__BYTES                           2
#define PMBUS__PMBUS_REVISION                            0x98
#define PMBUS__PMBUS_REVISION__PROT                      eRT_R8
#define PMBUS__PMBUS_REVISION__BYTES                     1
#define PMBUS__IC_DEVICE_ID                              0xAD
#define PMBUS__IC_DEVICE_ID__PROT                        eRT_RBLK
#define PMBUS__IC_DEVICE_ID__BYTES                       2
#define PMBUS__IC_DEVICE_REV                             0xAE
#define PMBUS__IC_DEVICE_REV__PROT                       eRT_RBLK
#define PMBUS__IC_DEVICE_REV__BYTES                      2
#define PMBUS__USER_DATA_00                              0xB0
#define PMBUS__USER_DATA_00__PROT                        eRT_RWBLK
#define PMBUS__USER_DATA_00__BYTES                       6
#define PMBUS__USER_DATA_01                              0xB1
#define PMBUS__USER_DATA_01__PROT                        eRT_RWBLK
#define PMBUS__USER_DATA_01__BYTES                       6
#define PMBUS__USER_DATA_02                              0xB2
#define PMBUS__USER_DATA_02__PROT                        eRT_RWBLK
#define PMBUS__USER_DATA_02__BYTES                       6
#define PMBUS__USER_DATA_03                              0xB3
#define PMBUS__USER_DATA_03__PROT                        eRT_RWBLK
#define PMBUS__USER_DATA_03__BYTES                       6
#define PMBUS__USER_DATA_04                              0xB4
#define PMBUS__USER_DATA_04__PROT                        eRT_RWBLK
#define PMBUS__USER_DATA_04__BYTES                       6
#define PMBUS__USER_DATA_05                              0xB5
#define PMBUS__USER_DATA_05__PROT                        eRT_RWBLK
#define PMBUS__USER_DATA_05__BYTES                       6
#define PMBUS__USER_DATA_06                              0xB6
#define PMBUS__USER_DATA_06__PROT                        eRT_RWBLK
#define PMBUS__USER_DATA_06__BYTES                       6
#define PMBUS__USER_DATA_07                              0xB7
#define PMBUS__USER_DATA_07__PROT                        eRT_RWBLK
#define PMBUS__USER_DATA_07__BYTES                       6
#define PMBUS__USER_DATA_08                              0xB8
#define PMBUS__USER_DATA_08__PROT                        eRT_RWBLK
#define PMBUS__USER_DATA_08__BYTES                       6
#define PMBUS__USER_DATA_09                              0xB9
#define PMBUS__USER_DATA_09__PROT                        eRT_RWBLK
#define PMBUS__USER_DATA_09__BYTES                       6
#define PMBUS__USER_DATA_10                              0xBA
#define PMBUS__USER_DATA_10__PROT                        eRT_RWBLK
#define PMBUS__USER_DATA_10__BYTES                       6
#define PMBUS__USER_DATA_11                              0xBB
#define PMBUS__USER_DATA_11__PROT                        eRT_RWBLK
#define PMBUS__USER_DATA_11__BYTES                       6
#define PMBUS__USER_DATA_12                              0xBC
#define PMBUS__USER_DATA_12__PROT                        eRT_RWBLK
#define PMBUS__USER_DATA_12__BYTES                       6
#define PMBUS__PAGE__PAGE__SHIFT                         0
#define PMBUS__PAGE__PAGE__MASK                          0xFF
#define PMBUS__OPERATION__ON__SHIFT                      7
#define PMBUS__OPERATION__ON__MASK                       0x01
#define PMBUS__OPERATION__MARGIN__SHIFT                  2
#define PMBUS__OPERATION__MARGIN__MASK                   0x0F
#define PMBUS__ON_OFF_CONFIG__PU__SHIFT                  4
#define PMBUS__ON_OFF_CONFIG__PU__MASK                   0x01
#define PMBUS__ON_OFF_CONFIG__CMD__SHIFT                 3
#define PMBUS__ON_OFF_CONFIG__CMD__MASK                  0x01
#define PMBUS__ON_OFF_CONFIG__CP__SHIFT                  2
#define PMBUS__ON_OFF_CONFIG__CP__MASK                   0x01
#define PMBUS__ON_OFF_CONFIG__PL__SHIFT                  1
#define PMBUS__ON_OFF_CONFIG__PL__MASK                   0x01
#define PMBUS__ON_OFF_CONFIG__SP__SHIFT                  0
#define PMBUS__ON_OFF_CONFIG__SP__MASK                   0x01
#define PMBUS__PHASE__PHASE__SHIFT                       0
#define PMBUS__PHASE__PHASE__MASK                        0xFF
#define PMBUS__WRITE_PROTECT__WRITE_PROTECT__SHIFT       0
#define PMBUS__WRITE_PROTECT__WRITE_PROTECT__MASK        0xFF
#define PMBUS__CAPABILITY__PEC__SHIFT                    7
#define PMBUS__CAPABILITY__PEC__MASK                     0x01
#define PMBUS__CAPABILITY__SPD__SHIFT                    5
#define PMBUS__CAPABILITY__SPD__MASK                     0x03
#define PMBUS__CAPABILITY__PMBALRT__SHIFT                4
#define PMBUS__CAPABILITY__PMBALRT__MASK                 0x01
#define PMBUS__VOUT_MODE__MODE__SHIFT                    5
#define PMBUS__VOUT_MODE__MODE__MASK                     0x07
#define PMBUS__VOUT_MODE__VID_TYPE__SHIFT                0
#define PMBUS__VOUT_MODE__VID_TYPE__MASK                 0x1F
#define PMBUS__VOUT_COMMAND__VOUT_CMD_VID__SHIFT         0
#define PMBUS__VOUT_COMMAND__VOUT_CMD_VID__MASK          0xFF
#define PMBUS__VOUT_MAX__VOUT_MAX_VID__SHIFT             0
#define PMBUS__VOUT_MAX__VOUT_MAX_VID__MASK              0xFF
#define PMBUS__VOUT_MARGIN_HIGH__VOUT_MARGH_VID__SHIFT   0
#define PMBUS__VOUT_MARGIN_HIGH__VOUT_MARGH_VID__MASK    0xFF
#define PMBUS__VOUT_MARGIN_LOW__VOUT_MARGL_VID__SHIFT    0
#define PMBUS__VOUT_MARGIN_LOW__VOUT_MARGL_VID__MASK     0xFF
#define PMBUS__VOUT_TRANSITION_RATE__VOTR_EXP__SHIFT     11
#define PMBUS__VOUT_TRANSITION_RATE__VOTR_EXP__MASK      0x1F
#define PMBUS__VOUT_TRANSITION_RATE__VOTR_MAN__SHIFT     0
#define PMBUS__VOUT_TRANSITION_RATE__VOTR_MAN__MASK      0x7FF
#define PMBUS__VOUT_DROOP__VDROOP_EXP__SHIFT             11
#define PMBUS__VOUT_DROOP__VDROOP_EXP__MASK              0x1F
#define PMBUS__VOUT_DROOP__VDROOP_MAN__SHIFT             0
#define PMBUS__VOUT_DROOP__VDROOP_MAN__MASK              0x7FF
#define PMBUS__VOUT_SCALE_LOOP__VOSL_EXP__SHIFT          11
#define PMBUS__VOUT_SCALE_LOOP__VOSL_EXP__MASK           0x1F
#define PMBUS__VOUT_SCALE_LOOP__VOSL_MAN__SHIFT          0
#define PMBUS__VOUT_SCALE_LOOP__VOSL_MAN__MASK           0x7FF
#define PMBUS__VOUT_SCALE_MONITOR__VOSL_EXP__SHIFT       11
#define PMBUS__VOUT_SCALE_MONITOR__VOSL_EXP__MASK        0x1F
#define PMBUS__VOUT_SCALE_MONITOR__VOSL_MAN__SHIFT       0
#define PMBUS__VOUT_SCALE_MONITOR__VOSL_MAN__MASK        0x7FF
#define PMBUS__VOUT_MIN__VOUT_MIN_VID__SHIFT             0
#define PMBUS__VOUT_MIN__VOUT_MIN_VID__MASK              0xFF
#define PMBUS__FREQUENCY_SWITCH__FSW_EXP__SHIFT          11
#define PMBUS__FREQUENCY_SWITCH__FSW_EXP__MASK           0x1F
#define PMBUS__FREQUENCY_SWITCH__FSW_MAN__SHIFT          0
#define PMBUS__FREQUENCY_SWITCH__FSW_MAN__MASK           0x7FF
#define PMBUS__VIN_ON__VINON_EXP__SHIFT                  11
#define PMBUS__VIN_ON__VINON_EXP__MASK                   0x1F
#define PMBUS__VIN_ON__VINON_MAN__SHIFT                  0
#define PMBUS__VIN_ON__VINON_MAN__MASK                   0x7FF
#define PMBUS__IOUT_CAL_GAIN__IOCG_EXP__SHIFT            11
#define PMBUS__IOUT_CAL_GAIN__IOCG_EXP__MASK             0x1F
#define PMBUS__IOUT_CAL_GAIN__IOCG_MAN__SHIFT            0
#define PMBUS__IOUT_CAL_GAIN__IOCG_MAN__MASK             0x7FF
#define PMBUS__IOUT_CAL_OFFSET__IOCOS_EXP__SHIFT         11
#define PMBUS__IOUT_CAL_OFFSET__IOCOS_EXP__MASK          0x1F
#define PMBUS__IOUT_CAL_OFFSET__IOCOS_MAN__SHIFT         0
#define PMBUS__IOUT_CAL_OFFSET__IOCOS_MAN__MASK          0x7FF
#define PMBUS__VOUT_OV_FAULT_LIMIT__VO_OVF_VID__SHIFT    0
#define PMBUS__VOUT_OV_FAULT_LIMIT__VO_OVF_VID__MASK     0xFF
#define PMBUS__VOUT_OV_FAULT_RESPONSE__VO_OV_RESP__SHIFT 0
#define PMBUS__VOUT_OV_FAULT_RESPONSE__VO_OV_RESP__MASK  0xFF
#define PMBUS__VOUT_UV_FAULT_LIMIT__VO_UVF_VID__SHIFT    0
#define PMBUS__VOUT_UV_FAULT_LIMIT__VO_UVF_VID__MASK     0xFF
#define PMBUS__VOUT_UV_FAULT_RESPONSE__VO_UV_RESP__SHIFT 0
#define PMBUS__VOUT_UV_FAULT_RESPONSE__VO_UV_RESP__MASK  0xFF
#define PMBUS__IOUT_OC_FAULT_LIMIT__IOOCF_EXP__SHIFT     11
#define PMBUS__IOUT_OC_FAULT_LIMIT__IOOCF_EXP__MASK      0x1F
#define PMBUS__IOUT_OC_FAULT_LIMIT__IOOCF_MAN__SHIFT     0
#define PMBUS__IOUT_OC_FAULT_LIMIT__IOOCF_MAN__MASK      0x7FF
#define PMBUS__IOUT_OC_FAULT_RESPONSE__IO_OC_RESP__SHIFT 0
#define PMBUS__IOUT_OC_FAULT_RESPONSE__IO_OC_RESP__MASK  0xFF
#define PMBUS__IOUT_OC_WARN_LIMIT__IOOCW_EXP__SHIFT      11
#define PMBUS__IOUT_OC_WARN_LIMIT__IOOCW_EXP__MASK       0x1F
#define PMBUS__IOUT_OC_WARN_LIMIT__IOOCW_MAN__SHIFT      0
#define PMBUS__IOUT_OC_WARN_LIMIT__IOOCW_MAN__MASK       0x7FF
#define PMBUS__OT_FAULT_LIMIT__OTF_EXP__SHIFT            11
#define PMBUS__OT_FAULT_LIMIT__OTF_EXP__MASK             0x1F
#define PMBUS__OT_FAULT_LIMIT__OTF_MAN__SHIFT            0
#define PMBUS__OT_FAULT_LIMIT__OTF_MAN__MASK             0x7FF
#define PMBUS__OT_FAULT_RESPONSE__OTF_RESP__SHIFT        0
#define PMBUS__OT_FAULT_RESPONSE__OTF_RESP__MASK         0xFF
#define PMBUS__OT_WARN_LIMIT__OTF_EXP__SHIFT             11
#define PMBUS__OT_WARN_LIMIT__OTF_EXP__MASK              0x1F
#define PMBUS__OT_WARN_LIMIT__OTF_MAN__SHIFT             0
#define PMBUS__OT_WARN_LIMIT__OTF_MAN__MASK              0x7FF
#define PMBUS__VIN_OV_FAULT_LIMIT__VIN_OVF_EXP__SHIFT    11
#define PMBUS__VIN_OV_FAULT_LIMIT__VIN_OVF_EXP__MASK     0x1F
#define PMBUS__VIN_OV_FAULT_LIMIT__VIN_OVF_MAN__SHIFT    0
#define PMBUS__VIN_OV_FAULT_LIMIT__VIN_OVF_MAN__MASK     0x7FF
#define PMBUS__VIN_OV_FAULT_RESPONSE__VI_OVF_RESP__SHIFT 0
#define PMBUS__VIN_OV_FAULT_RESPONSE__VI_OVF_RESP__MASK  0xFF
#define PMBUS__VIN_UV_FAULT_LIMIT__VIN_UVF_EXP__SHIFT    11
#define PMBUS__VIN_UV_FAULT_LIMIT__VIN_UVF_EXP__MASK     0x1F
#define PMBUS__VIN_UV_FAULT_LIMIT__VIN_UVF_MAN__SHIFT    0
#define PMBUS__VIN_UV_FAULT_LIMIT__VIN_UVF_MAN__MASK     0x7FF
#define PMBUS__VIN_UV_FAULT_RESPONSE__VI_UVF_RESP__SHIFT 0
#define PMBUS__VIN_UV_FAULT_RESPONSE__VI_UVF_RESP__MASK  0xFF
#define PMBUS__IIN_OC_FAULT_LIMIT__IIN_OCF_EXP__SHIFT    11
#define PMBUS__IIN_OC_FAULT_LIMIT__IIN_OCF_EXP__MASK     0x1F
#define PMBUS__IIN_OC_FAULT_LIMIT__IIN_OCF_MAN__SHIFT    0
#define PMBUS__IIN_OC_FAULT_LIMIT__IIN_OCF_MAN__MASK     0x7FF
#define PMBUS__IIN_OC_FAULT_RESPONSE__IIN_OC_RESP__SHIFT 0
#define PMBUS__IIN_OC_FAULT_RESPONSE__IIN_OC_RESP__MASK  0xFF
#define PMBUS__IIN_OC_WARN_LIMIT__IIN_OCW_EXP__SHIFT     11
#define PMBUS__IIN_OC_WARN_LIMIT__IIN_OCW_EXP__MASK      0x1F
#define PMBUS__IIN_OC_WARN_LIMIT__IIN_OCW_MAN__SHIFT     0
#define PMBUS__IIN_OC_WARN_LIMIT__IIN_OCW_MAN__MASK      0x7FF
#define PMBUS__TON_DELAY__TON_DLY_EXP__SHIFT             11
#define PMBUS__TON_DELAY__TON_DLY_EXP__MASK              0x1F
#define PMBUS__TON_DELAY__TON_DLY_MAN__SHIFT             0
#define PMBUS__TON_DELAY__TON_DLY_MAN__MASK              0x7FF
#define PMBUS__PIN_OP_WARN_LIMIT__PIN_OPW_EXP__SHIFT     11
#define PMBUS__PIN_OP_WARN_LIMIT__PIN_OPW_EXP__MASK      0x1F
#define PMBUS__PIN_OP_WARN_LIMIT__PIN_OPW_MAN__SHIFT     0
#define PMBUS__PIN_OP_WARN_LIMIT__PIN_OPW_MAN__MASK      0x7FF
#define PMBUS__STATUS_BYTE__BUSY__SHIFT                  7
#define PMBUS__STATUS_BYTE__BUSY__MASK                   0x01
#define PMBUS__STATUS_BYTE__OFF__SHIFT                   6
#define PMBUS__STATUS_BYTE__OFF__MASK                    0x01
#define PMBUS__STATUS_BYTE__VOUT_OV__SHIFT               5
#define PMBUS__STATUS_BYTE__VOUT_OV__MASK                0x01
#define PMBUS__STATUS_BYTE__IOUT_OC__SHIFT               4
#define PMBUS__STATUS_BYTE__IOUT_OC__MASK                0x01
#define PMBUS__STATUS_BYTE__VIN_UV__SHIFT                3
#define PMBUS__STATUS_BYTE__VIN_UV__MASK                 0x01
#define PMBUS__STATUS_BYTE__TEMP__SHIFT                  2
#define PMBUS__STATUS_BYTE__TEMP__MASK                   0x01
#define PMBUS__STATUS_BYTE__CML__SHIFT                   1
#define PMBUS__STATUS_BYTE__CML__MASK                    0x01
#define PMBUS__STATUS_BYTE__OTHER__SHIFT                 0
#define PMBUS__STATUS_BYTE__OTHER__MASK                  0x01
#define PMBUS__STATUS_WORD__VOUT__SHIFT                  15
#define PMBUS__STATUS_WORD__VOUT__MASK                   0x01
#define PMBUS__STATUS_WORD__IOUT__SHIFT                  14
#define PMBUS__STATUS_WORD__IOUT__MASK                   0x01
#define PMBUS__STATUS_WORD__INPUT__SHIFT                 13
#define PMBUS__STATUS_WORD__INPUT__MASK                  0x01
#define PMBUS__STATUS_WORD__PGOOD__SHIFT                 11
#define PMBUS__STATUS_WORD__PGOOD__MASK                  0x01
#define PMBUS__STATUS_WORD__FANS__SHIFT                  10
#define PMBUS__STATUS_WORD__FANS__MASK                   0x01
#define PMBUS__STATUS_WORD__OTHER2__SHIFT                9
#define PMBUS__STATUS_WORD__OTHER2__MASK                 0x01
#define PMBUS__STATUS_WORD__UNKNOWN__SHIFT               8
#define PMBUS__STATUS_WORD__UNKNOWN__MASK                0x01
#define PMBUS__STATUS_WORD__BUSY__SHIFT                  7
#define PMBUS__STATUS_WORD__BUSY__MASK                   0x01
#define PMBUS__STATUS_WORD__OFF__SHIFT                   6
#define PMBUS__STATUS_WORD__OFF__MASK                    0x01
#define PMBUS__STATUS_WORD__VOUT_OV__SHIFT               5
#define PMBUS__STATUS_WORD__VOUT_OV__MASK                0x01
#define PMBUS__STATUS_WORD__IOUT_OC__SHIFT               4
#define PMBUS__STATUS_WORD__IOUT_OC__MASK                0x01
#define PMBUS__STATUS_WORD__VIN_UV__SHIFT                3
#define PMBUS__STATUS_WORD__VIN_UV__MASK                 0x01
#define PMBUS__STATUS_WORD__TEMP__SHIFT                  2
#define PMBUS__STATUS_WORD__TEMP__MASK                   0x01
#define PMBUS__STATUS_WORD__CML__SHIFT                   1
#define PMBUS__STATUS_WORD__CML__MASK                    0x01
#define PMBUS__STATUS_WORD__OTHER__SHIFT                 0
#define PMBUS__STATUS_WORD__OTHER__MASK                  0x01
#define PMBUS__STATUS_VOUT__VOUT_OVF__SHIFT              7
#define PMBUS__STATUS_VOUT__VOUT_OVF__MASK               0x01
#define PMBUS__STATUS_VOUT__VOUT_OVW__SHIFT              6
#define PMBUS__STATUS_VOUT__VOUT_OVW__MASK               0x01
#define PMBUS__STATUS_VOUT__VOUT_UVW__SHIFT              5
#define PMBUS__STATUS_VOUT__VOUT_UVW__MASK               0x01
#define PMBUS__STATUS_VOUT__VOUT_UVF__SHIFT              4
#define PMBUS__STATUS_VOUT__VOUT_UVF__MASK               0x01
#define PMBUS__STATUS_VOUT__VOUT_MIN_MAX__SHIFT          3
#define PMBUS__STATUS_VOUT__VOUT_MIN_MAX__MASK           0x01
#define PMBUS__STATUS_VOUT__TON_MAX__SHIFT               2
#define PMBUS__STATUS_VOUT__TON_MAX__MASK                0x01
#define PMBUS__STATUS_VOUT__TOFF_MAX__SHIFT              1
#define PMBUS__STATUS_VOUT__TOFF_MAX__MASK               0x01
#define PMBUS__STATUS_VOUT__VOUT_TRACK__SHIFT            0
#define PMBUS__STATUS_VOUT__VOUT_TRACK__MASK             0x01
#define PMBUS__STATUS_IOUT__IOUT_OCF__SHIFT              7
#define PMBUS__STATUS_IOUT__IOUT_OCF__MASK               0x01
#define PMBUS__STATUS_IOUT__IOUT_OCUVF__SHIFT            6
#define PMBUS__STATUS_IOUT__IOUT_OCUVF__MASK             0x01
#define PMBUS__STATUS_IOUT__IOUT_OCW__SHIFT              5
#define PMBUS__STATUS_IOUT__IOUT_OCW__MASK               0x01
#define PMBUS__STATUS_IOUT__IOUT_UCF__SHIFT              4
#define PMBUS__STATUS_IOUT__IOUT_UCF__MASK               0x01
#define PMBUS__STATUS_IOUT__CUR_SHAREF__SHIFT            3
#define PMBUS__STATUS_IOUT__CUR_SHAREF__MASK             0x01
#define PMBUS__STATUS_IOUT__POW_LIMIT__SHIFT             2
#define PMBUS__STATUS_IOUT__POW_LIMIT__MASK              0x01
#define PMBUS__STATUS_IOUT__POUT_OPF__SHIFT              1
#define PMBUS__STATUS_IOUT__POUT_OPF__MASK               0x01
#define PMBUS__STATUS_IOUT__POUT_OPW__SHIFT              0
#define PMBUS__STATUS_IOUT__POUT_OPW__MASK               0x01
#define PMBUS__STATUS_INPUT__VIN_OVF__SHIFT              7
#define PMBUS__STATUS_INPUT__VIN_OVF__MASK               0x01
#define PMBUS__STATUS_INPUT__VIN_OVW__SHIFT              6
#define PMBUS__STATUS_INPUT__VIN_OVW__MASK               0x01
#define PMBUS__STATUS_INPUT__VIN_UVW__SHIFT              5
#define PMBUS__STATUS_INPUT__VIN_UVW__MASK               0x01
#define PMBUS__STATUS_INPUT__VIN_UVF__SHIFT              4
#define PMBUS__STATUS_INPUT__VIN_UVF__MASK               0x01
#define PMBUS__STATUS_INPUT__LOW_VIN__SHIFT              3
#define PMBUS__STATUS_INPUT__LOW_VIN__MASK               0x01
#define PMBUS__STATUS_INPUT__IIN_OCF__SHIFT              2
#define PMBUS__STATUS_INPUT__IIN_OCF__MASK               0x01
#define PMBUS__STATUS_INPUT__IIN_OCW__SHIFT              1
#define PMBUS__STATUS_INPUT__IIN_OCW__MASK               0x01
#define PMBUS__STATUS_INPUT__PIN_OPW__SHIFT              0
#define PMBUS__STATUS_INPUT__PIN_OPW__MASK               0x01
#define PMBUS__STATUS_TEMPERATURE__OTF__SHIFT            7
#define PMBUS__STATUS_TEMPERATURE__OTF__MASK             0x01
#define PMBUS__STATUS_TEMPERATURE__OTW__SHIFT            6
#define PMBUS__STATUS_TEMPERATURE__OTW__MASK             0x01
#define PMBUS__STATUS_TEMPERATURE__UTW__SHIFT            5
#define PMBUS__STATUS_TEMPERATURE__UTW__MASK             0x01
#define PMBUS__STATUS_TEMPERATURE__UTF__SHIFT            4
#define PMBUS__STATUS_TEMPERATURE__UTF__MASK             0x01
#define PMBUS__STATUS_CML__IV_CMD__SHIFT                 7
#define PMBUS__STATUS_CML__IV_CMD__MASK                  0x01
#define PMBUS__STATUS_CML__IV_DATA__SHIFT                6
#define PMBUS__STATUS_CML__IV_DATA__MASK                 0x01
#define PMBUS__STATUS_CML__PEC_FAIL__SHIFT               5
#define PMBUS__STATUS_CML__PEC_FAIL__MASK                0x01
#define PMBUS__STATUS_CML__MEM__SHIFT                    3
#define PMBUS__STATUS_CML__MEM__MASK                     0x01
#define PMBUS__STATUS_CML__COM_FAIL__SHIFT               1
#define PMBUS__STATUS_CML__COM_FAIL__MASK                0x01
#define PMBUS__STATUS_CML__CML_OTHER__SHIFT              0
#define PMBUS__STATUS_CML__CML_OTHER__MASK               0x01
#define PMBUS__STATUS_MFR_SPECIFIC__MFR_FAULT_PS__SHIFT  7
#define PMBUS__STATUS_MFR_SPECIFIC__MFR_FAULT_PS__MASK   0x01
#define PMBUS__STATUS_MFR_SPECIFIC__VSNS_OPEN__SHIFT     6
#define PMBUS__STATUS_MFR_SPECIFIC__VSNS_OPEN__MASK      0x01
#define PMBUS__STATUS_MFR_SPECIFIC__MAX_PH_WARN__SHIFT   5
#define PMBUS__STATUS_MFR_SPECIFIC__MAX_PH_WARN__MASK    0x01
#define PMBUS__STATUS_MFR_SPECIFIC__TSNS_LOW__SHIFT      4
#define PMBUS__STATUS_MFR_SPECIFIC__TSNS_LOW__MASK       0x01
#define PMBUS__STATUS_MFR_SPECIFIC__PHFLT__SHIFT         0
#define PMBUS__STATUS_MFR_SPECIFIC__PHFLT__MASK          0x01
#define PMBUS__READ_VIN__READ_VIN_EXP__SHIFT             11
#define PMBUS__READ_VIN__READ_VIN_EXP__MASK              0x1F
#define PMBUS__READ_VIN__READ_VIN_MAN__SHIFT             0
#define PMBUS__READ_VIN__READ_VIN_MAN__MASK              0x7FF
#define PMBUS__READ_IIN__READ_IIN_EXP__SHIFT             11
#define PMBUS__READ_IIN__READ_IIN_EXP__MASK              0x1F
#define PMBUS__READ_IIN__READ_IIN_MAN__SHIFT             0
#define PMBUS__READ_IIN__READ_IIN_MAN__MASK              0x7FF
#define PMBUS__READ_VOUT__READ_VOUT_VID__SHIFT           0
#define PMBUS__READ_VOUT__READ_VOUT_VID__MASK            0xFF
#define PMBUS__READ_IOUT__READ_IOUT_EXP__SHIFT           11
#define PMBUS__READ_IOUT__READ_IOUT_EXP__MASK            0x1F
#define PMBUS__READ_IOUT__READ_IOUT_MAN__SHIFT           0
#define PMBUS__READ_IOUT__READ_IOUT_MAN__MASK            0x7FF
#define PMBUS__READ_TEMPERATURE_1__READ_TEMP_EXP__SHIFT  11
#define PMBUS__READ_TEMPERATURE_1__READ_TEMP_EXP__MASK   0x1F
#define PMBUS__READ_TEMPERATURE_1__READ_TEMP_MAN__SHIFT  0
#define PMBUS__READ_TEMPERATURE_1__READ_TEMP_MAN__MASK   0x7FF
#define PMBUS__READ_POUT__READ_POUT_EXP__SHIFT           11
#define PMBUS__READ_POUT__READ_POUT_EXP__MASK            0x1F
#define PMBUS__READ_POUT__READ_POUT_MAN__SHIFT           0
#define PMBUS__READ_POUT__READ_POUT_MAN__MASK            0x7FF
#define PMBUS__READ_PIN__READ_PIN_EXP__SHIFT             11
#define PMBUS__READ_PIN__READ_PIN_EXP__MASK              0x1F
#define PMBUS__READ_PIN__READ_PIN_MAN__SHIFT             0
#define PMBUS__READ_PIN__READ_PIN_MAN__MASK              0x7FF
#define PMBUS__PMBUS_REVISION__PMB_REV_P1__SHIFT         4
#define PMBUS__PMBUS_REVISION__PMB_REV_P1__MASK          0x0F
#define PMBUS__PMBUS_REVISION__PMB_REV_P2__SHIFT         0
#define PMBUS__PMBUS_REVISION__PMB_REV_P2__MASK          0x0F
#define PMBUS__IC_DEVICE_ID__IC_DEVICE_ID__SHIFT         0
#define PMBUS__IC_DEVICE_ID__IC_DEVICE_ID__MASK          0xFFFF
#define PMBUS__IC_DEVICE_REV__IC_DEVICE_REV__SHIFT       0
#define PMBUS__IC_DEVICE_REV__IC_DEVICE_REV__MASK        0xFFFF
#define PMBUS__USER_DATA_00__USER_DATA_00__SHIFT         0
#define PMBUS__USER_DATA_00__USER_DATA_00__MASK          0xFFFFFFFFFFFF
#define PMBUS__USER_DATA_01__USER_DATA_01__SHIFT         0
#define PMBUS__USER_DATA_01__USER_DATA_01__MASK          0xFFFFFFFFFFFF
#define PMBUS__USER_DATA_02__USER_DATA_02__SHIFT         0
#define PMBUS__USER_DATA_02__USER_DATA_02__MASK          0xFFFFFFFFFFFF
#define PMBUS__USER_DATA_03__USER_DATA_03__SHIFT         0
#define PMBUS__USER_DATA_03__USER_DATA_03__MASK          0xFFFFFFFFFFFF
#define PMBUS__USER_DATA_04__USER_DATA_04__SHIFT         0
#define PMBUS__USER_DATA_04__USER_DATA_04__MASK          0xFFFFFFFFFFFF
#define PMBUS__USER_DATA_05__USER_DATA_05__SHIFT         0
#define PMBUS__USER_DATA_05__USER_DATA_05__MASK          0xFFFFFFFFFFFF
#define PMBUS__USER_DATA_06__USER_DATA_06__SHIFT         0
#define PMBUS__USER_DATA_06__USER_DATA_06__MASK          0xFFFFFFFFFFFF
#define PMBUS__USER_DATA_07__USER_DATA_07__SHIFT         0
#define PMBUS__USER_DATA_07__USER_DATA_07__MASK          0xFFFFFFFFFFFF
#define PMBUS__USER_DATA_08__USER_DATA_08__SHIFT         0
#define PMBUS__USER_DATA_08__USER_DATA_08__MASK          0xFFFFFFFFFFFF
#define PMBUS__USER_DATA_09__USER_DATA_09__SHIFT         0
#define PMBUS__USER_DATA_09__USER_DATA_09__MASK          0xFFFFFFFFFFFF
#define PMBUS__USER_DATA_10__USER_DATA_10__SHIFT         0
#define PMBUS__USER_DATA_10__USER_DATA_10__MASK          0xFFFFFFFFFFFF
#define PMBUS__USER_DATA_12__USER_DATA_12__SHIFT         0
#define PMBUS__USER_DATA_12__USER_DATA_12__MASK          0xFFFFFFFFFFFF

/* Table data */

extern t_structRegInfo const PMBus_RegInfo[];
extern t_structFieldInfo const PMBus_FieldInfo[];

#define PMBus_MAXREG (sizeof(PMBus_RegInfo) / sizeof(t_structRegInfo))

#ifdef INSTANTIATE_DEVINFO
t_structRegInfo const PMBus_RegInfo[] = {
    { "PAGE", 0, 0, 0x00, 0x00, eRT_RW8, 1 },                   /*0*/
    { "OPERATION", 0, 0, 0x01, 0x01, eRT_RW8, 1 },              /*1*/
    { "ON_OFF_CONFIG", 0, 0, 0x02, 0x02, eRT_RW8, 1 },          /*2*/
    { "CLEAR_FAULTS", 0, 0, 0x03, 0x03, eRT_W0, 0 },            /*3*/
    { "PHASE", 9, 1, 0x04, 0x04, eRT_RW8, 1 },                  /*4*/
    { "PAGE_PLUS_WRITE", 0, 0, 0x05, 0x05, eRT_WBLK, 6 },       /*4*/
    { "PAGE_PLUS_READ", 0, 0, 0x06, 0x06, eRT_RWBLK, 3 },       /*5*/
    { "WRITE_PROTECT", 0, 0, 0x10, 0x10, eRT_RW8, 1 },          /*6*/
    { "STORE_DEFAULT_ALL", 11, 1, 0x11, 0x11, eRT_W0, 0 },      /*6*/
    { "RESTORE_DEFAULT_ALL", 12, 1, 0x12, 0x12, eRT_W0, 0 },    /*7*/
    { "STORE_USER_ALL", 0, 0, 0x15, 0x15, eRT_W0, 0 },          /*7*/
    { "RESTORE_USER_ALL", 0, 0, 0x16, 0x16, eRT_W0, 0 },        /*8*/
    { "CAPABILITY", 0, 0, 0x19, 0x19, eRT_R8, 1 },              /*9*/
    { "SMBALERT_MASK", 0, 0, 0x1B, 0x1B, eRT_RWBLK, 2 },        /*10*/
    { "VOUT_MODE", 0, 0, 0x20, 0x20, eRT_R8, 1 },               /*11*/
    { "VOUT_COMMAND", 0, 0, 0x21, 0x21, eRT_RW16, 2 },          /*12*/
    { "VOUT_MAX", 0, 0, 0x24, 0x24, eRT_RW16, 2 },              /*13*/
    { "VOUT_MARGIN_HIGH", 0, 0, 0x25, 0x25, eRT_RW16, 2 },      /*14*/
    { "VOUT_MARGIN_LOW", 0, 0, 0x26, 0x26, eRT_RW16, 2 },       /*15*/
    { "VOUT_TRANSITION_RATE", 0, 0, 0X27, 0X27, eRT_RW16, 2 },  /*16*/
    { "VOUT_DROOP", 25, 2, 0x28, 0x28, eRT_RW16, 2 },           /*16*/
    { "VOUT_SCALE_LOOP", 27, 2, 0x29, 0x29, eRT_RW16, 2 },      /*17*/
    { "VOUT_SCALE_MONITOR", 29, 2, 0x2A, 0x2A, eRT_RW16, 2 },   /*18*/
    { "VOUT_MIN", 31, 1, 0x2B, 0x2B, eRT_RW16, 2 },             /*19*/
    { "FREQUENCY_SWITCH", 0, 0, 0x33, 0x33, eRT_RW16, 2 },      /*17*/
    { "VIN_ON", 0, 0, 0x35, 0x35, eRT_RW16, 2 },                /*18*/
    { "VIN_OFF", 0, 0, 0x36, 0x36, eRT_RW16, 2 },               /*19*/
    { "IOUT_CAL_GAIN", 36, 2, 0x38, 0x38, eRT_RW16, 2 },        /*22*/
    { "IOUT_CAL_OFFSET", 38, 2, 0x39, 0x39, eRT_RW16, 2 },      /*23*/
    { "VOUT_OV_FAULT_LIMIT", 0, 0, 0x40, 0x40, eRT_RW16, 2 },   /*20*/
    { "VOUT_OV_FAULT_RESPONSE", 0, 0, 0x41, 0x41, eRT_RW8, 1 }, /*21*/
    { "VOUT_OV_WARN_LIMIT", 0, 0, 0x42, 0x42, eRT_RW16, 2 },    /*22*/
    { "VOUT_UV_WARN_LIMIT", 0, 0, 0x43, 0x43, eRT_RW16, 2 },    /*23*/
    { "VOUT_UV_FAULT_LIMIT", 0, 0, 0x44, 0x44, eRT_RW16, 2 },   /*24*/
    { "VOUT_UV_FAULT_RESPONSE", 0, 0, 0x45, 0x45, eRT_RW8, 1 }, /*25*/
    { "IOUT_OC_FAULT_LIMIT", 0, 0, 0x46, 0x46, eRT_RW16, 2 },   /*26*/
    { "IOUT_OC_FAULT_RESPONSE", 0, 0, 0x47, 0x47, eRT_RW8, 1 }, /*27*/
    { "IOUT_OC_WARN_LIMIT", 0, 0, 0x4A, 0x4A, eRT_RW16, 2 },    /*28*/
    { "OT_FAULT_LIMIT", 0, 0, 0x4F, 0x4F, eRT_RW16, 2 },        /*29*/
    { "OT_FAULT_RESPONSE", 0, 0, 0x50, 0x50, eRT_RW8, 1 },      /*30*/
    { "OT_WARN_LIMIT", 0, 0, 0x51, 0x51, eRT_RW16, 2 },         /*31*/
    { "UT_FAULT_LIMIT", 0, 0, 0x53, 0x53, eRT_RW16, 2 },        /*32*/
    { "UT_FAULT_RESPONSE", 0, 0, 0x54, 0x54, eRT_RW8, 1 },      /*33*/
    { "VIN_OV_FAULT_LIMIT", 0, 0, 0x55, 0x55, eRT_RW16, 2 },    /*34*/
    { "VIN_OV_FAULT_RESPONSE", 0, 0, 0x56, 0x56, eRT_RW8, 1 },  /*35*/
    { "VIN_UV_WARN_LIMIT", 0, 0, 0x58, 0x58, eRT_RW16, 2 },     /*36*/
    { "VIN_UV_FAULT_LIMIT", 57, 2, 0x59, 0x59, eRT_RW16, 2 },   /*36*/
    { "VIN_UV_FAULT_RESPONSE", 59, 1, 0x5A, 0x5A, eRT_R8, 1 },  /*37*/
    { "IIN_OC_FAULT_LIMIT", 60, 2, 0x5B, 0x5B, eRT_RW16, 2 },   /*38*/
    { "IIN_OC_FAULT_RESPONSE", 62, 1, 0x5C, 0x5C, eRT_R8, 1 },  /*39*/
    { "IIN_OC_WARN_LIMIT", 0, 0, 0x5D, 0x5D, eRT_RW16, 2 },     /*37*/
    { "TON_DELAY", 0, 0, 0x60, 0x60, eRT_RW16, 2 },             /*38*/
    { "TON_RISE", 0, 0, 0x61, 0x61, eRT_RW16, 2 },              /*39*/
    { "TON_MAX_FAULT_LIMIT", 0, 0, 0x62, 0x62, eRT_RW16, 2 },   /*40*/
    { "TON_MAX_FAULT_RESPONSE", 0, 0, 0x63, 0x63, eRT_RW8, 1 }, /*41*/
    { "TOFF_DELAY", 0, 0, 0x64, 0x64, eRT_RW16, 2 },            /*42*/
    { "TOFF_FALL", 0, 0, 0x65, 0x65, eRT_RW16, 2 },             /*43*/
    { "TOFF_MAX_WARN_LIMIT", 0, 0, 0x66, 0x66, eRT_RW16, 2 },   /*44*/
    { "PIN_OP_WARN_LIMIT", 67, 2, 0x6B, 0x6B, eRT_RW16, 2 },    /*42*/
    { "STATUS_BYTE", 0, 0, 0x78, 0x78, eRT_RW8, 1 },            /*45*/
    { "STATUS_WORD", 0, 0, 0x79, 0x79, eRT_RW16, 2 },           /*46*/
    { "STATUS_VOUT", 0, 0, 0x7A, 0x7A, eRT_RW8, 1 },            /*47*/
    { "STATUS_IOUT", 0, 0, 0x7B, 0x7B, eRT_RW8, 1 },            /*48*/
    { "STATUS_INPUT", 0, 0, 0x7C, 0x7C, eRT_RW8, 1 },           /*49*/
    { "STATUS_TEMPERATURE", 0, 0, 0x7D, 0x7D, eRT_RW8, 1 },     /*50*/
    { "STATUS_CML", 0, 0, 0x7E, 0x7E, eRT_RW8, 1 },             /*51*/
    { "STATUS_MFR_SPECIFIC", 0, 0, 0x80, 0x80, eRT_RW8, 1 },    /*52*/
    { "READ_VIN", 0, 0, 0x88, 0x88, eRT_R16, 2 },               /*53*/
    { "READ_IIN", 0, 0, 0x89, 0x89, eRT_R16, 2 },               /*54*/
    { "READ_VOUT", 0, 0, 0x8B, 0x8B, eRT_R16, 2 },              /*55*/
    { "READ_IOUT", 0, 0, 0x8C, 0x8C, eRT_R16, 2 },              /*56*/
    { "READ_TEMPERATURE_1", 0, 0, 0x8D, 0x8D, eRT_R16, 2 },     /*57*/
    { "READ_TEMPERATURE_2", 0, 0, 0x8E, 0x8E, eRT_R16, 2 },     /*58*/
    { "READ_POUT", 0, 0, 0x96, 0x96, eRT_R16, 2 },              /*60*/
    { "READ_PIN", 0, 0, 0x97, 0x97, eRT_R16, 2 },               /*61*/
    { "PMBus_REVISION", 0, 0, 0x98, 0x98, eRT_R8, 1 },          /*62*/
    { "MFR_ID", 0, 0, 0x99, 0x99, eRT_RBLK, 4 },                /*63*/
    { "MFR_MODEL", 0, 0, 0x9A, 0x9A, eRT_RBLK, 8 },             /*64*/
    { "MFR_REVISION", 149, 1, 0x9B, 0x9B, eRT_RWBLK, 2 },       /*61*/
    { "MFR_DATE", 150, 1, 0x9D, 0x9D, eRT_RWBLK, 2 },           /*62*/
    { "MFR_SERIAL", 151, 1, 0x9E, 0x9E, eRT_RBLK, 4 },          /*63*/
    { "MFR_VOUT_MAX", 0, 0, 0xA5, 0xA5, eRT_R16, 2 },           /*65*/
    { "MFR_PIN_ACCURACY", 0, 0, 0xAC, 0xAC, eRT_R8, 1 },        /*66*/
    { "IC_DEVICE_ID", 152, 1, 0xAD, 0xAD, eRT_RBLK, 2 },        /*64*/
    { "IC_DEVICE_REV", 153, 1, 0xAE, 0xAE, eRT_RBLK, 2 },       /*65*/
    { "USER_DATA_00", 0, 0, 0xB0, 0xB0, eRT_RW16, 2 },          /*67*/
    { "USER_DATA_01", 0, 0, 0xB1, 0xB1, eRT_RW16, 2 },          /*68*/
    { "USER_DATA_02", 0, 0, 0xB2, 0xB2, eRT_RW16, 2 },          /*69*/
    { "USER_DATA_03", 0, 0, 0xB3, 0xB3, eRT_RW16, 2 },          /*70*/
    { "USER_DATA_04", 0, 0, 0xB4, 0xB4, eRT_RW16, 2 },          /*71*/
    { "USER_DATA_05", 159, 1, 0xB5, 0xB5, eRT_RWBLK, 6 },       /*71*/
    { "USER_DATA_06", 160, 1, 0xB6, 0xB6, eRT_RWBLK, 6 },       /*72*/
    { "USER_DATA_07", 161, 1, 0xB7, 0xB7, eRT_RWBLK, 6 },       /*73*/
    { "USER_DATA_08", 162, 1, 0xB8, 0xB8, eRT_RWBLK, 6 },       /*74*/
    { "USER_DATA_09", 163, 1, 0xB9, 0xB9, eRT_RWBLK, 6 },       /*75*/
    { "USER_DATA_10", 164, 1, 0xBA, 0xBA, eRT_RWBLK, 6 },       /*76*/
    { "USER_DATA_11", 165, 1, 0xBB, 0xBB, eRT_RWBLK, 6 },       /*77*/
    { "USER_DATA_12", 166, 1, 0xBC, 0xBC, eRT_RWBLK, 6 },       /*78*/
    { "MFR_CHAN_CONFIG", 0, 0, 0xD0, 0xD0, eRT_RW8, 1 },        /*72*/
    { "MFR_CONFIG_ALL", 0, 0, 0xD1, 0xD1, eRT_RW8, 1 },         /*73*/
    { "MFR_FAULT_PROPAGATE", 0, 0, 0xD2, 0xD2, eRT_RW16, 2 },   /*74*/
    { "MFR_PWM_COMP", 0, 0, 0xD3, 0xD3, eRT_RW8, 1 },           /*75*/
    { "MFR_PWM_MODE", 0, 0, 0xD4, 0xD4, eRT_RW8, 1 },           /*76*/
    { "MFR_FAULT_RESPONSE", 0, 0, 0xD5, 0xD5, eRT_RW8, 1 },     /*77*/
    { "MFR_OT_FAULT_RESPONSE", 0, 0, 0xD6, 0xD6, eRT_R8, 1 },   /*78*/
    { "MFR_IOUT_PEAK", 0, 0, 0xD7, 0xD7, eRT_R16, 2 },          /*79*/
    { "MFR_ADC_CONTROL", 0, 0, 0xD8, 0xD8, eRT_RW8, 1 },        /*80*/
    { "MFR_SPECIFIC_09", 194, 5, 0xD9, 0xD9, eRT_W16, 2 },      /*86*/
    { "MFR_IOUT_CAL_GAIN", 0, 0, 0xDA, 0xDA, eRT_R16, 2 },      /*81*/
    { "MFR_RETRY_DELAY", 0, 0, 0xDB, 0xDB, eRT_RW16, 2 },       /*82*/
    { "MFR_RESTART_DELAY", 0, 0, 0xDC, 0xDC, eRT_RW16, 2 },     /*83*/
    { "MFR_VOUT_PEAK", 0, 0, 0xDD, 0xDD, eRT_R16, 2 },          /*84*/
    { "MFR_VIN_PEAK", 0, 0, 0xDE, 0xDE, eRT_R16, 2 },           /*85*/
    { "MFR_TEMPERATURE_1_PEAK", 0, 0, 0xDF, 0xDF, eRT_R16, 2 }, /*86*/
    { "MFR_READ_IIN_PEAK", 0, 0, 0xE1, 0xE1, eRT_R16, 2 },      /*87*/
    { "MFR_CLEAR_PEAKS", 0, 0, 0xE3, 0xE3, eRT_W0, 0 },         /*88*/
    { "MFR_READ_ICHIP", 0, 0, 0xE4, 0xE4, eRT_R16, 2 },         /*89*/
    { "MFR_PADS", 0, 0, 0xE5, 0xE5, eRT_R16, 2 },               /*90*/
    { "MFR_ADDRESS", 0, 0, 0xE6, 0xE6, eRT_RW8, 1 },            /*91*/
    { "MFR_SPECIAL_ID", 0, 0, 0xE7, 0xE7, eRT_R16, 2 },         /*92*/
    { "MFR_IIN_CAL_GAIN", 0, 0, 0xE8, 0xE8, eRT_RW16, 2 },      /*93*/
    { "MFR_FAULT_LOG_STORE", 0, 0, 0xEA, 0xEA, eRT_W0, 0 },     /*94*/
    { "MFR_FAULT_LOG_CLEAR", 0, 0, 0xEC, 0xEC, eRT_W0, 0 },     /*95*/
    { "MFR_FAULT_LOG", 0, 0, 0xEE, 0xEE, eRT_RBLK, 148 },       /*96*/
    { "MFR_COMMON", 0, 0, 0xEF, 0xEF, eRT_R8, 1 },              /*97*/
    { "MFR_COMPARE_USER_ALL", 0, 0, 0xF0, 0xF0, eRT_W0, 0 },    /*98*/
    { "MFR_TEMPERATURE_2_PEAK", 0, 0, 0xF4, 0xF4, eRT_R16, 2 }, /*99*/
    { "MFR_PWM_CONFIG", 0, 0, 0xF5, 0xF5, eRT_RW8, 1 },         /*100*/
    { "MFR_IOUT_CAL_GAIN_TC", 0, 0, 0xF6, 0xF6, eRT_RW16, 2 },  /*101*/
    { "MFR_ICHIP_CAL_GAIN", 0, 0, 0xF7, 0xF7, eRT_RW16, 2 },    /*102*/
    { "MFR_TEMP_1_GAIN", 0, 0, 0xF8, 0xF8, eRT_RW16, 2 },       /*103*/
    { "MFR_TEMP_1_OFFSET", 0, 0, 0xF9, 0xF9, eRT_RW16, 2 },     /*104*/
    { "MFR_RAIL_ADDRESS", 0, 0, 0xFA, 0xFA, eRT_RW8, 1 },       /*105*/
    { "MFR_REAL_TIME", 0, 0, 0xFB, 0xFB, eRT_RBLK, 6 },         /*106*/
    { "MFR_RESET", 0, 0, 0xFD, 0xFD, eRT_W0, 0 },               /*107*/
};

/* This structure defines PMBus 1.2 specs register field information.
   Currently used by LTM4680.
*/
t_structFieldInfo const PMBus1_2_FieldInfo[] = {
    { "PAGE", 0, 7 },                    /*0*/
    { "ON_OFF_CONFIG", 8, 15 },          /*1*/
    { "CLEAR_FAULTS", 0, 0 },            /*2*/
    { "PHASE", 9, 9 },                   /*3*/
    { "WRITE_PROTECT", 10, 10 },         /*4*/
    { "STORE_DEFAULT_ALL", 11, 11 },     /*5*/
    { "RESTORE_DEFAULT_ALL", 12, 12 },   /*6*/
    { "STORE_USER_ALL", 13, 13 },        /*7*/
    { "RESTORE_USER_ALL", 14, 14 },      /*8*/
    { "STORE_DEFAULT_ALL", 15, 15 },     /*9*/
    { "RESTORE_DEFAULT_ALL", 0, 0 },     /*10*/
    { "STORE_USER_ALL", 1, 1 },          /*11*/
    { "RESTORE_USER_ALL", 2, 2 },        /*12*/
    { "PAGE_PLUS_WRITE", 0, 7 },         /*13*/
    { "PAGE_PLUS_READ", 8, 15 },         /*14*/
    { "STORE_DEFAULT_CODE", 0, 0 },      /*15*/
    { "RESTORE_DEFAULT_CODE", 1, 1 },    /*16*/
    { "STORE_USER_CODE", 2, 2 },         /*17*/
    { "RESTORE_USER_CODE", 3, 3 },       /*18*/
    { "USER_DATA", 0, 7 },               /*19*/
    { "PAGE_PLUS_WRITE", 0, 0 },         /*20*/
    { "PAGE_PLUS_READ", 1, 1 },          /*21*/
    { "STORE_DEFAULT_ALL", 0, 0 },       /*22*/
    { "RESTORE_DEFAULT_ALL", 1, 1 },     /*23*/
    { "STORE_USER_ALL", 2, 2 },          /*24*/
    { "RESTORE_USER_ALL", 3, 3 },        /*25*/
    { "PAGE_PLUS_WRITE", 4, 7 },         /*26*/
    { "PAGE_PLUS_READ", 8, 11 },         /*27*/
    { "STORE_DEFAULT_CODE", 12, 15 },    /*28*/
    { "RESTORE_DEFAULT_CODE", 0, 3 },    /*29*/
    { "STORE_USER_CODE", 4, 7 },         /*30*/
    { "RESTORE_USER_CODE", 8, 11 },      /*31*/
    { "IOUT_OC_FAULT_LIMIT", 0, 7 },     /*32*/
    { "IOUT_UC_FAULT_LIMIT", 8, 15 },    /*33*/
    { "VIN_OV_FAULT_LIMIT", 0, 7 },      /*34*/
    { "VIN_UV_FAULT_LIMIT", 8, 15 },     /*35*/
    { "OT_FAULT_LIMIT", 0, 7 },          /*36*/
    { "UT_FAULT_LIMIT", 8, 15 },         /*37*/
    { "IIN_OC_FAULT_LIMIT", 0, 7 },      /*38*/
    { "TURBO", 8, 15 },                  /*39*/
    { "VOUT_FAULT_RESPONSE", 0, 7 },     /*40*/
    { "IOUT_FAULT_RESPONSE", 8, 15 },    /*41*/
    { "IIN_FAULT_RESPONSE", 0, 7 },      /*42*/
    { "OT_FAULT_RESPONSE", 8, 15 },      /*43*/
    { "UT_FAULT_RESPONSE", 0, 7 },       /*44*/
    { "VIN_UV_FAULT_RESPONSE", 8, 15 },  /*45*/
    { "VIN_OV_FAULT_RESPONSE", 0, 7 },   /*46*/
    { "IOUT_OC_FAULT_RESPONSE", 8, 15 }, /*47*/
    { "IOUT_UC_FAULT_RESPONSE", 0, 7 },  /*48*/
    { "NONE", 0, 15 },                   /*49*/
};

/* This structure defines PMBus 1.3 specs register field information .
   Currently being used by TPS53681 regulator.
*/

t_structFieldInfo const PMBus1_3_FieldInfo[] = {
    { "PAGE", 0, 7 },                 /*0*/
    { "ON", 7, 7 },                   /*1*/
    { "MARGIN", 2, 5 },               /*2*/
    { "PU", 4, 4 },                   /*3*/
    { "CMD", 3, 3 },                  /*4*/
    { "CP", 2, 2 },                   /*5*/
    { "PL", 1, 1 },                   /*6*/
    { "SP", 0, 0 },                   /*7*/
    { "none", 0, -1 },                /*8*/
    { "PHASE", 0, 7 },                /*9*/
    { "WRITE_PROTECT", 0, 7 },        /*10*/
    { "none", 0, -1 },                /*11*/
    { "none", 0, -1 },                /*12*/
    { "PEC", 7, 7 },                  /*13*/
    { "SPD", 5, 6 },                  /*14*/
    { "PMBALRT", 4, 4 },              /*15*/
    { "none", 0, 0 },                 /*16*/
    { "MODE", 5, 7 },                 /*17*/
    { "VID_TYPE", 0, 4 },             /*18*/
    { "VOUT_CMD_VID", 0, 7 },         /*19*/
    { "VOUT_MAX_VID", 0, 7 },         /*20*/
    { "VOUT_MARGH_VID", 0, 7 },       /*21*/
    { "VOUT_MARGL_VID", 0, 7 },       /*22*/
    { "VOTR_EXP", 11, 15 },           /*23*/
    { "VOTR_MAN", 0, 10 },            /*24*/
    { "VDROOP_EXP", 11, 15 },         /*25*/
    { "VDROOP_MAN", 0, 10 },          /*26*/
    { "VOSL_EXP", 11, 15 },           /*27*/
    { "VOSL_MAN", 0, 10 },            /*28*/
    { "VOSL_EXP", 11, 15 },           /*29*/
    { "VOSL_MAN", 0, 10 },            /*30*/
    { "VOUT_MIN_VID", 0, 7 },         /*31*/
    { "FSW_EXP", 11, 15 },            /*32*/
    { "FSW_MAN", 0, 10 },             /*33*/
    { "VINON_EXP", 11, 15 },          /*34*/
    { "VINON_MAN", 0, 10 },           /*35*/
    { "IOCG_EXP", 11, 15 },           /*36*/
    { "IOCG_MAN", 0, 10 },            /*37*/
    { "IOCOS_EXP", 11, 15 },          /*38*/
    { "IOCOS_MAN", 0, 10 },           /*39*/
    { "VO_OVF_VID", 0, 7 },           /*40*/
    { "VO_OV_RESP", 0, 7 },           /*41*/
    { "VO_UVF_VID", 0, 7 },           /*42*/
    { "VO_UV_RESP", 0, 7 },           /*43*/
    { "IOOCF_EXP", 11, 15 },          /*44*/
    { "IOOCF_MAN", 0, 10 },           /*45*/
    { "IO_OC_RESP", 0, 7 },           /*46*/
    { "IOOCW_EXP", 11, 15 },          /*47*/
    { "IOOCW_MAN", 0, 10 },           /*48*/
    { "OTF_EXP", 11, 15 },            /*49*/
    { "OTF_MAN", 0, 10 },             /*50*/
    { "OTF_RESP", 0, 7 },             /*51*/
    { "OTF_EXP", 11, 15 },            /*52*/
    { "OTF_MAN", 0, 10 },             /*53*/
    { "VIN_OVF_EXP", 11, 15 },        /*54*/
    { "VIN_OVF_MAN", 0, 10 },         /*55*/
    { "VI_OVF_RESP", 0, 7 },          /*56*/
    { "VIN_UVF_EXP", 11, 15 },        /*57*/
    { "VIN_UVF_MAN", 0, 10 },         /*58*/
    { "VI_UVF_RESP", 0, 7 },          /*59*/
    { "IIN_OCF_EXP", 11, 15 },        /*60*/
    { "IIN_OCF_MAN", 0, 10 },         /*61*/
    { "IIN_OC_RESP", 0, 7 },          /*62*/
    { "IIN_OCW_EXP", 11, 15 },        /*63*/
    { "IIN_OCW_MAN", 0, 10 },         /*64*/
    { "TON_DLY_EXP", 11, 15 },        /*65*/
    { "TON_DLY_MAN", 0, 10 },         /*66*/
    { "PIN_OPW_EXP", 11, 15 },        /*67*/
    { "PIN_OPW_MAN", 0, 10 },         /*68*/
    { "BUSY", 7, 7 },                 /*69*/
    { "OFF", 6, 6 },                  /*70*/
    { "VOUT_OV", 5, 5 },              /*71*/
    { "IOUT_OC", 4, 4 },              /*72*/
    { "VIN_UV", 3, 3 },               /*73*/
    { "TEMP", 2, 2 },                 /*74*/
    { "CML", 1, 1 },                  /*75*/
    { "OTHER", 0, 0 },                /*76*/
    { "VOUT", 15, 15 },               /*77*/
    { "IOUT", 14, 14 },               /*78*/
    { "INPUT", 13, 13 },              /*79*/
    { "MFR", 12, 12 },                /*80*/
    { "PGOOD", 11, 11 },              /*81*/
    { "FANS", 10, 10 },               /*82*/
    { "OTHER", 9, 9 },                /*83*/
    { "UNKNOWN", 8, 8 },              /*84*/
    { "BUSY", 7, 7 },                 /*85*/
    { "OFF", 6, 6 },                  /*86*/
    { "VOUT_OV", 5, 5 },              /*87*/
    { "IOUT_OC", 4, 4 },              /*88*/
    { "VIN_UV", 3, 3 },               /*89*/
    { "TEMP", 2, 2 },                 /*90*/
    { "CML", 1, 1 },                  /*91*/
    { "OTHER", 0, 0 },                /*92*/
    { "VOUT_OVF", 7, 7 },             /*93*/
    { "VOUT_OVW", 6, 6 },             /*94*/
    { "VOUT_UVW", 5, 5 },             /*95*/
    { "VOUT_UVF", 4, 4 },             /*96*/
    { "VOUT_MIN_MAX", 3, 3 },         /*97*/
    { "TON_MAX", 2, 2 },              /*98*/
    { "TOFF_MAX", 1, 1 },             /*99*/
    { "VOUT_TRACK", 0, 0 },           /*100*/
    { "IOUT_OCF", 7, 7 },             /*101*/
    { "IOUT_OCUVF", 6, 6 },           /*102*/
    { "IOUT_OCW", 5, 5 },             /*103*/
    { "IOUT_UCF", 4, 4 },             /*104*/
    { "CUR_SHAREF", 3, 3 },           /*105*/
    { "POW_LIMIT", 2, 2 },            /*106*/
    { "POUT_OPF", 1, 1 },             /*107*/
    { "POUT_OPW", 0, 0 },             /*108*/
    { "VIN_OVF", 7, 7 },              /*109*/
    { "VIN_OVW", 6, 6 },              /*110*/
    { "VIN_UVW", 5, 5 },              /*111*/
    { "VIN_UVF", 4, 4 },              /*112*/
    { "LOW_VIN", 3, 3 },              /*113*/
    { "IIN_OCF", 2, 2 },              /*114*/
    { "IIN_OCW", 1, 1 },              /*115*/
    { "PIN_OPW", 0, 0 },              /*116*/
    { "OTF", 7, 7 },                  /*117*/
    { "OTW", 6, 6 },                  /*118*/
    { "UTW", 5, 5 },                  /*119*/
    { "UTF", 4, 4 },                  /*120*/
    { "IV_CMD", 7, 7 },               /*121*/
    { "IV_DATA", 6, 6 },              /*122*/
    { "PEC_FAIL", 5, 5 },             /*123*/
    { "MEM", 3, 3 },                  /*124*/
    { "COM_FAIL", 1, 1 },             /*125*/
    { "CML_OTHER", 0, 0 },            /*126*/
    { "MFR_FAULT_PS", 7, 7 },         /*127*/
    { "VSNS_OPEN", 6, 6 },            /*128*/
    { "MAX_PH_WARN", 5, 5 },          /*129*/
    { "TSNS_LOW", 4, 4 },             /*130*/
    { "PHFLT", 0, 0 },                /*131*/
    { "READ_VIN_EXP", 11, 15 },       /*132*/
    { "READ_VIN_MAN", 0, 10 },        /*133*/
    { "READ_IIN_EXP", 11, 15 },       /*134*/
    { "READ_IIN_MAN", 0, 10 },        /*135*/
    { "READ_VOUT_VID", 0, 7 },        /*136*/
    { "READ_IOUT_EXP", 11, 15 },      /*137*/
    { "READ_IOUT_MAN", 0, 10 },       /*138*/
    { "READ_TEMP_EXP", 11, 15 },      /*139*/
    { "READ_TEMP_MAN", 0, 10 },       /*140*/
    { "READ_POUT_EXP", 11, 15 },      /*141*/
    { "READ_POUT_MAN", 0, 10 },       /*142*/
    { "READ_PIN_EXP", 11, 15 },       /*143*/
    { "READ_PIN_MAN", 0, 10 },        /*144*/
    { "PMB_REV_P1", 4, 7 },           /*145*/
    { "PMB_REV_P2", 0, 3 },           /*146*/
    { "MFR_ID", 0, 15 },              /*147*/
    { "MFR_MODEL", 0, 15 },           /*148*/
    { "MFR_REV", 0, 15 },             /*149*/
    { "MFR_DATE", 0, 15 },            /*150*/
    { "MFR_SERIAL", 0, 31 },          /*151*/
    { "IC_DEVICE_ID", 0, 15 },        /*152*/
    { "IC_DEVICE_REV", 0, 15 },       /*153*/
    { "USER_DATA_00", 0, 47 },        /*154*/
    { "USER_DATA_01", 0, 47 },        /*155*/
    { "USER_DATA_02", 0, 47 },        /*156*/
    { "USER_DATA_03", 0, 47 },        /*157*/
    { "USER_DATA_04", 0, 47 },        /*158*/
    { "USER_DATA_05", 0, 47 },        /*159*/
    { "USER_DATA_06", 0, 47 },        /*160*/
    { "USER_DATA_07", 0, 47 },        /*161*/
    { "USER_DATA_08", 0, 47 },        /*162*/
    { "USER_DATA_09", 0, 47 },        /*163*/
    { "USER_DATA_10", 0, 47 },        /*164*/
    { "none", 0, 0 },                 /*165*/
    { "USER_DATA_12", 0, 47 },        /*166*/
    { "TI_INTERNAL", 10, 10 },        /*167*/
    { "VDACDWN_OFS", 8, 9 },          /*168*/
    { "VDACUP_OFS", 6, 7 },           /*169*/
    { "CUR_SHARE_TH", 4, 5 },         /*170*/
    { "PHASE_OCL", 0, 3 },            /*171*/
    { "PH6_IB", 8, 8 },               /*172*/
    { "PH5_IB", 7, 7 },               /*173*/
    { "PH4_IB", 6, 6 },               /*174*/
    { "PH3_IB", 5, 5 },               /*175*/
    { "PH2_IB", 4, 4 },               /*176*/
    { "PH1_IB", 3, 3 },               /*177*/
    { "ACTIVE_PHASES", 0, 2 },        /*178*/
    { "VOUT_LIN_EXP", 11, 15 },       /*179*/
    { "VOUT_LIN_MAN", 0, 10 },        /*180*/
    { "VID_OFFSET_EXTN", 6, 7 },      /*181*/
    { "VID_OFFSET", 0, 5 },           /*182*/
    { "NOSKIP", 15, 15 },             /*183*/
    { "DAC_DOWN_DCLL", 12, 14 },      /*184*/
    { "DAC_UP_DCLL", 8, 10 },         /*185*/
    { "DAC_DOWN_ACLL", 4, 6 },        /*186*/
    { "DAC_UP_ACLL", 0, 3 },          /*187*/
    { "INT_GAIN", 12, 13 },           /*188*/
    { "INT_TC", 8, 11 },              /*189*/
    { "AC_GAIN", 6, 7 },              /*190*/
    { "ACLL", 0, 5 },                 /*191*/
    { "CF_CHA", 3, 5 },               /*192*/
    { "CF_CHB", 0, 2 },               /*193*/
    { "MINOFF", 9, 10 },              /*194*/
    { "TBLANK", 6, 8 },               /*195*/
    { "PH_USR1", 5, 5 },              /*196*/
    { "OSR_USR_HYS", 3, 4 },          /*197*/
    { "USR1", 0, 2 },                 /*198*/
    { "IOUT_MAX", 0, 7 },             /*199*/
    { "VBOOT_VID", 0, 7 },            /*200*/
    { "OSR", 8, 10 },                 /*201*/
    { "OSR_INTSET", 4, 7 },           /*202*/
    { "OSR_TRUNC_BB", 3, 3 },         /*203*/
    { "TMAX", 0, 2 },                 /*204*/
    { "VBOOT_SR", 8, 8 },             /*205*/
    { "VR_MODE", 5, 7 },              /*206*/
    { "TI_INTERNAL", 0, 4 },          /*207*/
    { "DPS_6TO5_FINE_DROP", 14, 15 }, /*208*/
    { "DPS_5TO4_FINE_DROP", 12, 13 }, /*209*/
    { "DPS_4TO3_FINE_DROP", 10, 11 }, /*210*/
    { "DPS_3TO2_FINE_DROP", 8, 9 },   /*211*/
    { "DPS_EN", 7, 7 },               /*212*/
    { "DYN_RAMP_USR", 5, 6 },         /*213*/
    { "DYN_RAMP_2PH", 4, 4 },         /*214*/
    { "DYN_RAMP_1PH", 3, 3 },         /*215*/
    { "RAMP", 0, 2 },                 /*216*/
    { "DPS_DCM", 15, 15 },            /*217*/
    { "DPS_2TO1_FINE_DROP", 13, 14 }, /*218*/
    { "DPS_5TO6_FINE_ADD", 11, 12 },  /*219*/
    { "DPS_4TO5_FINE_ADD", 9, 10 },   /*220*/
    { "DPS_3TO4_FINE_ADD", 7, 8 },    /*221*/
    { "DPS_2TO3_FINE_ADD", 5, 6 },    /*222*/
    { "DPS_1TO2_FINE_ADD", 4, 5 },    /*223*/
    { "DPS_COURSE_TH", 0, 2 },        /*224*/
    { "PHASE_NUM", 0, 2 },            /*225*/
    { "PIN_OP_WARN_LIMIT", 0, 7 },    /*226*/
    { "NVM_SECURITY_KEY", 0, 15 },    /*227*/
};

#endif
#endif
