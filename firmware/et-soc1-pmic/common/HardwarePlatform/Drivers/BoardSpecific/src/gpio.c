/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file gpio.c
    \brief C file for SAMD20 GPIO pins
*/

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "features.h"
#define USE_EIC_INTERRUPT

#include "iostructs.h"
#include "globals.h"
#include "gpio.h"
#include "gpiopins_penguin_revb.h"
#include "gpiopins_pcie_v3.h"
#include "gpiopins_bub_v2.h"
#include "hw_encoding.h"
#include "utils.h"
#include "ioxpander.h"

/***********************************************************************
 * Macros:  Defininition of local macros
***********************************************************************/
#define NUMIOPORTS PORT_GROUPS

// pin function
#define GPIO_PIN_FUNCTION_OFF 0xff /**< \brief PORT Function Off (GPIO Mode) */
#define GPIO_PIN_FUNCTION_A   0    /**< \brief PORT Function A (EIC) */
#define GPIO_PIN_FUNCTION_B   1    /**< \brief PORT Function B (Analog) */
#define GPIO_PIN_FUNCTION_C   2    /**< \brief PORT Function C (SERCOM) */
#define GPIO_PIN_FUNCTION_D   3    /**< \brief PORT Function D (SERCOM) */
#define GPIO_PIN_FUNCTION_E   4    /**< \brief PORT Function E (TC) */
#define GPIO_PIN_FUNCTION_F   5    /**< \brief PORT Function F (TCC) */
#define GPIO_PIN_FUNCTION_G   6    /**< \brief PORT Function G (TCC, PDEC) */
#define GPIO_PIN_FUNCTION_H   7    /**< \brief PORT Function H (QSPI, CAN, USB, CM4) */
#define GPIO_PIN_FUNCTION_I   8    /**< \brief PORT Function I (SDHC, CAN) */
#define GPIO_PIN_FUNCTION_J   9    /**< \brief PORT Function J (I2S) */
#define GPIO_PIN_FUNCTION_K   10   /**< \brief PORT Function K (PCC) */
#define GPIO_PIN_FUNCTION_L   11   /**< \brief PORT Function L (GMAC) */
#define GPIO_PIN_FUNCTION_M   12   /**< \brief PORT Function M (GCLK, AC) */
#define GPIO_PIN_FUNCTION_N   13   /**< \brief PORT Function N (CCL) */

#define NUM_OF_CONFIGURED_ANALOG_PINS 2
#define NUM_OF_AVAILABLE_ANALOG_PINS  20
#define NUM_OF_AVAILABLE_EIC_PINS     16

/***********************************************************************
 * GLOBAL variables:  Defininition of global variables and constants
***********************************************************************/
uint32_t isOD[NUMIOPORTS] = { 0, 0 }; // pins=bits, ports A and B

static uint32_t volatile rdyCnt = 0;
static uint32_t volatile ovrnCnt = 0;
static uint8_t volatile curInitOrderIdx = 0;

// AIN
static uint8_t const ainToPin[NUM_OF_AVAILABLE_ANALOG_PINS] = {
    PIN_PA02,
    PIN_PA03,
    PIN_PB08,
    PIN_PB09,
    PIN_PA04,
    PIN_PA05,
    PIN_PA06,
    PIN_PA07,
    PIN_PB00,
    PIN_PB01,
    PIN_PB02,
    PIN_PB03,
    PIN_PB04,
    PIN_PB05,
    PIN_PB05,
    PIN_PB07,
    PIN_PA08,
    PIN_PA09,
    PIN_PA10,
    PIN_PA11,
};
static void (*adcHandlerByInitOrder[NUM_OF_CONFIGURED_ANALOG_PINS])(uint16_t result);
static uint8_t ainByInitOrder[NUM_OF_CONFIGURED_ANALOG_PINS];
static uint8_t numActiveAnalogPins;

//EIC
// indices are eic num, pri/alt, port A/B -  values are portpins 0-31
static uint8_t const priPaEicPins[NUM_OF_AVAILABLE_EIC_PINS] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
static uint8_t const altPaEicPins[NUM_OF_AVAILABLE_EIC_PINS] = { 16, 17, 18, 19, 20, 21, 22, 23, 28, 99, 99, 99, 24, 25,
    99, 27 };
