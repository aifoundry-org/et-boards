/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/
/***********************************************************************/
/*! \file adc.c
    \brief C file for SAMD20 ADC and counter drivers and ISR for PMIC Test Fixture
*/
/***********************************************************************/

#include <stdint.h>
#include "board_defs.h"
#include "adc.h"

#if 0
uint16_t adc_read_channel(uint32_t channel, uint32_t gain)
{
    adc_wait_for_sync();
    ADC->INPUTCTRL.reg = ADC_INPUTCTRL_MUXNEG_GND
/*                       ADC_INPUTCTRL_MUXNEG_IOGND  alternate ground */
                       | ADC_INPUTCTRL_GAIN(gain)
                       | (channel & ADC_INPUTCTRL_MUXPOS_Msk);
    ADC->INTFLAG.reg |= ADC_INTFLAG_RESRDY;
    ADC->SWTRIG.reg |= ADC_SWTRIG_START;
    while((ADC->INTFLAG.reg & ADC_INTFLAG_RESRDY) == 0) {};
    adc_wait_for_sync();
    return ADC->RESULT.reg;
}
#endif

temperature_calibration_t tcal;

int32_t get_compensated_int1v(int32_t temperature)
{
    return(tcal.room_int1v + ((tcal.hot_int1v - tcal.room_int1v) * (temperature - tcal.room_temp))
                                  / (tcal.hot_temp - tcal.room_temp));
}

int32_t get_compensated_temperature(uint16_t adc)
{
    int32_t int1v = get_compensated_int1v(convert_ADC_to_temperature(adc));
    int32_t n = (tcal.hot_temp - tcal.room_temp) * ((adc * int1v) - (tcal.room_adc * tcal.room_int1v));
    int32_t d = (tcal.hot_adc * tcal.hot_int1v) - (tcal.room_adc * tcal.room_int1v);
    return (tcal.room_temp + n/d);

}

temperature_calibration_t *extractTemperatureCalibration(void)
{
    uint32_t LOG0 = ((uint32_t *) NVMCTRL_TEMP_LOG)[0];
    uint32_t LOG1 = ((uint32_t *) NVMCTRL_TEMP_LOG)[1];
    tcal.room_temp = (((LOG0 & NVMCTRL_FUSES_ROOM_TEMP_VAL_INT_Msk) >> NVMCTRL_FUSES_ROOM_TEMP_VAL_INT_Pos) * 10)
           + ((LOG0 & NVMCTRL_FUSES_ROOM_TEMP_VAL_DEC_Msk) >> NVMCTRL_FUSES_ROOM_TEMP_VAL_DEC_Pos);
    tcal.hot_temp = (((LOG0 & NVMCTRL_FUSES_HOT_TEMP_VAL_INT_Msk) >> NVMCTRL_FUSES_HOT_TEMP_VAL_INT_Pos) * 10)
           + ((LOG0 & NVMCTRL_FUSES_HOT_TEMP_VAL_DEC_Msk) >> NVMCTRL_FUSES_HOT_TEMP_VAL_DEC_Pos);

    tcal.room_int1v = 1000 - (int32_t) ((int8_t) ((LOG0 & NVMCTRL_FUSES_ROOM_INT1V_VAL_Msk) >> NVMCTRL_FUSES_ROOM_INT1V_VAL_Pos));
    tcal.hot_int1v = 1000 - (int32_t) ((int8_t) ((LOG1 & NVMCTRL_FUSES_HOT_INT1V_VAL_Msk) >> NVMCTRL_FUSES_HOT_INT1V_VAL_Pos));
    tcal.room_adc = (LOG1 & NVMCTRL_FUSES_ROOM_ADC_VAL_Msk) >> NVMCTRL_FUSES_ROOM_ADC_VAL_Pos;
    tcal.hot_adc = (LOG1 & NVMCTRL_FUSES_HOT_ADC_VAL_Msk) >> NVMCTRL_FUSES_HOT_ADC_VAL_Pos;
    return &tcal;

}
