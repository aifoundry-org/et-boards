/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file adc.h
    \brief ADC driver.
*/
/***********************************************************************/


#ifndef __ADC_H__
#define __ADC_H__


#define ADC_FULL_SCALE_MV    (1000UL)   // from ADC programming
#define ADC_FULL_SCALE_COUNT (4096UL)   // from ADC Atmel datasheet
#define VOLTAGE_DIVIDER_INVERSE_RATIO (12.54f) // for VMON input - from schematic (9530 Ohm + 110000 Ohm) / 9530 Ohm

// for IMON input
#define HOTSWAP_SENSE_RESISTOR (.001f)           // Ohm from 1 milliohm - from schematic
#define LTC4218_TRANSCONDUCTANCE (.00667f)       // Mho from 6.67 uA/mV - from 4218 datasheet page 7 "IMON"
#define LTC4218_OUTPUT_LOAD_RESISTOR (12400)    // Ohm from 12.4k from - schematic
#define CUR_ADJ_FACTOR (1.0787f) // fudge factor due to PCB resistance near R406 (PCIE card) - average of two cards
#define SHIFT_FOR_INCREASED_PRECISION (9)
#define IMON_DENOMINATOR  ( \
              CUR_ADJ_FACTOR \
            * HOTSWAP_SENSE_RESISTOR \
            * LTC4218_TRANSCONDUCTANCE \
            * LTC4218_OUTPUT_LOAD_RESISTOR \
            * ADC_FULL_SCALE_COUNT \
                    ) // constant floating point number

/*
 * ROOM_TEMP_VAL_INT 20
 * ROOM_TEMP_VAL_DEC 1
 * HOT_TEMP_VAL_INT 55
 * HOT_TEMP_VAL_DEC 0
 * ROOM_INT1V_VAL F6 (-10) 1.010V = 1- (-10/1000)
 * HOT_INT1V_VAL EF (-17) 1.017V = 1 - (-17/1000)
 * ROOM_ADC_VAL B3A (2874 0.702V)
 * HOT_ADC_VAL D33 (3379 0.825V)

 * ROOM TEMP 32.1
 * HOT TEMP 85.0
 * Note Raw Temperature is 2.4mV/C� * T + 607, at 25C ADC = 0.667V (2732 or 0xAAC)
*/

typedef struct {
    int32_t room_temp;
    int32_t hot_temp;
    int32_t room_int1v;
    int32_t hot_int1v;
    uint32_t room_adc;
    uint32_t hot_adc;
} temperature_calibration_t;

temperature_calibration_t *extractTemperatureCalibration(void);

