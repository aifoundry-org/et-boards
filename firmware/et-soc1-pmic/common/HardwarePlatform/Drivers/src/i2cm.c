/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file i2cm.c
    \brief C file for SAMD20 SPI drivers for PMIC Test Fixture
*/

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "board_defs.h"

#include "FreeRTOS.h"
#include "stream_buffer.h"
#include "task.h"
#include "semphr.h"
#include "globals.h"

#include "boardchipinfo.h"
#include "utils.h"
#include "i2cm.h"
#include "gpio.h"

//#define I2CMTRACE /* uncomment for trace functionality */

//  trace tool
#ifdef I2CMTRACE
#define TRACE printfFromInterruptNoFlush("%3d %08X", __LINE__, (pI2cm->rxlen << 16) | (status << 8) | flag);

#else
#define TRACE
#endif

#define TXBUFFER_LENGTH 16
#define RXBUFFER_LENGTH 16

// I2C Commands
#define I2CM_CMD_NOP   (0x00ul << SERCOM_I2CM_CTRLB_CMD_Pos)
#define I2CM_CMD_RS    (0x01ul << SERCOM_I2CM_CTRLB_CMD_Pos)
#define I2CM_CMD_RDACK (0x02ul << SERCOM_I2CM_CTRLB_CMD_Pos)
#define I2CM_CMD_STOP  (0x03ul << SERCOM_I2CM_CTRLB_CMD_Pos)
// I2C Busstate
#define I2CM_BUSSTATE_UNKNOWN (0x00ul << SERCOM_I2CM_STATUS_BUSSTATE_Pos)
#define I2CM_BUSSTATE_IDLE    (0x01ul << SERCOM_I2CM_STATUS_BUSSTATE_Pos)
#define I2CM_BUSSTATE_OWNER   (0x02ul << SERCOM_I2CM_STATUS_BUSSTATE_Pos)
#define I2CM_BUSSTATE_BUSY    (0x03ul << SERCOM_I2CM_STATUS_BUSSTATE_Pos)
// I2C address protocol least significant bit indicates direction
#define I2C_RD 1
#define I2C_WR 0

typedef struct {
    Sercom *sercomDev;
    SemaphoreHandle_t protectionSem;
    SemaphoreHandle_t pageRegisterProtectionSem;
    SemaphoreHandle_t timerReleaseSem;
    uint32_t txlen;
    uint32_t rxlen;
    uint8_t *tx;
    uint8_t *rx;
    uint8_t address;
    uint8_t sdaPin, sclPin;
    i2cmErr_t status;
    bool ready;
} i2cmMsg_t;

static i2cmMsg_t gblI2cmInfo[NUM_I2CM];

void i2cm_clear_bus(i2cmMsg_t *pI2cm);

int i2cmInit(void)
{
    i2cmMsg_t *pI2cm; // temp pointer to structure for either I2C port used by interrupt hander common to both ports

    static StaticSemaphore_t i2cm1ProtectionSemBuffer;
    static StaticSemaphore_t i2cm1PageRegisterProtectionSemBuffer;
    static StaticSemaphore_t i2cm1TimerReleaseBuffer;
    pI2cm = &gblI2cmInfo[0]; // assign members to gblI2cmInfo[0]
    pI2cm->sercomDev = I2CM1_SERCOM;
    pI2cm->sdaPin = PSBUS0_SDA;
    pI2cm->sclPin = PSBUS0_SCL;
    pI2cm->protectionSem = xSemaphoreCreateBinaryStatic(&i2cm1ProtectionSemBuffer);
    xSemaphoreGive(pI2cm->protectionSem);
    pI2cm->pageRegisterProtectionSem = xSemaphoreCreateBinaryStatic(&i2cm1PageRegisterProtectionSemBuffer);
    xSemaphoreGive(pI2cm->pageRegisterProtectionSem);
    pI2cm->timerReleaseSem = xSemaphoreCreateBinaryStatic(&i2cm1TimerReleaseBuffer);
    NVIC_DisableIRQ(I2CM1_IRQn);
    NVIC_ClearPendingIRQ(I2CM1_IRQn);
    NVIC_EnableIRQ(I2CM1_IRQn);

    static StaticSemaphore_t i2cm2ProtectionSemBuffer;
    static StaticSemaphore_t i2cm2PageRegisterProtectionSemBuffer;
    static StaticSemaphore_t i2cm2TimerReleaseBuffer;
    pI2cm = &gblI2cmInfo[1]; // assign members to gblI2cmInfo[1]
    pI2cm->sercomDev = I2CM2_SERCOM;
    pI2cm->sdaPin = PSBUS1_SDA;
    pI2cm->sclPin = PSBUS1_SCL;
    pI2cm->protectionSem = xSemaphoreCreateBinaryStatic(&i2cm2ProtectionSemBuffer);
    xSemaphoreGive(pI2cm->protectionSem);
    pI2cm->pageRegisterProtectionSem = xSemaphoreCreateBinaryStatic(&i2cm2PageRegisterProtectionSemBuffer);
    xSemaphoreGive(pI2cm->pageRegisterProtectionSem);
    pI2cm->timerReleaseSem = xSemaphoreCreateBinaryStatic(&i2cm2TimerReleaseBuffer);
    NVIC_DisableIRQ(I2CM2_IRQn);
    NVIC_ClearPendingIRQ(I2CM2_IRQn);
    NVIC_EnableIRQ(I2CM2_IRQn);

    return STATUS_SUCCESS;
}

