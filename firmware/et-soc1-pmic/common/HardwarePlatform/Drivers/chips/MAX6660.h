/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/
#ifndef MAX6660_H
#define MAX6660_H

#define MAX6660__RL 0x00
#define MAX6660__RL__PROT eRT_R8
#define MAX6660__RL__BYTES 1
#define MAX6660__RH 0x01
#define MAX6660__RH__PROT eRT_R8
#define MAX6660__RH__BYTES 1
#define MAX6660__SL 0x02
#define MAX6660__SL__PROT eRT_R8
#define MAX6660__SL__BYTES 1
#define MAX6660__CL 0x0903
#define MAX6660__CL__PROT eRT_RW8SEPREG
#define MAX6660__CL__BYTES 1
#define MAX6660__FCR 0x0A04
#define MAX6660__FCR__PROT eRT_RW8SEPREG
#define MAX6660__FCR__BYTES 1
#define MAX6660__TMAX 0x1210
#define MAX6660__TMAX__PROT eRT_RW8SEPREG
#define MAX6660__TMAX__BYTES 1
#define MAX6660__THYST 0x1311
#define MAX6660__THYST__PROT eRT_RW8SEPREG
#define MAX6660__THYST__BYTES 1
#define MAX6660__THIGH 0x0D07
#define MAX6660__THIGH__PROT eRT_RW8SEPREG
#define MAX6660__THIGH__BYTES 1
#define MAX6660__TLOW 0x0E08
#define MAX6660__TLOW__PROT eRT_RW8SEPREG
#define MAX6660__TLOW__BYTES 1
#define MAX6660__SPOR 0xFC
#define MAX6660__SPOR__PROT eRT_W0
#define MAX6660__SPOR__BYTES 1
#define MAX6660__OSHT 0x0F
#define MAX6660__OSHT__PROT eRT_W0
#define MAX6660__OSHT__BYTES 1
#define MAX6660__TFAN 0x1914
#define MAX6660__TFAN__PROT eRT_RW8SEPREG
#define MAX6660__TFAN__BYTES 1
#define MAX6660__FSC 0x1A15
#define MAX6660__FSC__PROT eRT_RW8SEPREG
#define MAX6660__FSC__BYTES 1
#define MAX6660__FG 0x1B16
#define MAX6660__FG__PROT eRT_RW8SEPREG
#define MAX6660__FG__BYTES 1
#define MAX6660__FTC 0x17
#define MAX6660__FTC__PROT eRT_R8
#define MAX6660__FTC__BYTES 1
#define MAX6660__FTCL 0x1C18
#define MAX6660__FTCL__PROT eRT_RW8SEPREG
#define MAX6660__FTCL__BYTES 1
#define MAX6660__FCD 0x1E1D
#define MAX6660__FCD__PROT eRT_RW8SEPREG
#define MAX6660__FCD__BYTES 1
#define MAX6660__FS 0x201F
#define MAX6660__FS__PROT eRT_RW8SEPREG
#define MAX6660__FS__BYTES 1
#define MAX6660__M 0xFBFA
#define MAX6660__M__PROT eRT_RW8SEPREG
#define MAX6660__M__BYTES 1
#define MAX6660__MFGIDCODE 0xFE
#define MAX6660__MFGIDCODE__PROT eRT_R8
#define MAX6660__MFGIDCODE__BYTES 1
#define MAX6660__DEVIDCODE 0x9D
#define MAX6660__DEVIDCODE__PROT eRT_R8
#define MAX6660__DEVIDCODE__BYTES 1

#define MAX6660__SL__OVERT_LOCAL__SHIFT 7
#define MAX6660__SL__OVERT_LOCAL__MASK 0x01
#define MAX6660__SL__ALERT__SHIFT 6
#define MAX6660__SL__ALERT__MASK 0x01
#define MAX6660__SL__FAN_FULL__SHIFT 5
#define MAX6660__SL__FAN_FULL__MASK 0x01
#define MAX6660__SL__OVERT_REMOTE__SHIFT 4
#define MAX6660__SL__OVERT_REMOTE__MASK 0x01
#define MAX6660__SL__UNDERT_REMOTE__SHIFT 3
#define MAX6660__SL__UNDERT_REMOTE__MASK 0x01
#define MAX6660__SL__OPEN_REMOTE__SHIFT 2
#define MAX6660__SL__OPEN_REMOTE__MASK 0x01
#define MAX6660__SL__OVERT__SHIFT 1
#define MAX6660__SL__OVERT__MASK 0x01
#define MAX6660__SL__FTC_TOOHIGH__SHIFT 0
#define MAX6660__SL__FTC_TOOHIGH__MASK 0x01
#define MAX6660__CL__ALERT_MASK__SHIFT 7
#define MAX6660__CL__ALERT_MASK__MASK 0x01
#define MAX6660__CL__NOT_RUN__SHIFT 6
#define MAX6660__CL__NOT_RUN__MASK 0x01
#define MAX6660__CL__OVERT_POLHI__SHIFT 5
#define MAX6660__CL__OVERT_POLHI__MASK 0x01
#define MAX6660__CL__WRITE_PROT__SHIFT 4
#define MAX6660__CL__WRITE_PROT__MASK 0x01
#define MAX6660__CL__OPENLOOP__SHIFT 3
#define MAX6660__CL__OPENLOOP__MASK 0x01
#define MAX6660__CL__OVERT_INP_INH__SHIFT 2
#define MAX6660__CL__OVERT_INP_INH__MASK 0x01
#define MAX6660__CL__OVERT_MASK_OUTP__SHIFT 1
#define MAX6660__CL__OVERT_MASK_OUTP__MASK 0x01
#define MAX6660__CL__ALERT_READ_NO_CLEAR__SHIFT 0
#define MAX6660__CL__ALERT_READ_NO_CLEAR__MASK 0x01