static uint8_t const *const paEicPins[2] = { priPaEicPins, altPaEicPins };
static uint8_t const priPbEicPins[NUM_OF_AVAILABLE_EIC_PINS] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
static uint8_t const altPbEicPins[NUM_OF_AVAILABLE_EIC_PINS] = { 16, 17, 99, 99, 99, 99, 22, 23, 99, 99, 99, 99, 99, 99,
    30, 31 };
static uint8_t const *const pbEicPins[2] = { priPbEicPins, altPbEicPins };
static uint8_t const *const *const eicToPins[2] = { paEicPins, pbEicPins };

void (*EicHandlerByEic[NUM_OF_AVAILABLE_EIC_PINS])(void);
uint8_t pinByEic[NUM_OF_AVAILABLE_EIC_PINS];

/***********************************************************************
 * Functions:   Declaration of local functions
***********************************************************************/
static void gpio_set_out_high(const uint8_t portpin);
static void gpio_set_out_low(const uint8_t portpin);
static void gpio_set_out_pin_id(gpioId_t id, bool state);
static bool gpio_get_OD(const uint8_t portpin);
static void gpio_set_mux(uint8_t portpin, uint8_t function);
static void gpio_set_config(uint8_t portpin, uint8_t cfg, uint8_t useMask);
static void gpio_set_drive(const uint8_t portpin, gpio_drive_t drive);
static void eic_set_CONFIG(uint8_t eicn, uint8_t mask);
static void eic_set_INTEN(uint32_t mask);
static void eic_write_NMICTRL(uint8_t data);
static void eic_clear_INTFLAG(uint32_t mask);
static uint32_t eic_read_INTFLAG(void);
static void eic_set_ENABLE(void);
static void gpio_define_output_pin(uint8_t *portpin, bool state, const gpio_pull_t pull, gpio_drive_t drive);
static void gpio_define_od_output_pin(uint8_t *portpin, bool state, const gpio_pull_t pull, gpio_drive_t drive);
static void gpio_define_input_pin(uint8_t *portpin, const gpio_pull_t pull);
static void gpio_define_pins(const gpioConfig_t *gpioCnfg, size_t elements);
static const gpioConfig_t *get_gpio_config(void);
static size_t get_gpio_config_size(void);
static const gpioPinIdMap_t *getGpioPinIdMap(void);
static size_t getGpioPinIdMapSize(void);

/***********************************************************************
 * GLOBAL Functions:   Defininition of global functions
***********************************************************************/
void gpio_set_OD(const uint8_t portpin, bool state)
{
    uint32_t t = (isOD[GPIO_PORT(portpin)] & ~(1U << GPIO_PIN(portpin)));
    isOD[GPIO_PORT(portpin)] = t | ((uint32_t)(state != 0) << GPIO_PIN(portpin));
}

void gpio_set_out(const uint8_t portpin, bool state)
{
    bool od = gpio_get_OD(portpin);
    if (state)
    {
        if (od)
        {
            gpio_set_dir_off(portpin);
        }
        gpio_set_out_high(portpin); // for pull up
    }
    else
    {
        gpio_set_out_low(portpin);
        if (od)
        {
            gpio_set_dir_out(portpin);
        }
    }
}

void gpio_set_dir_off(const uint8_t portpin)
{
    PORT->Group[GPIO_PORT(portpin)].DIRCLR.reg = 1U << GPIO_PIN(portpin); // set as input with inen disabled
    gpio_set_config(portpin, 0, PORT_PINCFG_MASK);                        // clear DRVSTR, PULLEN, INEN, PMUXEN
}

uint32_t gpio_read_dir(const uint8_t portpin)
{
    return (PORT->Group[GPIO_PORT(portpin)].DIR.reg >> GPIO_PIN(portpin)) & 0x01;
}

bool gpio_get_in(const uint8_t portpin)
{
    return (PORT->Group[GPIO_PORT(portpin)].IN.reg >> (portpin & 0x1F)) & 0x01;
}