// declarations of interrupt handler common to both ports
void I2CM_Handler(i2cmMsg_t *pI2cm);

// declarations of actual i2cm1 interrupt handler
void I2CM1_Handler(void);
// actual i2cm1 interrupt handler which call the common handlers
void I2CM1_Handler(void)
{
    I2CM_Handler(&gblI2cmInfo[0]);
}

// declarations of actual i2cm2 interrupt handler
void I2CM2_Handler(void);
// actual i2cm2 interrupt handler which call the common handler
void I2CM2_Handler(void)
{
    I2CM_Handler(&gblI2cmInfo[1]);
}

// i2c interrupt handler common to both ports
void I2CM_Handler(i2cmMsg_t *pI2cm)
{
    Sercom *sercomDev = pI2cm->sercomDev;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    uint8_t flag = sercomDev->I2CM.INTFLAG.reg;
    uint16_t status = sercomDev->I2CM.STATUS.reg;

    // master on bus handler?
    if (flag & SERCOM_I2CM_INTFLAG_MB)
    { // byte from master
        TRACE
        if (status & (SERCOM_I2CM_STATUS_ARBLOST | SERCOM_I2CM_STATUS_BUSERR))
        {
            sercomDev->I2CM.STATUS.reg = SERCOM_I2CM_STATUS_ARBLOST | SERCOM_I2CM_STATUS_BUSERR; // clear error
            pI2cm->status = I2CM_BUSFAULT;
            pI2cm->ready = true;
            pI2cm->rxlen = 0;
            xSemaphoreGiveFromISR(pI2cm->timerReleaseSem, &xHigherPriorityTaskWoken);
            // send bus fail error to task here instead
            TRACE
        }
        else if (status & SERCOM_I2CM_STATUS_RXNACK)
        {
            TRACE
            // check for NACK from slave
            pI2cm->status = I2CM_NO_ACK;
            pI2cm->ready = true;
            pI2cm->rxlen = 0;
            xSemaphoreGiveFromISR(pI2cm->timerReleaseSem, &xHigherPriorityTaskWoken);
            sercomDev->I2CM.CTRLB.reg |= I2CM_CMD_STOP; // clears MB and issues stop
            i2cm_wait_for_sync(sercomDev);
        }
        else if (pI2cm->txlen > 0)
        {
            TRACE
            sercomDev->I2CM.DATA.reg = *pI2cm->tx++;
            i2cm_wait_for_sync(sercomDev);
            pI2cm->txlen--;
        }
        else
        {
            if (pI2cm->rxlen > 0)
            {
                TRACE
                // send ACK to slave while rxlen > 0
                sercomDev->I2CM.CTRLB.reg &= ~SERCOM_I2CM_CTRLB_ACKACT; // send ACK
                // force repeated start by sending address, clears MB int flag
                sercomDev->I2CM.ADDR.reg = pI2cm->address << 1 | I2C_RD;
                i2cm_wait_for_sync(sercomDev);
            }
            else
            {
                TRACE
                // nothing to read, nothing more to write then stop and flag ready
                sercomDev->I2CM.CTRLB.reg |= I2CM_CMD_STOP;
                i2cm_wait_for_sync(sercomDev);
                pI2cm->status = I2CM_OK;
                pI2cm->ready = true;
                pI2cm->rxlen = 0;
                xSemaphoreGiveFromISR(pI2cm->timerReleaseSem, &xHigherPriorityTaskWoken);
            }
        }
        sercomDev->I2CM.INTFLAG.reg = SERCOM_I2CM_INTFLAG_MB; //clear interrupt flag
    }
    else if (flag & SERCOM_I2CM_INTFLAG_SB)
    { // byte from slave
        if (pI2cm->rxlen > 0)
        {
            TRACE
            pI2cm->rxlen--;
            if (pI2cm->rxlen == 0)
            {
                TRACE
                // send NACK and STOP for final byte
                sercomDev->I2CM.CTRLB.reg |= I2CM_CMD_STOP;
                i2cm_wait_for_sync(sercomDev);
                sercomDev->I2CM.CTRLB.reg |= SERCOM_I2CM_CTRLB_ACKACT; // send NAK
                pI2cm->ready = true;
                xSemaphoreGiveFromISR(pI2cm->timerReleaseSem, &xHigherPriorityTaskWoken);
            }
            TRACE
            // read data
            *pI2cm->rx++ = sercomDev->I2CM.DATA.reg; // clears SB
            i2cm_wait_for_sync(sercomDev);
        }
        sercomDev->I2CM.INTFLAG.reg = SERCOM_I2CM_INTFLAG_SB; //clear interrupt flag
    }
    else
    {
        printfFromInterruptNoFlush(
            "I2C master invalid interrupt flag 0x%x, reg = 0x%x", flag, sercomDev->I2CM.INTFLAG.reg);
    }
}

