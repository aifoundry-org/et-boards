/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/
/***********************************************************************/
/*! \file chipmacro.h
    \brief Chip related definitions.
*/

#ifndef CHIPMACRO_H
#define CHIPMACRO_H

#define JOIN(x, y) JOIN_AGAIN(x, y)
#define JOIN_AGAIN(x, y) x ## y

#define JOIN3(x, y, z) JOIN_AGAIN3(x, y, z)
#define JOIN_AGAIN3(x, y, z) x ## y ## z

#define JOIN4(w, x, y, z) JOIN_AGAIN4(w, x, y, z)
#define JOIN_AGAIN4(w, x, y, z) w ## x ## y ## z

#define JOIN6(u, v, w, x, y, z) JOIN_AGAIN6(u, v, w, x, y, z)
#define JOIN_AGAIN6(u, v, w, x, y, z) u ## v ## w ## x ## y ## z


#define CHIP_ID_( chipname ) JOIN3( eID_, chipname, CHIP_ID )
#define CHIP_ADDR7( chipname ) JOIN( ADDR7_, chipname )
#define REG_NUM( chipname, regname ) JOIN3( chipname, __, regname )
#define REG_PROT( chipname, regname ) JOIN4(chipname, __, regname , __PROT)
#define REG_BYTES( chipname, regname ) JOIN4(chipname, __, regname , __BYTES)
#define FIELD_SHIFT( chipname, regname, fieldname ) JOIN3( chipname, __, regname, __, fieldname, __SHIFT )
#define FIELD_MASK( chipname, regname, fieldname ) JOIN6( chipname, __, regname, __, fieldname, __MASK )

#define SETREGVAL_PMBUS(type, regname, regval)                                       \
    hwSetRegVal(JOIN(eID_, type), REG_NUM(PMBUS, regname), REG_PROT(PMBUS, regname), \
        REG_BYTES(PMBUS, regname), regval, pcprf);

#define SETREGVAL_MFR(type, regname, regval, chipID)                                        \
    hwSetRegVal(JOIN3(eID_, type, chipID), REG_NUM(type, regname), REG_PROT(type, regname), \
        REG_BYTES(type, regname), regval, pcprf);

#define SETCHKREGVAL_PMBUS(type, regname, regval)                                       \
    hwSetChkRegVal(JOIN(eID_, type), REG_NUM(PMBUS, regname), REG_PROT(PMBUS, regname), \
        REG_BYTES(PMBUS, regname), regval, pcprf);

#define GETREGVAL(regname)                                                                   \
    hwGetRegVal(CHIP_ID_(CHIPNAME), REG_NUM(CHIPTYPE, regname), REG_PROT(CHIPNAME, regname), \
        REG_BYTES(CHIPNAME, regname), pcprf);
#define CHKREGVAL(regname, val)                                                              \
    hwChkRegVal(CHIP_ID_(CHIPTYPE), REG_NUM(CHIPNAME, regname), REG_PROT(CHIPNAME, regname), \
        REG_BYTES(CHIPNAME, regname), val, pcprf);
#define PAUSE(ms) hwPause( ms, pcprf );

#define CHIPADDR (structChipInfo[CHIP_ID_(CHIPNAME)].addr7)
#define DUMMY 0

#endif /* CHIPMACRO_H */
