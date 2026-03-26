/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file gpio.h
    \brief GPIO (PORT)
    \note This file is modified from the Atmel ASF4 hri include
          to work with ATSAMD20 processor by Alkgrove and Bob Alkire.
          
          GPIO drivers are pin oriented and as such set up as bits.
          As such, it has more sets and gets of bits.
*/
/***********************************************************************/

#ifndef __GPIO_H__
#define __GPIO_H__

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <compiler.h>

/***********************************************************************
 * GLOBAL Macros:  Defininition of global macros
***********************************************************************/
/**
 * @brief Macros for the pin and port group, lower 5
 * bits are pin number in the group, higher 3
 * bits are the port group A, B etc
 */
#define GPIO_PIN(n)     ((n)&0x1Fu)
#define GPIO_PORT(n)    (((n)&0xFFu) >> 5)
#define GPIO(port, pin) ((((port)&0x7u) << 5) + ((pin)&0x1Fu))

//Pins used for hw encoding
#define BRDID_0 PIN_PB13
#define BRDID_1 PIN_PB14
#define BRDID_2 PIN_PB15

#define EXPANDER_0  (PORT_BITS) // start after the 64 Atmel GPIOs PA00=0 ... PB31=63
#define EXPANDER_1  (EXPANDER_0 + 16)
#define EXPANDER_P0 (0)
#define EXPANDER_P1 (8)

/***********************************************************************
 * GLOBAL Data types:      Defininition of global data types
***********************************************************************/
typedef enum {
    GPIO_INPUT,
    ANALOG_INPUT,
    GPIO_OUTPUT,
    GPIO_OD_OUTPUT,
    EIC_INPUT,
    NMI_INPUT
} gpio_type_t;

typedef enum { GPIO_PULL_OFF = 0, GPIO_PULL_UP, GPIO_PULL_DOWN } gpio_pull_t;

typedef enum { NORMAL_DRIVE = 0, HIGH_DRIVE } gpio_drive_t;

typedef enum { GPIO_DIRECTION_OFF = 0, GPIO_DIRECTION_IN, GPIO_DIRECTION_OUT } gpio_direction_t;

typedef enum { ACTIVE_LOW = 0, ACTIVE_HIGH } gpio_active_state_t;

typedef enum { PORTA = 0, PORTB, PORTC, PORTD, PORTE } portgroup_t;

typedef enum {
    EIC_CONFIG_SENSE_NONE = 0,
    EIC_CONFIG_SENSE_RISE,
    EIC_CONFIG_SENSE_FALL,
    EIC_CONFIG_SENSE_BOTH,
    EIC_CONFIG_SENSE_HIGH,
    EIC_CONFIG_SENSE_LOW,
    EIC_CONFIG_FILTEN /*this to be ored*/
} eic_config_t;

typedef enum {
    SOC_RST_OUT = 0,
    PMIC_RDY_OUT,
    SOC_PERST_OUT,
    PMIC_INT_OUT,
    MNN_NOC_PWR_RST_OUT,
    NOC_EN_OUT,
    MNN_EN_OUT,
    MXN_EN_OUT,
    QLP_EN_OUT,
    SRAM_EN_OUT,
    LOGIC_EN_OUT,
    DDR_EN_OUT,
    Q_EN_OUT,
    PCIE_EN_OUT,
    VDD_1P8V_EN_OUT,
    VDDA_1P8V_EN_OUT,
    VDD_3P3V_EN_OUT,
    MCU_GPIO0_OUT,
    MCU_GPIO1_OUT,
    MCU_GPIO2_OUT,
    MCU_GPIO3_OUT,
    MCU_GPIO4_OUT,
    MCU_GPIO5_OUT,
    MCU_GPIO7_OUT,
    MCU_GPIO8_OUT,
    MCU_GPIO9_OUT,
    MCU_GPIO10_OUT,
    MCU_GPIO_PAD0_OUT,
    MCU_GPIO_PAD1_OUT,
    MCU_GPIO_PAD2_OUT,
    MCU_GPIO_PAD3_OUT,
    MEZ_GPIO0_OUT,
    MEZ_GPIO1_OUT,
    MEZ_GPIO2_OUT,
    MEZ_GPIO3_OUT,
    MEZ_GPIO4_OUT,
    MEZ_GPIO5_OUT,
    MEZ_CONN_ALERT_OUT,
    LED_OUT,
    IOXP_0_RST_OUT,
    IOXP_1_RST_OUT,
    PERST_IN,
    WDT_RST_IN,
    TEMP_ALERT_IN,
    MNN_NOC_ALERT_IN,
    POWER_GOOD_12V_IN,
    POWER_GOOD_3P3V_IN,
    POWER_GOOD_BUS0_IN,
    POWER_GOOD_BUS1_IN,
    IOXP_0_IN,
    IOXP_1_IN,
    POWER_GOOD_MNN_IN,
    POWER_GOOD_NOC_IN,
    POWER_GOOD_DDR_IN,
    POWER_GOOD_MXN_IN,
    POWER_GOOD_QLP_IN,
    POWER_GOOD_Q_IN,
    POWER_GOOD_LOGIC_IN,
    POWER_GOOD_PCIE_IN,
    POWER_GOOD_1P8_IN,
    POWER_GOOD_SRAM_IN,
    SRAM_ALERT_IN,
    SRAM_FAULT_IN,
    SRAM_EN_SENSE_IN,
    VMON_12V_AIN,
    IMON_12V_AIN,
} gpioId_t;