// Bob's algorithm to reset stuck slaves
// takes control SCL and SDA pins, uses them as gpios and returns them to i2c controller
void i2cm_clear_bus(i2cmMsg_t *pI2cm)
{
    uint8_t sda = pI2cm->sdaPin;
    uint8_t scl = pI2cm->sclPin;
    uint8_t last_sda_function;
    uint8_t last_scl_function;
    uint8_t sdahigh;
    int i;

    Sercom *sercomDev = pI2cm->sercomDev;

    // wait a few ms to clear, __NOP() is from CMSIS
    for (i = 0; i < 10000; i++)
        __NOP();
    sercomDev->I2CM.CTRLA.reg &= ~SERCOM_I2CM_CTRLA_ENABLE;
    i2cm_wait_for_sync(sercomDev);
    last_sda_function = gpio_port_get_pin_function(sda);
    last_scl_function = gpio_port_get_pin_function(scl);
    gpio_set_OD(scl, 1);
    gpio_set_out(scl, 1); // OD output
    gpio_set_dir_in(sda);
    sdahigh = 0;
    for (int j = 0; j < 100; j++)
    {
        if (gpio_get_in(sda))
        {
            if (++sdahigh > 8)
                break;
        }
        else
        {
            sdahigh = 0;
        }
        // drive the clock low, then release (open drain)
        gpio_set_out(scl, 0);
        for (i = 0; i < 300; i++)
            __NOP();
        gpio_set_out(scl, 1);
        for (i = 0; i < 300; i++)
            __NOP();
    }
    gpio_set_OD(scl, 0);
    gpio_port_set_pin_function(sda, last_sda_function);
    gpio_port_set_pin_function(scl, last_scl_function);
    sercomDev->I2CM.CTRLA.reg |= SERCOM_I2CM_CTRLA_ENABLE;
    i2cm_wait_for_sync(sercomDev);
    // force bus state to idle
    sercomDev->I2CM.STATUS.reg = I2CM_BUSSTATE_IDLE;
    i2cm_wait_for_sync(sercomDev);
    // if bus isn't idle then unrecoverable bus fault
}