/* Table data */

#include <stdint.h>
#include "chips.h"


extern t_structRegInfo const structRegInfo_MAX6660[];

#define MAXREG_MAX6660 (21) //(sizeof(structRegInfo_MAX6660)/sizeof(t_structRegInfo))

extern t_structFieldInfo const structFieldInfo_MAX6660[];

#ifdef INSTANTIATE_DEVINFO

t_structRegInfo const structRegInfo_MAX6660[] = {
    {"RL", 0, 0, 0x00,  0x00, eRT_R8, 1 }, /*0*/
    {"RH", 0, 0, 0x01,  0x01, eRT_R8, 1 }, /*1*/
    {"SL", 0, 8, 0x02,  0x02, eRT_R8, 1 }, /*2*/
    {"CL", 8, 8, 0x03,  0x09, eRT_RW8SEPREG, 1 }, /*3*/
    {"FCR", 16, 0, 0x04,  0x0A, eRT_RW8SEPREG, 1 }, /*4*/
    {"TMAX", 16, 0, 0x10,  0x12, eRT_RW8SEPREG, 1 }, /*5*/
    {"THYST", 16, 0, 0x11,  0x13, eRT_RW8SEPREG, 1 }, /*6*/
    {"THIGH", 16, 0, 0x07,  0x0D, eRT_RW8SEPREG, 1 }, /*7*/
    {"TLOW", 16, 0, 0x08,  0x0E, eRT_RW8SEPREG, 1 }, /*8*/
    {"SPOR", 16, 0, 0xFF,  0xFC, eRT_W0, 1 }, /*9*/
    {"OSHT", 16, 0, 0xFF,  0x0F, eRT_W0, 1 }, /*10*/
    {"TFAN", 16, 0, 0x14,  0x19, eRT_RW8SEPREG, 1 }, /*11*/
    {"FSC", 16, 0, 0x15,  0x1A, eRT_RW8SEPREG, 1 }, /*12*/
    {"FG", 16, 0, 0x16,  0x1B, eRT_RW8SEPREG, 1 }, /*13*/
    {"FTC", 16, 0, 0x17,  0x17, eRT_R8, 1 }, /*14*/
    {"FTCL", 16, 0, 0x18,  0x1C, eRT_RW8SEPREG, 1 }, /*15*/
    {"FCD", 16, 0, 0x1D,  0x1E, eRT_RW8SEPREG, 1 }, /*16*/
    {"FS", 16, 0, 0x1F,  0x20, eRT_RW8SEPREG, 1 }, /*17*/
    {"M", 16, 0, 0xFA,  0xFB, eRT_RW8SEPREG, 1 }, /*18*/
    {"MFGIDCODE", 16, 0, 0xFE,  0xFE, eRT_R8, 1 }, /*19*/
    {"DEVIDCODE", 16, 0, 0x9D,  0x9D, eRT_R8, 1 }, /*20*/
};


t_structFieldInfo const structFieldInfo_MAX6660[] = {
    {"OVERT_LOCAL",7,7}, /*0*/
    {"ALERT",6,6}, /*1*/
    {"FAN_FULL",5,5}, /*2*/
    {"OVERT_REMOTE",4,4}, /*3*/
    {"UNDERT_REMOTE",3,3}, /*4*/
    {"OPEN_REMOTE",2,2}, /*5*/
    {"OVERT",1,1}, /*6*/
    {"FTC_TOOHIGH",0,0}, /*7*/
    {"ALERT_MASK",7,7}, /*8*/
    {"NOT_RUN",6,6}, /*9*/
    {"OVERT_POLHI",5,5}, /*10*/
    {"WRITE_PROT",4,4}, /*11*/
    {"OPENLOOP",3,3}, /*12*/
    {"OVERT_INP_INH",2,2}, /*13*/
    {"OVERT_MASK_OUTP",1,1}, /*14*/
    {"ALERT_READ_NO_CLEAR",0,0}, /*15*/
};


#endif /* INSTANTIATE_DEVINFO */
#endif /* MAX6660_H */
