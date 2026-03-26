/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/
#ifndef CHIPS_H
#define CHIPS_H

enum { eRT_None, eRT_W0, eRT_RW8, eRT_R8, eRT_W8, eRT_RW8SEPREG, eRT_RW16, eRT_R16, eRT_W16, eRT_RWBLK, eRT_RBLK, eRT_WBLK, eRT_SMBALERT_MASK, eRT_SMBALERT_MASK2 };
enum { eP_0Bit=(1<<0), eP_8Bit=(1<<1), eP_16Bit=(1<<2), eP_Blk=(1<<3), eP_AlMsk=(1<<4), eP_SepReg=(1<<5), eP_Read=(1<<6), eP_Write=(1<<7) };
extern uint8_t const xlatRTtoProp[];

#ifdef INSTANTIATE_DEVINFO
uint8_t const xlatRTtoProp[] =
{
    /*eRT_None*/   eP_8Bit | eP_Read | eP_Write,
    /*eRT_W0*/     eP_0Bit |           eP_Write ,
    /*eRT_RW8*/    eP_8Bit | eP_Read | eP_Write,
    /*eRT_R8*/     eP_8Bit | eP_Read ,
    /*eRT_W8*/     eP_8Bit |           eP_Write,
    /*eRT_RW8SEPREG */ eP_8Bit | eP_Read | eP_Write | eP_SepReg,
    /*eRT_RW16*/   eP_16Bit| eP_Read | eP_Write,
    /*eRT_R16*/    eP_16Bit| eP_Read,
    /*eRT_W16*/    eP_16Bit|           eP_Write,
    /*eRT_RWBLK*/  eP_Blk  | eP_Read | eP_Write,
    /*eRT_RBLK*/   eP_Blk  | eP_Read,
    /*eRT_WBLK*/   eP_Blk  | eP_Write,
    /*eRT_SMBALERT_MASK*/    eP_AlMsk,
    /*eRT_SMBALERT_MASK2*/   eP_AlMsk,
};
#endif

typedef struct
{
    char const * regName;
    uint16_t firstFieldIdx;
    uint8_t numFieldIdx;
    uint8_t regnum;
    uint8_t wrtieRegnum;
    uint8_t epType;
    uint8_t nbytes;
} t_structRegInfo;


typedef struct
{
    char const * fieldName;
    uint8_t loBit;
    uint8_t hiBit;
} t_structFieldInfo;


#endif /* CHIPS_H */