// sets up i2c controller, starts i2c operation and starts timeout timer
static void i2cm_start_transfer(i2cmMsg_t *pI2cm);
void i2cm_start_transfer(i2cmMsg_t *pI2cm)
{
    //TRACE
    Sercom *sercomDev = pI2cm->sercomDev;

    if ((sercomDev->I2CM.STATUS.reg & SERCOM_I2CM_STATUS_BUSSTATE_Msk) == I2CM_BUSSTATE_UNKNOWN)
    {
        sercomDev->I2CM.STATUS.reg = I2CM_BUSSTATE_IDLE;
        i2cm_wait_for_sync(sercomDev);
    }
    // clear interrupts
    sercomDev->I2CM.INTENCLR.reg = SERCOM_I2CM_INTENCLR_MASK;

    // clear and reset interrupts
    sercomDev->I2CM.INTFLAG.reg = SERCOM_I2CM_INTFLAG_MASK;
    sercomDev->I2CM.INTENSET.reg = SERCOM_I2CM_INTENSET_MB | SERCOM_I2CM_INTENSET_SB;

    sercomDev->I2CM.CTRLB.reg &= ~SERCOM_I2CM_CTRLB_ACKACT; // send ACK

    sercomDev->I2CM.ADDR.reg = (pI2cm->address << 1) // actual transfer started by writing address
                               | (((pI2cm->txlen == 0) && (pI2cm->rxlen > 0)) ? I2C_RD : I2C_WR);
    i2cm_wait_for_sync(sercomDev);
}

// Sets up i2cmMsg_t structure for i2c interrupt handler, starts i2c operation, waits for it to finish
// and handles clearing bus hangs.
// Chooses i2cm port 1 or 2 according to bit 7 (unused in 7 bit address) of requested i2c address,
// clearing bit 7 when passed on to i2cm port 2.
static i2cmErr_t i2cmDo1(uint8_t address, uint8_t *tx, uint32_t txlen, uint8_t *rx, uint32_t rxlen)
{
    // choose the i2cmMsg_t structure to use according to bit 7 of address
    uint8_t i2cmChan = (address >> 7) & 0x01;
    assertMsg(i2cmChan < NUM_I2CM, "No 2nd I2cm");
    i2cmMsg_t *pI2cm = &gblI2cmInfo[i2cmChan];

    address &= 0x7F; // cleat bit 7 of address
    Sercom *sercomDev = pI2cm->sercomDev;

    // protect each i2c port from use by multiple threads
    xSemaphoreTake(pI2cm->protectionSem, portMAX_DELAY);

    // set up member variables used by i2c interrupt handler
    i2cmErr_t rv;
    pI2cm->address = address;
    pI2cm->rx = rx;
    pI2cm->tx = tx;
    pI2cm->txlen = txlen;
    pI2cm->rxlen = rxlen;
    pI2cm->status = I2CM_OK;
    pI2cm->ready = false;

    // do the operation
    i2cm_start_transfer(pI2cm);
    if (xSemaphoreTake(pI2cm->timerReleaseSem, pdMS_TO_TICKS(50)) != pdPASS)
    {
        pI2cm->status = I2CM_TIMEOUT;
    }
    rv = pI2cm->status;

    // check for bus hangs and try to fix
    if ((sercomDev->I2CM.STATUS.reg & SERCOM_I2CM_STATUS_BUSERR) != 0 ||
        (sercomDev->I2CM.STATUS.reg & SERCOM_I2CM_STATUS_BUSSTATE_Msk) == I2CM_BUSSTATE_UNKNOWN ||
        (sercomDev->I2CM.STATUS.reg & SERCOM_I2CM_STATUS_BUSSTATE_Msk) == I2CM_BUSSTATE_BUSY)
    { // something wrong with bus state, try to fix
        sercomDev->I2CM.STATUS.reg = SERCOM_I2CM_STATUS_BUSERR | I2CM_BUSSTATE_IDLE; // clear i2c controller
        i2cm_wait_for_sync(sercomDev);
        i2cm_clear_bus(pI2cm); // try to clear slaves
        rv = I2CM_BUSFAULT;    // report error
    }

    xSemaphoreGive(pI2cm->protectionSem);
    return rv;
}

// Main i2c API helper function
// Handles fake i2c device addresses that specifies a particular page on PBM regulators
// by setting the page using a separate i2c operation before doing the requested i2c operation.
// It translating fake address to the real device address according to table in boardchipinfo.h
// It calls i2cmDo1 (above) to do the operation(s)
static i2cmErr_t i2cmDo(uint8_t address, uint8_t *tx, uint32_t txlen, uint8_t *rx, uint32_t rxlen)
{
    i2cmErr_t rv;

    // choose the i2cmMsg_t structure to use according to bit 7 of address
    uint8_t i2cmChan = (address >> 7) & 0x01;
    assertMsg(i2cmChan < NUM_I2CM, "No 2nd I2cm");
    i2cmMsg_t *pI2cm = &gblI2cmInfo[i2cmChan];

    // select page if necessary
    t_pageXlate const *p;

    xSemaphoreTake(pI2cm->pageRegisterProtectionSem, portMAX_DELAY);

    for (p = pageXlate; p->addr != 0; ++p)
    {
        if (p->addr == address)
        {
            address = p->realAddr;
            if (p->pageVal == *(p->pCurPageVal))
            {
                break;
            }
            uint8_t outBuf[2];
            outBuf[0] = p->pageReg;
            outBuf[1] = p->pageVal;
            rv = i2cmDo1(address, outBuf, 2, NULL, 0);
            if (rv != I2CM_OK)
            {
                xSemaphoreGive(pI2cm->pageRegisterProtectionSem);
                return rv;
            }
            *(p->pCurPageVal) =
                p->pageVal; // this is safe because the two threads act on non overlapping curPageVal elements
            break;
        }
    }
    // then do requested operation

    rv = i2cmDo1(address, tx, txlen, rx, rxlen);

    xSemaphoreGive(pI2cm->pageRegisterProtectionSem);

    return rv;
}