uint8_t gpio_get_tristate(const uint8_t portpin)
{
    bool upst;
    bool downst;

    /* assume the pin dir has been configured as an input, which is the default */
    /* enable internal pull and input buffer */
    gpio_set_config(portpin, (PORT_PINCFG_INEN | PORT_PINCFG_PULLEN), (PORT_PINCFG_INEN | PORT_PINCFG_PULLEN));

    /* pull-up and read */
    gpio_set_out_high(portpin);
    upst = gpio_get_in(portpin);

    /* pull-down and read */
    gpio_set_out_low(portpin);
    downst = gpio_get_in(portpin);

    /* disable internal pull and input buffer */
    gpio_set_config(portpin, 0, (PORT_PINCFG_INEN | PORT_PINCFG_PULLEN));

    /* determine the pin state */
    if (upst && downst)
    {
        return _P;
    }
    else if (upst && !downst)
    {
        return _U;
    }
    else
    {
        return _G;
    }
}

bool gpio_get_out(const uint8_t portpin)
{
    return (PORT->Group[GPIO_PORT(portpin)].OUT.reg >> (portpin & 0x1F)) & 0x01;
}

uint8_t gpio_port_get_pin_function(const uint8_t portpin)
{
    uint8_t val;
    val = PORT->Group[GPIO_PORT(portpin)].PMUX[GPIO_PIN(portpin) >> 1].reg;
    if (GPIO_PIN(portpin) & 1)
    {
        return (val & PORT_PMUX_PMUXO_Msk) >> PORT_PMUX_PMUXO_Pos;
    }
    else
    {
        return (val & PORT_PMUX_PMUXE_Msk) >> PORT_PMUX_PMUXE_Pos;
    }
}

void gpio_set_out_low_pin_id(gpioId_t id)
{
    gpio_set_out_pin_id(id, 0);
}

void gpio_set_out_high_pin_id(gpioId_t id)
{
    gpio_set_out_pin_id(id, 1);
}

bool gpio_get_out_pin_id(gpioId_t id)
{
    uint8_t portpin = gpio_get_pin_from_id(id);
    assert1(portpin < 0xFF, "Wrong pin id value", id, portpin);

    if (portpin < PORT_BITS)
    {
        return gpio_get_out(portpin);
    }
    else
    {
        return ioxpander_get_out(portpin);
    }
}

bool gpio_get_in_pin_id(gpioId_t id)
{
    uint8_t portpin = gpio_get_pin_from_id(id);
    assert1(portpin < 0xFF, "Wrong pin id value", id, portpin);

    if (portpin < PORT_BITS)
    {
        return gpio_get_in(portpin);
    }
    else
    {
        return ioxpander_get_in_saved(portpin);
    }
}

void gpio_init(void)
{
    gpio_define_pins(get_gpio_config(), get_gpio_config_size());
}

void gpio_register_EIC_handler(gpioId_t pinId, EIC_handler_callback handler)
{
    uint8_t pin = gpio_get_pin_from_id(pinId);
    switch (Hw_Encoding_Get_Hw_Version_Encoding())
    {
        case HW_ENCODING_PCIE_1088: //Penguin RevB
            setEicHandler_penguin_revb(pin, handler);
            break;
        case HW_ENCODING_PCIE_V3_2: //PCIE v3.2
        case HW_ENCODING_PCIE_V3_3: //PCIE v3.3
            setEicHandler_pcie_v3(pin, handler);
            break;
        // case HW_ENCODING_BUB_REV_2: //BUB v2
        default: //assuming BUB as BUB hw encoding might not be unified
            setEicHandler_bub_v2(pin, handler);
            break;
    }
}

void gpio_register_ADC_handler(gpioId_t pinId, ADC_handler_callback handler)
{
    uint8_t pin = gpio_get_pin_from_id(pinId);
    switch (Hw_Encoding_Get_Hw_Version_Encoding())
    {
        case HW_ENCODING_PCIE_1088: //Penguin RevB
            setAdcHandler_penguin_revb(pin, handler);
            break;
        case HW_ENCODING_PCIE_V3_2: //PCIE v3.2
        case HW_ENCODING_PCIE_V3_3: //PCIE v3.3
            setAdcHandler_pcie_v3(pin, handler);
            break;
        // case HW_ENCODING_BUB_REV_2: //BUB v2
        default: //assuming BUB as BUB hw encoding might not be unified
            setAdcHandler_bub_v2(pin, handler);
            break;
    }
}

#ifdef USE_EIC_INTERRUPT
void EIC_Handler(void) // interrupt handler
{
    uint32_t flags = eic_read_INTFLAG();

    for (uint8_t i = 0; i < NUM_OF_AVAILABLE_EIC_PINS; ++i)
    {
        uint32_t mask = 1U << i;
        if (flags & mask)
        {
            void (*EicHandler)(void) = EicHandlerByEic[i];
            if (EicHandler)
            {
                (*EicHandler)();
            }
            eic_clear_INTFLAG(mask);
        }
    }
}
#endif

