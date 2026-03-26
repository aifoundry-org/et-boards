/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/
#ifndef FS1406_H
#define FS1406_H

#define FS1406__VOUT_HIGH 0x12
#define FS1406__VOUT_HIGH__PROT eRT_None
#define FS1406__VOUT_HIGH__BYTES 1
#define FS1406__VOUT_LOW 0x13
#define FS1406__VOUT_LOW__PROT eRT_None
#define FS1406__VOUT_LOW__BYTES 1
#define FS1406__REG14 0x14
#define FS1406__REG14__PROT eRT_None
#define FS1406__REG14__BYTES 1
#define FS1406__REG15 0x15
#define FS1406__REG15__PROT eRT_None
#define FS1406__REG15__BYTES 1
#define FS1406__REG17 0x17
#define FS1406__REG17__PROT eRT_None
#define FS1406__REG17__BYTES 1
#define FS1406__REG18 0x18
#define FS1406__REG18__PROT eRT_None
#define FS1406__REG18__BYTES 1
#define FS1406__REG19 0x19
#define FS1406__REG19__PROT eRT_None
#define FS1406__REG19__BYTES 1
#define FS1406__REG1A 0x1A
#define FS1406__REG1A__PROT eRT_None
#define FS1406__REG1A__BYTES 1
#define FS1406__REG1C 0x1C
#define FS1406__REG1C__PROT eRT_None
#define FS1406__REG1C__BYTES 1
#define FS1406__REG1D 0x1D
#define FS1406__REG1D__PROT eRT_None
#define FS1406__REG1D__BYTES 1
#define FS1406__REG20 0x20
#define FS1406__REG20__PROT eRT_None
#define FS1406__REG20__BYTES 1
#define FS1406__REG21 0x21
#define FS1406__REG21__PROT eRT_None
#define FS1406__REG21__BYTES 1

#define FFS1406__VOUT_HIGH__BYTE__SHIFT 0
#define FFS1406__VOUT_HIGH__BYTE__MASK 0xFF
#define FFS1406__VOUT_LOW__BYTE__SHIFT 0
#define FFS1406__VOUT_LOW__BYTE__MASK 0xFF
#define FS1406__REG14__PG_CONTROL__SHIFT 0
#define FS1406__REG14__PG_CONTROL__MASK 0x01
#define FS1406__REG14__SOFT_STOP_ENABLE__SHIFT 2
#define FS1406__REG14__SOFT_STOP_ENABLE__MASK 0x01
#define FS1406__REG14__SS_RATE__SHIFT 3
#define FS1406__REG14__SS_RATE__MASK 0x01
#define FS1406__REG15__OC_SET__SHIFT 0
#define FS1406__REG15__OC_SET__MASK 0x07
#define FS1406__REG17__OV_THRESHOLD__SHIFT 0
#define FS1406__REG17__OV_THRESHOLD__MASK 0x03
#define FS1406__REG18__PG_THRESHOLD__SHIFT 0
#define FS1406__REG18__PG_THRESHOLD__MASK 0x03
#define FS1406__REG19__OT_THRESHOLD__SHIFT 0
#define FS1406__REG19__OT_THRESHOLD__MASK 0x03
#define FS1406__REG1A__BUS_VOLTAGE_SEL__SHIFT 1
#define FS1406__REG1A__BUS_VOLTAGE_SEL__MASK 0x01
#define FS1406__REG1C__SOFT_DISABLE__SHIFT 3
#define FS1406__REG1C__SOFT_DISABLE__MASK 0x01
#define FS1406__REG1D__USER_OPT__SHIFT 1
#define FS1406__REG1D__USER_OPT_MASK 0x01
#define FS1406__REG1D__TRIM_OPT__SHIFT 0
#define FS1406__REG1D__TRIM_OPT_MASK 0x01

#define FS1406__REG20__USER_POINTER__SHIFT 3
#define FS1406__REG20__USER_POINTER_MASK 0x07
#define FS1406__REG20__TRIM_POINTER__SHIFT 0
#define FS1406__REG20__TRIM_POINTER_MASK 0x07