///////////////////////////////////////////
// i2c API functions

// Probe - ST, addr+W, SP - check that addr got ACK
i2cmErr_t i2cm_probe(uint8_t address)
{
    // TODO SW-18070:
    // Workaround for receiving communication fault alert for MNN and NOC regulators.
    // Instead of dummy write (ST, addr+W, ST), which is treated as a protocol violation,
    // actual read of a random command register is called.
    if ((address == ADR_TPS53681) || (address == ADR_MNN) || (address == ADR_NOC))
    {
        uint8_t cmdReg[1];
        uint8_t data[1];
        cmdReg[0] = PMBUS__PMBUS_REVISION;
        return i2cmDo(address, cmdReg, 1, data, 1);
    }
    else
    {
        return i2cmDo(address, NULL, 0, NULL, 0);
    }
}

// ST, addr+W, data1 ..., SP
i2cmErr_t i2cm_write(uint8_t address, uint8_t *tx, uint32_t txlen)
{
    return i2cmDo(address, tx, txlen, NULL, 0);
}

// ST, addr+W, data1 ..., ST, addr+R, rd, ... SP - data1 could be register id byte, but more write bytes allowed
i2cmErr_t i2cm_read(uint8_t address, uint8_t *tx, uint32_t txlen, uint8_t *rx, uint32_t rxlen)
{
    return i2cmDo(address, tx, txlen, rx, rxlen);
}

// Skips fake address page setting - does normal raw i2c read
i2cmErr_t i2cm_readPageless(uint8_t address, uint8_t *tx, uint32_t txlen, uint8_t *rx, uint32_t rxlen)
{
    return i2cmDo1(address, tx, txlen, rx, rxlen);
}

// ST, addr+W, reg, SP (1 byte data interpreted as register with no (additional) data
i2cmErr_t i2cm_write0(uint8_t addr7, uint8_t reg)
{
    i2cmErr_t status;
    status = i2cm_write(addr7, &reg, 1);
    if (status != I2CM_OK)
    {
        setRegulatorCommFailError(addr7, reg, status);
    }
    return status;
}

// ST, addr+W, reg, databyte, SP
i2cmErr_t i2cm_write8(uint8_t addr7, uint8_t reg, uint8_t data)
{
    i2cmErr_t status;
    uint8_t tx[2];
    tx[0] = reg;
    tx[1] = data;
    status = i2cm_write(addr7, tx, 2);
    if (status != I2CM_OK)
    {
        setRegulatorCommFailError(addr7, reg, status);
    }
    return status;
}

// ST, addr+W, reg, databyte1, databyte2, SP
i2cmErr_t i2cm_write16(uint8_t addr7, uint8_t reg, uint16_t data)
{
    i2cmErr_t status;
    uint8_t tx[3];
    tx[0] = reg;
    tx[1] = (uint8_t)data;
    tx[2] = (uint8_t)(data >> 8);
    status = i2cm_write(addr7, tx, 3);
    if (status != I2CM_OK)
    {
        setRegulatorCommFailError(addr7, reg, status);
    }
    return status;
}

// ST, addr+W, reg, databyte1, databyte2, databyte3, databyte4, SP
i2cmErr_t i2cm_write32(uint8_t addr7, uint8_t reg, uint32_t data)
{
    i2cmErr_t status;
    uint8_t tx[5];
    tx[0] = reg;
    tx[1] = (uint8_t)data;
    tx[2] = (uint8_t)(data >> 8);
    tx[3] = (uint8_t)(data >> 16);
    tx[4] = (uint8_t)(data >> 24);
    status = i2cm_write(addr7, tx, 5);
    if (status != I2CM_OK)
    {
        setRegulatorCommFailError(addr7, reg, status);
    }
    return status;
}