void ADC_Handler(void) // interrupt handler
{
    if (ADC->INTFLAG.reg & ADC_INTFLAG_RESRDY)
    {
        void (*adcHandler)(uint16_t result) = adcHandlerByInitOrder[curInitOrderIdx];
        uint16_t result = ADC->RESULT.reg;

        if (adcHandler)
        {
            (*adcHandler)(result);
        }

        if (++curInitOrderIdx >= NUM_OF_CONFIGURED_ANALOG_PINS)
            curInitOrderIdx = 0;

        ADC->INPUTCTRL.bit.MUXPOS = ainByInitOrder[curInitOrderIdx]; //other bit fields are 0
        ADC->SWTRIG.bit.START = 1;                                   //other bit fields are 0
        ++rdyCnt;
        ADC->INTFLAG.reg = ADC_INTENCLR_RESRDY; // Clear result ready interrupt
    }
    if (ADC->INTFLAG.reg & ADC_INTFLAG_OVERRUN)
    {
        ++ovrnCnt;
        ADC->INTFLAG.reg = ADC_INTENCLR_OVERRUN; // Clear overrun interrupt
    }
}

// modified not to change DRVSTR, PULLEN, INEN
void gpio_port_set_pin_function(const uint8_t portpin, const uint8_t pinfunction)
{
    gpio_set_config(portpin, -(pinfunction != GPIO_PIN_FUNCTION_OFF),
        PORT_PINCFG_PMUXEN); // set to PORT_PINCFG_PMUXEN if pinfunction!=GPIO_PIN_FUNCTION_OFF
    gpio_set_mux(portpin, pinfunction);
}

void gpio_set_dir_out(const uint8_t portpin)
{
    PORT->Group[GPIO_PORT(portpin)].DIRSET.reg = 1U << GPIO_PIN(portpin); // set as output pin
    gpio_set_config(portpin, 0, PORT_PINCFG_MASK);                        // clear DRVSTR, PULLEN, INEN, PMUXEN
}

void gpio_set_dir_in(const uint8_t portpin)
{
    PORT->Group[GPIO_PORT(portpin)].DIRCLR.reg = 1U << GPIO_PIN(portpin); // set as input pin
    gpio_set_config(portpin, PORT_PINCFG_INEN, PORT_PINCFG_MASK);         // set INEN, clear DRVSTR, PULLEN, PMUXEN
}

void gpio_set_pull(const uint8_t portpin, const gpio_pull_t pull)
{
    gpio_set_config(portpin, -(pull != GPIO_PULL_OFF), PORT_PINCFG_PULLEN); // 0->0, 1->0xFF
    if (pull == GPIO_PULL_UP)
    {
        gpio_set_out_high(portpin);
    }
    else if (pull == GPIO_PULL_DOWN)
    {
        gpio_set_out_low(portpin);
    }
}

uint8_t gpio_initial_Ain(void)
{
    return ainByInitOrder[0];
}

uint8_t gpio_get_pg_pin_from_en(gpioId_t enablePinId)
{
    uint8_t pgPin = 0xFF;

    switch (Hw_Encoding_Get_Hw_Version_Encoding())
    {
        case HW_ENCODING_PCIE_1088: //Penguin RevB
            pgPin = pgPinFromEnPinId_penguin_revb(enablePinId);
            break;
        case HW_ENCODING_PCIE_V3_2: //PCIE v3.2
        case HW_ENCODING_PCIE_V3_3: //PCIE v3.3
            assert1(0, "Not supported on hw version", 0, 0);
            break;
        // case HW_ENCODING_BUB_REV_2: //BUB v2
        default: //assuming BUB as BUB hw encoding might not be unified
            pgPin = pgPinFromEnPinId_bub_v2(enablePinId);
            break;
    }
    return pgPin;
}

gpioId_t gpio_get_id_from_pin(uint8_t pin)
{
    gpioId_t pinId = 0xFF;
    const gpioPinIdMap_t *gpioPinIdMap = getGpioPinIdMap();
    const size_t gpioPinIdMapSize = getGpioPinIdMapSize();

    for (uint32_t i = 0; i < gpioPinIdMapSize; i++)
    {
        if (gpioPinIdMap[i].pin == pin)
        {
            pinId = gpioPinIdMap[i].id;
            break;
        }
    }
    return pinId;
}