#define FS1406__REG21__STATUS_PGOOD__SHIFT 7
#define FS1406__REG21__STATUS_PGOOD__MASK 0x01
#define FS1406__REG21__STATUS_ENABLE__SHIFT 3
#define FS1406__REG21__STATUS_ENABLE__MASK 0x01
#define FS1406__REG21__STATUS_TRIM_BURN__SHIFT 2
#define FS1406__REG21__STATUS_TRIM_BURN__MASK 0x01
#define FS1406__REG21__STATUS_USER_BURN__SHIFT 1
#define FS1406__REG21__TATUS_USER_BURN__MASK 0x01

/* Table data */

#include <stdint.h>
#include "chips.h"


extern t_structRegInfo const structRegInfo_FS1406[];

#define MAXREG_FS1406 (12) // (sizeof(structRegInfo_FS1406)/sizeof(t_structRegInfo))

extern t_structFieldInfo const structFieldInfo_FS1406[];

#ifdef INSTANTIATE_DEVINFO

t_structRegInfo const structRegInfo_FS1406[MAXREG_FS1406] = {
    {"VOUT_HIGH", 0, 1, 0x12,  0x12, eRT_None, 1 }, /*0*/
    {"VOUT_LOW",  1, 1, 0x13,  0x13, eRT_None, 1 }, /*1*/
    {"PG_CONTROL",2, 3, 0x14,  0x14, eRT_None, 1 }, /*2*/
    {"OC_SET",    5, 1, 0x15,  0x15, eRT_None, 1 }, /*3*/
    {"OV_THRESHOLD",6, 1, 0x17,  0x17, eRT_None, 1 }, /*4*/
    {"PG_THRESHOLD",7, 1, 0x18,  0x18, eRT_None, 1 }, /*5*/
    {"OT_THRESHOLD",8, 1, 0x19,  0x19, eRT_None, 1 }, /*6*/
    {"BUS_VOLTAGE_SEL",9, 1, 0x1A,  0x1A, eRT_None, 1 }, /*7*/
    {"SOFT_DISABLE",10, 1, 0x1C,  0x1C, eRT_None, 1 }, /*8*/
    {"USER_OPT",  11, 2, 0x1D,  0x1D, eRT_None, 1 }, /*9*/
    {"USER_PTR",  13, 2, 0x20,  0x20, eRT_None, 1 }, /*10*/
    {"STATUS",    15, 4, 0x21,  0x21, eRT_None, 1 }, /*11*/
};


t_structFieldInfo const structFieldInfo_FS1406[] = {
    {"BYTE",0,7}, /*0*/
    {"BYTE",0,7}, /*1*/
    {"PG_CONTROL",0,0}, /*2*/
    {"SOFT_STOP_ENABLE",2,2}, /*3*/
    {"SS_RATE",3,3}, /*4*/
    {"OC_SET",2,0}, /*5*/
    {"OV_THRESHOLD_",0,1}, /*6*/
    {"PG_THRESHOLD",0,1}, /*7*/
    {"OT_THRESHOLD",0,1}, /*8*/
    {"BUS_VOLTAGE_SEL",1,1}, /*9*/
    {"USER_OPT_ON",3,3}, /*10*/
    {"TRIM_OPT_ON",3,3}, /*10*/
    {"USER_POINTER",3,3}, /*10*/
    {"SOFT_DISABLE",3,3}, /*10*/
    {"USER_OPT_ON",1,1}, /*11*/
    {"TRIM_OPT_ON",0,0}, /*12*/
    {"USER_POINTER",5,3}, /*13*/
    {"TRIM_POINTER",2,0}, /*14*/
    {"STATUS_PGOOD",7,7}, /*15*/
    {"STATUS_ENABLE",3,3}, /*16*/
    {"STATUS_TRIM_BURN",2,2}, /*17*/
    {"STATUS_USER_BURN",1,1}, /*18*/
};


#endif /* INSTANTIATE_DEVINFO */
#endif /* FS1406_H */