// i2cm_write8 or i2cm_write16 depending on whether nBytes is 1 or 2
i2cmErr_t i2cm_write8or16(uint8_t addr7, uint8_t reg, uint16_t data, uint32_t nBytes)
{
    i2cmErr_t status;
    uint8_t tx[3];
    assertMsg(1 <= nBytes && nBytes <= 2, "Bad nbytes");
    tx[0] = reg;
    tx[1] = (uint8_t)data;
    tx[2] = (uint8_t)(data >> 8);
    status = i2cm_write(addr7, tx, nBytes + 1);
    if (status != I2CM_OK)
    {
        setRegulatorCommFailError(addr7, reg, status);
    }
    return status;
}

// ST, addr+W, reg, lng databyte1, databyte2, ..., databyte_lng, SP - used by PMBus
// databytes are taken from 64 bit parameter
i2cmErr_t i2cm_write_blk_word(uint8_t addr7, uint8_t reg, uint64_t data, uint8_t lng)
{
    i2cmErr_t status;
    uint8_t i, tx[18];
    tx[0] = reg;
    tx[1] = lng;
    for (i = 0; i < lng; ++i)
    {
        tx[2 + lng - 1 - i] = (uint8_t)data; // lo byte is last out
        data >>= 8;
    }
    status = i2cm_write(addr7, tx, lng + 2);
    if (status != I2CM_OK)
    {
        setRegulatorCommFailError(addr7, reg, status);
    }
    return status;
}

// ST, addr+W, reg, ST, addr+R, rd, ..., SP - one byte write, n byte read
i2cmErr_t i2cm_read1w(uint8_t addr7, uint8_t reg, uint8_t *rx, uint32_t rxlen)
{
    i2cmErr_t ret = i2cm_read(addr7, &reg, 1, rx, rxlen);
    if (ret != I2CM_OK)
    {
        setRegulatorCommFailError(addr7, reg, ret);
    }
    return ret;
}

// ST, addr+W, reg, ST, addr+R, rd, SP
i2cmErr_t i2cm_read8(uint8_t addr7, uint8_t reg, uint8_t *pdata)
{
    i2cmErr_t rv;
    uint8_t rx[1] = { 0 };
    if (I2CM_OK == (rv = i2cm_read1w(addr7, reg, rx, 1)))
    {
        *pdata = rx[0];
    }
    if (rv != I2CM_OK)
    {
        setRegulatorCommFailError(addr7, reg, rv);
    }
    return rv;
}

// ST, addr+W, reg, ST, addr+R, rd1, rd2, SP
i2cmErr_t i2cm_read16(uint8_t addr7, uint8_t reg, uint16_t *pdata)
{
    i2cmErr_t rv;
    uint8_t rx[2] = { 0 };
    rv = i2cm_read1w(addr7, reg, rx, 2);
    *pdata = rx[0] | (((uint16_t)rx[1]) << 8);
    if (rv != I2CM_OK)
    {
        setRegulatorCommFailError(addr7, reg, rv);
    }
    return rv;
}

// i2cm_read8 or i2cm_read16 depending on whether nBytes is 1 or 2
i2cmErr_t i2cm_read8or16(uint8_t addr7, uint8_t reg, uint16_t *pdata, uint32_t nBytes)
{
    i2cmErr_t rv;
    uint8_t rx[2] = { 0 };
    assertMsg(1 <= nBytes && nBytes <= 2, "Bad nbytes");
    rv = i2cm_read1w(addr7, reg, rx, nBytes);
    *pdata = rx[0];
    if (nBytes == 2)
        *pdata |= (((uint16_t)rx[1]) << 8);
    if (rv != I2CM_OK)
    {
        setRegulatorCommFailError(addr7, reg, rv);
    }
    return rv;
}