uint8_t gpio_get_pin_from_id(gpioId_t pinId)
{
    uint8_t pin = 0xFF;
    const gpioPinIdMap_t *gpioPinIdMap = getGpioPinIdMap();
    const size_t gpioPinIdMapSize = getGpioPinIdMapSize();

    for (uint32_t i = 0; i < gpioPinIdMapSize; i++)
    {
        if (gpioPinIdMap[i].id == pinId)
        {
            pin = gpioPinIdMap[i].pin;
            break;
        }
    }
    return pin;
}

/***********************************************************************
 * Functions:   Definition of local functions
***********************************************************************/
static void gpio_set_out_high(const uint8_t portpin)
{
    PORT->Group[GPIO_PORT(portpin)].OUTSET.reg = 1U << GPIO_PIN(portpin);
}

static void gpio_set_out_low(const uint8_t portpin)
{
    PORT->Group[GPIO_PORT(portpin)].OUTCLR.reg = 1U << GPIO_PIN(portpin);
}

static void gpio_set_out_pin_id(gpioId_t id, bool state)
{
    uint8_t portpin = gpio_get_pin_from_id(id);
    assert1(portpin < 0xFF, "Wrong pin id value", id, portpin);

    if (portpin < PORT_BITS)
    {
        gpio_set_out(portpin, state);
    }
    else
    {
        ioxpander_set_out(portpin, state);
    }
}

static void gpio_set_mux(uint8_t portpin, uint8_t function)
{
    uint8_t volatile *pmux = &PORT->Group[GPIO_PORT(portpin)].PMUX[GPIO_PIN(portpin) >> 1].reg;
    function &= 0x0F;
    if ((portpin & 0x01) == 0)
    {
        *pmux = (*pmux & 0xF0) | function;
    }
    else
    {
        *pmux = (uint8_t)((*pmux & 0xF) | (function << 4));
    }
}

static bool gpio_get_OD(const uint8_t portpin)
{
    return (isOD[GPIO_PORT(portpin)] >> GPIO_PIN(portpin)) & 0x01;
}

static void gpio_set_config(uint8_t portpin, uint8_t cfg, uint8_t useMask)
{
    uint8_t volatile *pcfg = &PORT->Group[GPIO_PORT(portpin)].PINCFG[GPIO_PIN(portpin)].reg;
    cfg &= PORT_PINCFG_MASK;
    *pcfg = (*pcfg & ~useMask) | (cfg & useMask);
}

static void gpio_set_drive(const uint8_t portpin, gpio_drive_t drive)
{
    gpio_set_config(portpin, -(drive == HIGH_DRIVE), PORT_PINCFG_DRVSTR); // 0->0, 1->0xFF
}

static void eic_set_CONFIG(uint8_t eicn, uint8_t mask)
{
    uint32_t tmp;
    uint8_t index = eicn >> 3;
    uint8_t pos = (uint8_t)((eicn & 7) << 2);
    tmp = EIC->CONFIG[index].reg & ~((uint32_t)(0x0000000F << pos));
    EIC->CONFIG[index].reg = tmp | ((mask & 0x0F) << pos);
}

static void eic_set_INTEN(uint32_t mask)
{
    EIC->INTENSET.reg = mask;
}

static void eic_write_NMICTRL(uint8_t data)
{
    EIC->NMICTRL.reg = data;
}

static void eic_clear_INTFLAG(uint32_t mask)
{
    EIC->INTFLAG.reg = mask;
}

static uint32_t eic_read_INTFLAG(void)
{
    return EIC->INTFLAG.reg;
}

/* set ENABLE bit enabling the EIC module
 * this must be synchronized with
 * eic_wait_for_sync(EIC_SYNCBUSY_ENABLE) */
static void eic_set_ENABLE(void)
{
    EIC->CTRL.reg |= EIC_CTRL_ENABLE;
}

static void gpio_define_output_pin(uint8_t *portpin, bool state, const gpio_pull_t pull, gpio_drive_t drive)
{
    gpio_set_out(*portpin, state == ACTIVE_LOW); // set inactive
    gpio_set_dir_out(*portpin);
    gpio_set_pull(*portpin, pull);
    gpio_set_drive(*portpin, drive);
}