typedef void (*EIC_handler_callback)(void);
typedef void (*ADC_handler_callback)(uint16_t result);

//Pin ID to pin map structure
typedef struct {
    gpioId_t id;
    uint8_t pin;
} gpioPinIdMap_t;

//group config structure
typedef struct {
    uint8_t pin;
    gpio_type_t type;
    gpio_active_state_t state; // inp / out
    gpio_pull_t pull;          // relevant for input pins
    gpio_drive_t drive;        // relevant for output pins
    eic_config_t sense;        // relevant for EIC pins
    union {
        EIC_handler_callback eicHandler;
        ADC_handler_callback adcHandler;
    };

} gpioConfig_t;

enum {
    GPIO0_P0_0 = EXPANDER_0 + EXPANDER_P0,
    GPIO0_P0_1,
    GPIO0_P0_2,
    GPIO0_P0_3,
    GPIO0_P0_4,
    GPIO0_P0_5,
    GPIO0_P0_6,
    GPIO0_P0_7,
};
enum {
    GPIO0_P1_0 = EXPANDER_0 + EXPANDER_P1,
    GPIO0_P1_1,
    GPIO0_P1_2,
    GPIO0_P1_3,
    GPIO0_P1_4,
    GPIO0_P1_5,
    GPIO0_P1_6,
    GPIO0_P1_7,
};
enum {
    GPIO1_P0_0 = EXPANDER_1 + EXPANDER_P0,
    GPIO1_P0_1,
    GPIO1_P0_2,
    GPIO1_P0_3,
    GPIO1_P0_4,
    GPIO1_P0_5,
    GPIO1_P0_6,
    GPIO1_P0_7,
};
enum {
    GPIO1_P1_0 = EXPANDER_1 + EXPANDER_P1,
    GPIO1_P1_1,
    GPIO1_P1_2,
    GPIO1_P1_3,
    GPIO1_P1_4,
    GPIO1_P1_5,
    GPIO1_P1_6,
    GPIO1_P1_7,
};

/***********************************************************************
 * GLOBAL Functions:   Declaration of global functions
***********************************************************************/
/**
 * @brief Set pin direction as an output for port/pin
 * it sets output drive to normal and pin function to GPIO
 * call gpio_set_high_drive() to change drive and gpio_port_set_pin_function() to change function
 * @param[in] portpin - uint8_t  Pin number is bits 0..4 and Port is bits 5..7
 */
void gpio_set_dir_out(const uint8_t portpin);

/**
 * @brief Set pin direction as an input on port/pin
 * it sets pin function to GPIO and no pullup or pulldown
 * @param[in] portpin - uint8_t  Pin number is bits 0..4 and Port is bits 5..7
 */
void gpio_set_dir_in(const uint8_t portpin);

/**
 * @brief Set pin direction as an off on port/pin
 * GPIO for this port/pin is turned off and is hi-z
 * @param[in] portpin - uint8_t  Pin number is bits 0..4 and Port is bits 5..7
 */
void gpio_set_dir_off(const uint8_t portpin);

/**
 * @brief Get pin direction on PORTA, PORTB
 *
 * @param[in] portpin - uint8_t  Pin number is bits 0..4 and Port is bits 5..7
 *                      PORTA = group 0, PORTB = group 1, etc
 * @return uint32_t direction '1' is output, '0' is input
 */
uint32_t gpio_read_dir(const uint8_t portpin);

/**
 * @brief Set pin function on port/pin
 * @param[in] portpin - uint8_t  Pin number is bits 0..4 and Port is bits 5..7
 * @param[in] pinfunction - uint8_t  GPIO_PIN_FUNCTION_OFF, GPIO_PIN_FUNCTION_A to GPIO_PIN_FUNCTION_N
 * @note also can be from includes/pio defines ie MUX_PA04D_SERCOM0_PAD0
 * @note modified not to change DRVSTR, PULLEN, INEN
 */