/* @brief adc_read_channel
 * @param[in] uint32_t channel or muxpos - note muxneg is ground
 * ADC_INPUTCTRL_MUXPOS(value) (ADC_INPUTCTRL) Positive Mux Input Selection
 *    ADC_INPUTCTRL_MUXPOS_PIN0 (ADC_INPUTCTRL) ADC AIN0 Pin
 *    ADC_INPUTCTRL_MUXPOS_PIN1 (ADC_INPUTCTRL) ADC AIN1 Pin
 *    ADC_INPUTCTRL_MUXPOS_PIN2 (ADC_INPUTCTRL) ADC AIN2 Pin
 *    ADC_INPUTCTRL_MUXPOS_PIN3 (ADC_INPUTCTRL) ADC AIN3 Pin
 *    ADC_INPUTCTRL_MUXPOS_PIN4 (ADC_INPUTCTRL) ADC AIN4 Pin
 *    ADC_INPUTCTRL_MUXPOS_PIN5 (ADC_INPUTCTRL) ADC AIN5 Pin
 *    ADC_INPUTCTRL_MUXPOS_PIN6 (ADC_INPUTCTRL) ADC AIN6 Pin
 *    ADC_INPUTCTRL_MUXPOS_PIN7 (ADC_INPUTCTRL) ADC AIN7 Pin
 *    ADC_INPUTCTRL_MUXPOS_PIN8 (ADC_INPUTCTRL) ADC AIN8 Pin
 *    ADC_INPUTCTRL_MUXPOS_PIN9 (ADC_INPUTCTRL) ADC AIN9 Pin
 *    ADC_INPUTCTRL_MUXPOS_PIN10 (ADC_INPUTCTRL) ADC AIN10 Pin
 *    ADC_INPUTCTRL_MUXPOS_PIN11 (ADC_INPUTCTRL) ADC AIN11 Pin
 *    ADC_INPUTCTRL_MUXPOS_PIN12 (ADC_INPUTCTRL) ADC AIN12 Pin
 *    ADC_INPUTCTRL_MUXPOS_PIN13 (ADC_INPUTCTRL) ADC AIN13 Pin
 *    ADC_INPUTCTRL_MUXPOS_PIN14 (ADC_INPUTCTRL) ADC AIN14 Pin
 *    ADC_INPUTCTRL_MUXPOS_PIN15 (ADC_INPUTCTRL) ADC AIN15 Pin
 *    ADC_INPUTCTRL_MUXPOS_PIN16 (ADC_INPUTCTRL) ADC AIN16 Pin
 *    ADC_INPUTCTRL_MUXPOS_PIN17 (ADC_INPUTCTRL) ADC AIN17 Pin
 *    ADC_INPUTCTRL_MUXPOS_PIN18 (ADC_INPUTCTRL) ADC AIN18 Pin
 *    ADC_INPUTCTRL_MUXPOS_PIN19 (ADC_INPUTCTRL) ADC AIN19 Pin
 *    ADC_INPUTCTRL_MUXPOS_TEMP (ADC_INPUTCTRL) Temperature Reference
 *    ADC_INPUTCTRL_MUXPOS_BANDGAP (ADC_INPUTCTRL) Bandgap Voltage
 *    ADC_INPUTCTRL_MUXPOS_SCALEDCOREVCC (ADC_INPUTCTRL) 1/4  Scaled Core Supply
 *    ADC_INPUTCTRL_MUXPOS_SCALEDIOVCC (ADC_INPUTCTRL) 1/4  Scaled I/O Supply
 *    ADC_INPUTCTRL_MUXPOS_DAC (ADC_INPUTCTRL) DAC Output
 * @param[in] gain
 * gain is one of the following
 *    ADC_INPUTCTRL_GAIN_1X (ADC_INPUTCTRL) 1x
 *    ADC_INPUTCTRL_GAIN_2X (ADC_INPUTCTRL) 2x
 *    ADC_INPUTCTRL_GAIN_4X (ADC_INPUTCTRL) 4x
 *    ADC_INPUTCTRL_GAIN_8X (ADC_INPUTCTRL) 8x
 *    ADC_INPUTCTRL_GAIN_16X (ADC_INPUTCTRL) 16x
 *    ADC_INPUTCTRL_GAIN_DIV2 (ADC_INPUTCTRL) 1/2x
 * @return uint16_t adc result
 */
uint16_t adc_read_channel(uint32_t channel, uint32_t gain);
/*
 * @brief get_compensated_int1v
 * @param[in] temperature - coarse temperature which is scaled by 10 ie 25.2 is 252
 * @return fine estimate of int1V regulator in mV ie 1000 = 1V
 */
int32_t get_compensated_int1v(int32_t temperature);
/*
 * @brief get_compensated_temperature
 * @param[in] adc_temperature - measured 12 bit ADC temperature
 * @return fine estimate of temperature scaled by 10
 */
 int32_t get_compensated_temperature(uint16_t adc_temperature);
/*
 * @brief adc_wait_for_sync
 */
static inline void adc_wait_for_sync(void)
{
    while(ADC->STATUS.reg & ADC_STATUS_SYNCBUSY) {};
}
/*
 * @brief convert_ADC_to_temperature
 * @param[in] uint16_t ADC value
 * @return temperature scaled by 10 ie 254 = 24.4C
 */
static inline int32_t convert_ADC_to_temperature(uint16_t adc)
{
     return ((adc * 3125) - 7769600) / 3072;
}


static inline int32_t convert_ADC_to_voltage(uint16_t adc)
{
    return ((adc * (ADC_FULL_SCALE_MV * VOLTAGE_DIVIDER_INVERSE_RATIO)) / ADC_FULL_SCALE_COUNT);
}

#endif /* __ADC_H__ */