// ST, addr+W, reg, ST, addr+R, rdlng, rd1, rd2, ..., rd_lng, SP - used by PMBus
// lng parameter is how many bytes to read after first,
// first byte, rdlng, is length reported from the slave,
// rd1, ..., rd_lng are the remaining lng bytes which are combined into single 64 bit value
i2cmErr_t i2cm_read_blk_word(uint8_t addr7, uint8_t reg, uint64_t *pdata, uint8_t lng)
{
    i2cmErr_t rv;
    uint8_t rx[17] = { 0 };
    rv = i2cm_read1w(addr7, reg, rx, lng + 1);
    *pdata = 0ULL;
    for (uint8_t i = 0; i < lng; ++i)
    {
        *pdata <<= 8;
        *pdata |= rx[i + 1]; // first out is high byte
    }
    if (rv != I2CM_OK)
    {
        setRegulatorCommFailError(addr7, reg, rv);
    }
    return rv;
}

// returns pointer to text string depending on status/error code returned by i2c functions
const char *i2cm_get_error_message(i2cmErr_t err)
{
    const char *errstring[] = { "OK", "Bus Fault", "No Ack", "Timeout", "Bad msg", "Lost Arb" };
    return err < countof(errstring) ? errstring[err] : "BAD ERRCODE";
}

///////////////////////////////////////////
// block scan function and helpers - used to scan PMBus regulators periodically
//
// i2cmScanBlock( t_I2cmScanBlock const * ) takes the list of specified i2c reads, executes them
// and puts the resulting values and return status bytes in memory locations specified in the list.
//
// I2c addresses belonging to separate i2cm ports (as determined by bit 7 of the address) run in parallel
// to reduce the time it takes to do the list.
// Reads for i2cm port1 are done in the thread context of the caller and reads for i2cm port2
// are done in the context of a dedicated thread, I2cm2Task.

// declaration of helper function
static void i2cmScanChan(uint8_t chan);

// static pointer used to share input pointer to list with I2cm2Task thread
static t_I2cmScanBlock const *scanBlock;

// Reads list and save results.  See above.
// Runs i2cmScanChan() helper function simultaneously calling thread and dedicated thread.
void i2cmScanBlock(t_I2cmScanBlock const *_scanBlock)
{
    scanBlock = _scanBlock;
    // Release I2cm2Task to run helper. It has higher priority - will start immediately.
    // Note FreeRtos (in cooperative mode) won't run two threads simultaneously if they have the same priority.
    xSemaphoreGive(globals.i2cm2StartSemaphore);

    i2cmScanChan(1); // run helper in caller thread for i2cm port 1

    // wait for I2cm2Task to finish
    if (xSemaphoreTake(globals.i2cm2DoneSemaphore, pdMS_TO_TICKS(60)) != pdPASS)
    {
        printfFromInterruptNoFlush("globals.i2cm2DoneSemaphore timeout", 0, 0);
    }
}

// Helper function.  Runs simultaneously in each thread
void i2cmScanChan(uint8_t i2cmPort)
{
    uint8_t *pRv = scanBlock->rvList;
    uint8_t cnt;
    t_pmbScanItem const **pl;
    t_I2cmScanItem const *p;

    for (pl = scanBlock->scanList; *pl; ++pl)
    {
        for (cnt = 0; cnt < NREG; cnt++)
        {
            p = &((*pl)->items[cnt]);
            if (i2cmPort == ((p->addr >> 7) ? 2 : 1))
            { // skip addresses belonging to the other i2cmPort
                if (0xFF != p->lng)
                {
                    if (ADR_SRAMP0 == p->addr)
                    {
                        t_I2cmScanItem const *p2 = &((*(pl + 1))->items[cnt]);
                        *pRv = i2cm_read8or16(ADR_SRAMP0, p->reg, p->pVal, p->lng);
                        *(pRv + NREG) = i2cm_read8or16(ADR_SRAMP1, p2->reg, p2->pVal, p2->lng);
                    }
                    else if (ADR_SRAMP1 == p->addr)
                    {
                        return;
                    }
                    else
                    {
                        *pRv = i2cm_read8or16(p->addr, p->reg, p->pVal, p->lng);
                    }
                }
                // else both pRv and p->pVal remain 0/OK as they are static.
                ++pRv;
            }
        }
    }
}

// Thread dedicated to i2cm communication on port1 and port2.  Blocked when not being used.
void I2cm2Task(void *pvParameters)
{
    while (1)
    {
        xSemaphoreTake(globals.i2cm2StartSemaphore, portMAX_DELAY); // wait to work
        i2cmScanChan(2);                            // run helper in this (I2cm2Task) thread for i2cm port 2
        xSemaphoreGive(globals.i2cm2DoneSemaphore); // allow i2cmScanBlock to finish
    }
}