static void gpio_define_od_output_pin(uint8_t *portpin, bool state, const gpio_pull_t pull, gpio_drive_t drive)
{
    gpio_set_OD(*portpin, 1);
    if (pull == GPIO_PULL_UP)
    {
        PORT->Group[GPIO_PORT(*portpin)].PINCFG[GPIO_PIN(*portpin)].reg |= PORT_PINCFG_PULLEN;
    }
    else
    {
        PORT->Group[GPIO_PORT(*portpin)].PINCFG[GPIO_PIN(*portpin)].reg &= ~PORT_PINCFG_PULLEN; // ignore pull down
    }
    if (state == ACTIVE_LOW)
    {                                // set inactive
        gpio_set_dir_off(*portpin);  // active low = wired NOR
        gpio_set_out_high(*portpin); // for possible pull up
    }
    else
    {
        gpio_set_out_low(*portpin); // active high = wired AND
        gpio_set_dir_out(*portpin);
    }
    gpio_set_drive(*portpin, drive);
}

static void gpio_define_input_pin(uint8_t *portpin, const gpio_pull_t pull)
{
    gpio_set_dir_in(*portpin);
    gpio_set_pull(*portpin, pull);
}

static void gpio_define_pins(const gpioConfig_t *gpioCnfg, size_t elements)
{
    uint32_t eicBitmap = 0;
    numActiveAnalogPins = 0;
    memset(EicHandlerByEic, 0x00, sizeof(EicHandlerByEic));
    memset(pinByEic, 0xFF, sizeof(pinByEic));

    EIC->CTRL.reg = EIC_CTRL_SWRST;
    while (EIC->CTRL.reg & EIC_CTRL_SWRST)
        ;
    for (size_t i = 0; i < elements; i++)
    {
        uint8_t pin = gpioCnfg[i].pin;
        uint8_t pinPort = GPIO_PORT(pin);
        uint8_t pinIdx = GPIO_PIN(pin);

        assert1(pin < PORT_BITS, "Pin value is incorrect", pin, 0);

        gpio_set_OD(pin, 0); // clear bit, set it below if OD

        switch (gpioCnfg[i].type)
        {
            case GPIO_OUTPUT:
                gpio_define_output_pin(&pin, gpioCnfg[i].state, gpioCnfg[i].pull, gpioCnfg[i].drive);
                break;
            case GPIO_OD_OUTPUT:
                gpio_define_od_output_pin(&pin, gpioCnfg[i].state, gpioCnfg[i].pull, gpioCnfg[i].drive);
                break;
            case GPIO_INPUT:
                gpio_define_input_pin(&pin, gpioCnfg[i].pull);
                break;
            case ANALOG_INPUT:
                assert1(numActiveAnalogPins < NUM_OF_CONFIGURED_ANALOG_PINS, "increase NUM_OF_CONFIGURED_ANALOG_PINS",
                    0, 0);
                for (uint8_t k = 0; k < NUM_OF_AVAILABLE_ANALOG_PINS; ++k)
                {
                    if (ainToPin[k] == pin)
                    {
                        assert1(gpioCnfg[i].adcHandler, "ADC handler not specified", pin, 0);
                        ainByInitOrder[numActiveAnalogPins] = k;
                        adcHandlerByInitOrder[numActiveAnalogPins] = gpioCnfg[i].adcHandler;
                        numActiveAnalogPins++;
                        break;
                    }
                    assert1(k < NUM_OF_AVAILABLE_ANALOG_PINS, "specified pin not analog capable", k, 0);
                    gpio_port_set_pin_function(pin, GPIO_PIN_FUNCTION_B);
                }
                break;
            case EIC_INPUT:
            {
                uint8_t eic = 0xFF;
                for (int teic = 0; teic < NUM_OF_AVAILABLE_EIC_PINS; ++teic)
                {
                    for (int k = 0; k < 2; ++k)
                    {
                        if (eicToPins[pinPort][k][teic] == pinIdx)
                        {
                            assert1(pinByEic[teic] == 0xFF, "this EIC already in use", pin, pinByEic[teic]);
                            eic = (uint8_t)teic;
                            assert1(gpioCnfg[i].eicHandler, "EIC handler not specified", pin, 0);
                            EicHandlerByEic[eic] = gpioCnfg[i].eicHandler;
                            pinByEic[eic] = pin;
                        }
                    }
                }
                assert1(eic != 0xFF, "pin is not EIC capable", pin, 0);

                gpio_set_dir_in(pin); // this clears PMUXEN - gpio_port_set_pin_function must come after
                gpio_set_pull(pin, gpioCnfg[i].pull);
                gpio_port_set_pin_function(pin, GPIO_PIN_FUNCTION_A);

                eic_set_CONFIG(eic, gpioCnfg[i].sense);
                eicBitmap |= (1 << eic);
            }
            break;
            case NMI_INPUT:
                gpio_port_set_pin_function(pin, GPIO_PIN_FUNCTION_A);
                eic_write_NMICTRL(gpioCnfg[i].sense);
                break;
            default:
                break;
        }
    }
    eic_set_ENABLE();

#ifdef USE_EIC_INTERRUPT
    if (eicBitmap != 0)
    {
        eic_clear_INTFLAG(eicBitmap);
        eic_set_INTEN(eicBitmap);
        NVIC_DisableIRQ(EIC_IRQn);
    }
#endif
}

