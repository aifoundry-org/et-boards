/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file scripts.c
    \brief C file for scripts related functions
*/
/***********************************************************************/

#include <stddef.h>
#include "scripts.h"
char const * scriptPowerOn[] =
{
"# set all outputs low before making the outputs",
"sg pa10 0 #SOC_RESET_N",
"sg pa11 0 #MCU_PERST_OUT_N",
"sg pa24 0 #P3_3_EN",
"sg pa25 0 #P1_8_EN",
"sg pb22 0 #TPS_RESET_N",
"sg pb23 0 #MNN_ENA",
"sg pa27 0 #NOC_ENA",
"sg pb31 0 #PMIC1_GPI0",
"sg pb00 0 #PMIC1_GPI1",
"sg pb01 0 #PMIC1_EN",
"sg pa02 0 #PMIC2_EN0",
"sg pa01 0 #PMIC2_EN1",

"#make them outputs",
"sg pa10 out #SOC_RESET_N",
"sg pa11 out #MCU_PERST_OUT_N",
"sg pa24 out #P3_3_EN",
"sg pa25 out #P1_8_EN",
"sg pb22 out #TPS_RESET_N",
"sg pb23 out #MNN_ENA",
"sg pa27 out #NOC_ENA",
"sg pb31 out #PMIC1_GPI0",
"sg pb00 out #PMIC1_GPI1",
"sg pb01 out #PMIC1_EN",
"sg pa02 out #PMIC2_EN0",
"sg pa01 out #PMIC2_EN1",

"#program the PMIDs",
"sc tps",
"sp ff",
"sr ON_OFF_CONFIG 17",
"sr VOUT_COMMAND 1E",
"sr VOUT_MAX 2E",
"#sr VOUT_MARGIN_HIGH tbd",
"#sr VOUT_MARGIN_LOW tbd",
"sr VOUT_MIN 1A",
"#sr IOUT_OC_FAULT_LIMIT tbd",
"#sr IOUT_OC_WARN_LIMIT tbd",
"#sr OT_OC_FAULT_LIMIT tbd",
"#sr OT_OC_WARN_LIMIT tbd",
"sr OTF_RESP 80",

"sc 77812",
"sr GPI_FUNC FF",
"sr PROT_CFG 87",
"sr I2C_CFG 02",
"sr M1_VOUT$ 50",
"sr M2_VOUT$ 50",
"sr M3_VOUT$ 78",
"sr M4_VOUT$ 78",
"sr M1_VOUT_D 50",
"sr M2_VOUT_D 50",
"sr M3_VOUT_D 78",
"sr M4_VOUT_D 78",
"sr M1_VOUT_S 50",
"sr M2_VOUT_S 50",
"sr M3_VOUT_S 78",
"sr M4_VOUT_S 78",

"sc 77714",
"sr SD0_CNFG1 55",
"sr SD1_CNFG1 23",
"sr SD2_CNFG1 18",
"sr SD3_CNFG1 48",
"sr LDO_CNFG1_L0 28",
"sr LDO_CNFG1_L1 28",
"sr LDO_CNFG1_L2 14",
"sr LDO_CNFG1_L3 14",
"#sr LDO_CNFG1_L4 unused",
"sr LDO_CNFG1_L5 14",
"sr LDO_CNFG1_L6 14",
"#sr LDO_CNFG1_L7 unused",
"sr LDO_CNFG1_L8 14",
"sr LDO0FPS 00",
"sr LDO1FPS 00",
"sr LDO2FPS 00",
"sr LDO3FPS 00",
"sr LDO4FPS C0",
"sr LDO5FPS 00",
"sr LDO6FPS 00",
"sr LDO7FPS C0",
"sr LDO8FPS 00",
"sr GPIO0FPS C0",
"sr GPIO1FPS C0",
"sr GPIO2FPS C0",
"sr GPIO7FPS C0",
"sr RSTIOFPS C0",
"sc 6660",
"sr SPOR 0 #0 is dummy byte",
"sr TMAX 68 #100C",
"sr THYST 5F #95C",

"# enable power and release reset",
"sg pb22 1 #TPS_RESET_N",
"sg pb23 1 #MNN_ENA",
"sg pa27 1 #NOC_ENA",
"sg pb01 1 #PMIC1_EN",
"sg pa02 1 #PMIC2_EN0",
"sg pa24 1 #P3_3_EN",
"sg pa25 1 #P1_8_EN",
"sg pa10 1 #SOC_RESET_N",
"sg pa11 1 #MCU_PERST_OUT_N",
NULL
};

char const * scriptPowerOff[] =
{
"# set all outputs low before making the outputs",
"sg pa10 0 #SOC_RESET_N",
"sg pa11 0 #MCU_PERST_OUT_N",
"sg pa24 0 #P3_3_EN",
"sg pa25 0 #P1_8_EN",
"sg pb22 0 #TPS_RESET_N",
"sg pb23 0 #MNN_ENA",
"sg pa27 0 #NOC_ENA",
"sg pb31 0 #PMIC1_GPI0",
"sg pb00 0 #PMIC1_GPI1",
"sg pb01 0 #PMIC1_EN",
"sg pa02 0 #PMIC2_EN0",
"sg pa01 0 #PMIC2_EN1",
NULL
};



scriptStruct_t scriptList [] =
{
    { "poweron", scriptPowerOn },
    { "poweroff", scriptPowerOff },
     {   NULL, NULL }
};

int const MAX_SCRIPTLIST = sizeof(scriptList)/sizeof(scriptStruct_t);