// modified not to change DRVSTR, PULLEN, INEN
void gpio_port_set_pin_function(const uint8_t portpin, const uint8_t pinfunction);

/**
 * @brief Set OD gpio pin to state on port/pin
 *
 * @param[in] portpin - uint8_t  Pin number is bits 0..4 and Port is bits 5..7
 * @param[in] state - bool  true = high, false = low
 */
void gpio_set_OD(const uint8_t portpin, bool state);

/**
 * @brief Set output gpio pin to state on port/pin
 *
 * @param[in] portpin - uint8_t  Pin number is bits 0..4 and Port is bits 5..7
 * @param[in] state - bool  true = high, false = low
 */
void gpio_set_out(const uint8_t portpin, bool state);

/**
 * @brief get gpio pin input on port/pin
 *
 * @param[in] portpin - uint8_t  Pin number is bits 0..4 and Port is bits 5..7
 * @return bool true if pin was high false if pin was low
 */
bool gpio_get_in(const uint8_t portpin);

enum {
    _G = 0, // Grounded
    _P,     // Pulled-up
    _U      // Unconnected
};
/**
 * @brief get gpio tri-state pin input on port/pin
 *
 * @param[in] portpin - uint8_t  Pin number is bits 0..4 and Port is bits 5..7
 * @return uint8_t 1 if pin was high 0 if pin was low 2 if pin was floating
 */
uint8_t gpio_get_tristate(const uint8_t portpin);

/**
 * @brief get gpio pin output state on port/pin
 *
 * @param[in] portpin - uint8_t  Pin number is bits 0..4 and Port is bits 5..7
 * @return bool true if pin was high false if pin was low
 */
bool gpio_get_out(const uint8_t portpin);

/**
 * @brief get gpio function on port/pin
 *
 * @param[in] portpin - uint8_t  Pin number is bits 0..4 and Port is bits 5..7
 * @return uint8_t pmux register for port/pin
 */
uint8_t gpio_port_get_pin_function(const uint8_t portpin);

/**
 * @brief set gpio pull on port/pin
 *
 * @param[in] portpin - uint8_t  Pin number is bits 0..4 and Port is bits 5..7
 * @param[in] pull - gpio_pull_t  pull-up. pull-down, or pull-off
 * @return uint8_t pmux register for port/pin
 */
void gpio_set_pull(const uint8_t portpin, const gpio_pull_t pull);

/**
 * @brief gpio_init - initialize GPIO input, output and EIC from table
 */
void gpio_init(void);

/**
 * @brief Get initial analog input pin number
 *
 * @return uint8_t pin number set as analog input
 */
uint8_t gpio_initial_Ain(void);

/**
 * @brief Register EIC interrupt handler
 *
 * @param[in] pinId - gpioId_t  pin id
 * @param[in] EIC_handler_callback - callback function
 */
void gpio_register_EIC_handler(gpioId_t pinId, EIC_handler_callback handler);

/**
 * @brief Register ADC interrupt handler
 *
 * @param[in] pinId - gpioId_t  pin id
 * @param[in] ADC_handler_callback - callback function
 */
void gpio_register_ADC_handler(gpioId_t pinId, ADC_handler_callback handler);

/**
 * @brief Set output gpio pin id
 *
 * @param[in] id - gpioId_t  pin id
 */
void gpio_set_out_high_pin_id(gpioId_t id);

/**
 * @brief Clear output gpio pin id
 *
 * @param[in] id - gpioId_t  pin id
 */
void gpio_set_out_low_pin_id(gpioId_t id);

/**
 * @brief Get state of output gpio pin id
 *
 * @param[in] id - gpioId_t  pin id
 * @return bool state
 */
bool gpio_get_out_pin_id(gpioId_t id);

/**
 * @brief Get state of input gpio pin id
 *
 * @param[in] id - gpioId_t  pin id
 * @return bool state
 */
bool gpio_get_in_pin_id(gpioId_t id);

/**
 * @brief get regulator power good gpio pin number from regulator enable pin id
 *
 * @param[in] enablePinId - gpioId_t  pin id
 * @return gpioId_t pin id
 */
uint8_t gpio_get_pg_pin_from_en(gpioId_t enablePinId);

/**
 * @brief get gpio id from pin number
 *
 * @param[in] pin - uint8_t  pin number
 * @return gpioId_t pin id
 */
gpioId_t gpio_get_id_from_pin(uint8_t pin);

/**
 * @brief get gpio pin number from pin id
 *
 * @param[in] pin - gpioId_t pin id
 * @return uint8_t  pin number
 */
uint8_t gpio_get_pin_from_id(gpioId_t pinId);

#endif //__GPIO_H__