static const gpioConfig_t *get_gpio_config(void)
{
    const gpioConfig_t *gpioConfig = NULL;
    switch (Hw_Encoding_Get_Hw_Version_Encoding())
    {
        case HW_ENCODING_PCIE_1088: //Penguin RevB
            gpioConfig = getGpioConfig_penguin_revb();
            break;
        case HW_ENCODING_PCIE_V3_2: //PCIE v3.2
        case HW_ENCODING_PCIE_V3_3: //PCIE v3.3
            gpioConfig = getGpioConfig_pcie_v3();
            break;
        // case HW_ENCODING_BUB_REV_2: //BUB v2
        default: //assuming BUB as BUB hw encoding might not be unified
            gpioConfig = getGpioConfig_bub_v2();
            break;
    }
    return gpioConfig;
}

static size_t get_gpio_config_size(void)
{
    size_t size;
    switch (Hw_Encoding_Get_Hw_Version_Encoding())
    {
        case HW_ENCODING_PCIE_1088: //Penguin RevB
            size = getGpioConfigSize_penguin_revb();
            break;
        case HW_ENCODING_PCIE_V3_2: //PCIE v3.2
        case HW_ENCODING_PCIE_V3_3: //PCIE v3.3
            size = getGpioConfigSize_pcie_v3();
            break;
        // case HW_ENCODING_BUB_REV_2: //BUB v2
        default: //assuming BUB as BUB hw encoding might not be unified
            size = getGpioConfigSize_bub_v2();
            break;
    }
    return size;
}

static const gpioPinIdMap_t *getGpioPinIdMap(void)
{
    const gpioPinIdMap_t *gpioPinIdMap = NULL;
    switch (Hw_Encoding_Get_Hw_Version_Encoding())
    {
        case HW_ENCODING_PCIE_1088: //Penguin RevB
            gpioPinIdMap = getGpioPinIdMap_penguin_revb();
            break;
        case HW_ENCODING_PCIE_V3_2: //PCIE v3.2
        case HW_ENCODING_PCIE_V3_3: //PCIE v3.3
            gpioPinIdMap = getGpioPinIdMap_pcie_v3();
            break;
        // case HW_ENCODING_BUB_REV_2: //BUB v2
        default: //assuming BUB as BUB hw encoding might not be unified
            gpioPinIdMap = getGpioPinIdMap_bub_v2();
            break;
    }
    return gpioPinIdMap;
}

static size_t getGpioPinIdMapSize(void)
{
    size_t size;
    switch (Hw_Encoding_Get_Hw_Version_Encoding())
    {
        case HW_ENCODING_PCIE_1088: //Penguin RevB
            size = getGpioPinIdMapSize_penguin_revb();
            break;
        case HW_ENCODING_PCIE_V3_2: //PCIE v3.2
        case HW_ENCODING_PCIE_V3_3: //PCIE v3.3
            size = getGpioPinIdMapSize_pcie_v3();
            break;
        // case HW_ENCODING_BUB_REV_2: //BUB v2
        default: //assuming BUB as BUB hw encoding might not be unified
            size = getGpioPinIdMapSize_bub_v2();
            break;
    }
    return size;
}
