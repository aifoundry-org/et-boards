/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file i2cs.c
    \brief C file for i2c Slave driver
*/
/***********************************************************************/

#include <compiler.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "features.h"
#include "error_codes.h"

#ifdef USE_I2CS

#include "FreeRTOS.h"
#include "board_defs.h"
#include "globals.h"
#include "gpio.h"
#include "i2cs.h"
#include "pmbstats.h"
#include "semphr.h"
#include "stream_buffer.h"
#include "task.h"
#include "utils.h"
#include "etsoc_cmd_handler_event.h"
#include "etsoc_cmd_handler_task.h"
#include "system_clock.h"

/* Uncomment this to enable logging in the driver */
//#define I2CS_LOGS

#ifdef I2CS_LOGS
#define I2CS_DEBUG_LOG(x, y, z) printfFromInterruptNoFlush(x, y, z)
#else
#define I2CS_DEBUG_LOG(x, y, z)
#endif

/***********************************************************************
 * Macros:        Defininition of local macros
***********************************************************************/
#define  SLAVE_READ   0   /* I2C master write mode */
#define  SLAVE_WRITE  1   /* I2C master read mode */

/* I2C slave command codes */
#define I2CS_CMD_LAST 2  /* Command to complete transaction */

#define I2C_DATA_SHIFT_POS(pktCount) (8 * (pktCount - 2))
#define DATA_BYTE(data, index) ((data >> (8 * index)) & 0xff)

/***********************************************************************
 * Data types:      Defininition of local data types
***********************************************************************/
typedef struct {
    /* Flags */
    bool requestDir;     /* Flag indicating if a read or write request is being made */
    bool writeOk;        /* Flag indicating if a write operation was successful */
    bool repeatedStart;  /* Flag indicating if a repeated start condition occurred */
    bool expectStop;     /* Flag indicating if a stop condition is expected */

    /* Data fields */
    uint32_t data;       /* Data associated with a request */
    uint32_t regData;    /* Data associated with read request */
    uint32_t dataLen;    /* Length of the data associated with a request */
    uint8_t regAddr;     /* Register address associated with a request */
    uint8_t pktCount;    /* Number of packets associated with a request */
} i2c_Req_cb;

/***********************************************************************
 * Variables:  Defininition of local variables and constants
***********************************************************************/
/* Data structure to hold I2C request related information*/
i2c_Req_cb i2c_req = {0};

/* I2C slave driver control block*/
i2c_cb_t i2cs_cb = {0};

/***********************************************************************
 * Functions:   Declaration of local functions
***********************************************************************/
static inline void i2cs_wait_for_sync(const Sercom *dev);
static inline void setAck(void);
static inline void setNack(void);
static inline void releaseDataLine(void);
static inline void clearStopInterruptFlag(void);
static inline void clearAddressMatchInterruptFlag(void);
static inline void clearDataReadyInterruptFlag(void);
static uint8_t i2c_slave_read(void); /* Master wants to write data to I2C Slave */
static void i2c_slave_write(uint8_t data_byte); /* Master wants to read data from I2C Slave */
static void i2c_process_command(void);
static inline void i2cs_process_stop_flag(void);
static inline void i2cs_process_address_match(void);
static inline void i2cs_process_slave_read(void);
static inline void i2cs_process_slave_write(void);
static inline void i2cs_process_data_ready(void);

/***********************************************************************
 * GLOBAL Functions:   Defininition of global functions
***********************************************************************/
int I2CS_Init() {
    /*  I2C Slave I2C0_I2C from Service Processor */
    // 12MHz to I2C0_I2C
    gclk_write_CLKCTRL(I2CS_GCLK
                    | GCLK_CLKCTRL_CLKEN
                    | I2CS_GCLK_ID);
    // Reset I2C and wait for it to complete, enable is cleared
    I2CS_SERCOM->I2CS.CTRLA.reg = SERCOM_I2CS_CTRLA_SWRST;
    while(I2CS_SERCOM->I2CS.CTRLA.reg & SERCOM_I2CS_CTRLA_SWRST);
    i2cs_wait_for_sync(I2CS_SERCOM);

    I2CS_SERCOM->I2CS.CTRLA.reg = SERCOM_I2CS_CTRLA_LOWTOUT // timeout for clock stretch > 25ms
                                | SERCOM_I2CS_CTRLA_SDAHOLD_75 //50-100ns hold time - verify???
                                | SERCOM_I2CS_CTRLA_MODE_I2C_SLAVE;
    i2cs_wait_for_sync(I2CS_SERCOM);

    gpio_port_set_pin_function(I2C0_SDA, MUX_I2C0_SDA);
    gpio_port_set_pin_function(I2C0_SCL, MUX_I2C0_SCL);

    I2CS_SERCOM->I2CM.CTRLB.reg = 0 //SERCOM_I2CS_CTRLB_SMEN    // semi smart mode enable
                                | SERCOM_I2CS_CTRLB_AMODE(0); // single slave address
    i2cs_wait_for_sync(I2CS_SERCOM);

    I2CS_SERCOM->I2CS.ADDR.reg = SERCOM_I2CS_ADDR_ADDRMASK(0) // single address no general call
                               | SERCOM_I2CS_ADDR_ADDR(SP_SLAVE_ADDRESS);

    I2CS_SERCOM->I2CS.INTENSET.reg = SERCOM_I2CS_INTENSET_AMATCH
                                   | SERCOM_I2CS_INTENSET_DRDY
                                   | SERCOM_I2CS_INTENSET_PREC;

    i2cs_wait_for_sync(I2CS_SERCOM);
    I2CS_SERCOM->I2CS.CTRLA.reg |= SERCOM_I2CS_CTRLA_ENABLE;
    i2cs_wait_for_sync(I2CS_SERCOM);

    /* this will be enabled once system init is complete*/
    NVIC_DisableIRQ(I2CS_IRQn);

    /* unblock ETSOC slave at start */
    I2CS_Set_Slave_Ready();
    return 0;
}

int I2CS_Deinit() {

    /* Disable interrupts */
    I2CS_SERCOM->I2CS.INTENCLR.reg = SERCOM_I2CS_INTENCLR_MASK;

    /* Clear interrupt flags */
    I2CS_SERCOM->I2CS.INTFLAG.reg = SERCOM_I2CS_INTFLAG_MASK;

    /* Disable module */
    I2CS_SERCOM->I2CS.CTRLA.reg &= ~SERCOM_I2CS_CTRLA_ENABLE;
    i2cs_wait_for_sync(I2CS_SERCOM);

    return 0;
}

/* Register command handler callback function*/
void I2CSRegisterCMDHandler(i2c_cmd_handler_callback cmd_handler)
{
    assert1(cmd_handler != NULL, "I2C slave cmd handler is NULL!", 0, 0);

    i2cs_cb.i2c_command_handler = cmd_handler;
}

void I2CS_Handler(void)
{
    I2CS_Set_Slave_Busy();

    /* Flag register has three bits
        DRDY    This flag is set when a I2C slave byte transmission is successfully completed.
        AMATCH  This flag is set when the I2C slave address match logic detects that a valid address has been received.
        PREC    This flag is set when a stop condition is detected for a transaction being processed. A stop condition detected.
    */
    uint8_t flag = I2CS_SERCOM->I2CS.INTFLAG.reg;
    I2CS_DEBUG_LOG("I2CS flag 0x%x, reg = 0x%x", flag, I2CS_SERCOM->I2CS.INTFLAG.reg);

    /* Flag register has eight bits, of which we check(*) three:
        CLKHOLD Set when the slave is holding the SCL line low, stretching the I2C clock.
                (Presumed when hardware is waiting for a new command before transaction is finished.)
        LOWTOUT Set if an SCL low time-out occurs.
        SR*     When INTFLAG.AMATCH is raised due to an address match, SR indicates a repeated start or start condition.
                This flag is only valid while the INTFLAG.AMATCH flag is one.
        DIR*    0 = Master write, 1 = Master read
        RXNACK  0 = Master responded with ACK, 1 = Master responded with NACK.
        COLL    If set, the I2C slave was not able to transmit a high data or NACK bit.  (Used with multiple slaves for SMBus ARP protocol.)
        BUSERR  Indicates that an illegal bus condition has occurred on the bus.
    */

    if (flag & SERCOM_I2CS_INTFLAG_AMATCH)
    {
        I2CS_DEBUG_LOG("AMATCH 0x%x", flag, 0);
        i2cs_process_address_match();
        I2CS_Set_Slave_Ready();
    }
    else if (flag & SERCOM_I2CS_INTFLAG_DRDY)
    {
        I2CS_DEBUG_LOG("DRDY 0x%x", flag, 0);
        i2cs_process_data_ready(); //pmic ready is handled in task
    }
    else if (flag & SERCOM_I2CS_INTFLAG_PREC)
    {
        I2CS_DEBUG_LOG("PREC 0x%x", flag, 0);
        i2cs_process_stop_flag();
        I2CS_Set_Slave_Ready();
    }
    else
    {
        // TODO[SW-16650]: Add invalid flag error handling
        printfFromInterruptNoFlush("I2CS invalid interrupt flag 0x%x, reg = 0x%x", flag, I2CS_SERCOM->I2CS.INTFLAG.reg);
        I2CS_Set_Slave_Ready();
    }

    I2CS_DEBUG_LOG("SReg: 0x%x flag: 0x%x", I2CS_SERCOM->I2CS.STATUS.reg, flag);
}

void I2CS_Set_Slave_Ready()
{
    /* Setting PMIC_RDY to high signals master that I2C slave is ready to receive data*/
    gpio_set_out_high_pin_id(PMIC_RDY_OUT);
}

void I2CS_Set_Slave_Busy()
{
    /* Setting PMIC_RDY to low signals master that I2C slave is busy and cannot receive data*/
    gpio_set_out_low_pin_id(PMIC_RDY_OUT);
}

bool I2CS_Get_Slave_Busy_Status()
{
    return gpio_get_out_pin_id(PMIC_RDY_OUT);
}

/***********************************************************************
 * Functions:   Defininition of local functions
***********************************************************************/
static inline void i2cs_wait_for_sync(const Sercom *dev)
{
    while (dev->I2CS.STATUS.reg & SERCOM_I2CS_STATUS_SYNCBUSY)
    {
        I2CS_DEBUG_LOG("i2cs_wait_for_sync", 0, 0);
    }
}

static inline void setAck(void)
{
    __disable_irq();
    I2CS_SERCOM->I2CS.STATUS.reg = 0;
    I2CS_SERCOM->I2CS.CTRLB.reg = 0;
    __enable_irq();
}

static inline void setNack(void)
{
    __disable_irq();
    I2CS_SERCOM->I2CS.STATUS.reg = 0;
    I2CS_SERCOM->I2CS.CTRLB.reg = SERCOM_I2CS_CTRLB_ACKACT;
    __enable_irq();
}

static inline void releaseDataLine(void)
{
    I2CS_SERCOM->I2CS.CTRLB.reg |= SERCOM_I2CS_CTRLB_CMD(I2CS_CMD_LAST); //also clears DRDY interrupt flag
}

static inline void clearStopInterruptFlag(void)
{
    /* Clear flag by setting INTFLAG register bit (errata reference 13574) */
    I2CS_SERCOM->I2CS.INTFLAG.reg = SERCOM_I2CS_INTFLAG_PREC;
}

static inline void clearAddressMatchInterruptFlag(void)
{
    /* Clear flag by setting INTFLAG register bit (errata reference 13574) */
    if (I2CS_SERCOM->I2CS.INTFLAG.bit.PREC) {
        I2CS_SERCOM->I2CS.INTFLAG.reg = SERCOM_I2CS_INTFLAG_PREC;
    }
    I2CS_SERCOM->I2CS.INTFLAG.reg = SERCOM_I2CS_INTFLAG_AMATCH;
}

static inline void clearDataReadyInterruptFlag(void)
{
    /* Clear flag by setting INTFLAG register bit (errata reference 13574) */
    I2CS_SERCOM->I2CS.INTFLAG.reg = SERCOM_I2CS_INTFLAG_DRDY;
}

/* Master wants to write data to I2C Slave */
static uint8_t i2c_slave_read(void)
{
    uint8_t data = I2CS_SERCOM->I2CS.DATA.reg;
    i2cs_wait_for_sync(I2CS_SERCOM);
    return data;
}

/* Master wants to read data from I2C Slave */
static void i2c_slave_write(uint8_t data_byte)
{
    I2CS_SERCOM->I2CS.DATA.reg = data_byte; //also clears DRDY interrupt flag
    i2cs_wait_for_sync(I2CS_SERCOM);
}

static void i2c_process_command(void)
{
    /* Send command to ETSOC command handler to process */
    i2cs_cb.i2c_command_handler((i2c_req.requestDir == SLAVE_WRITE), i2c_req.regAddr, i2c_req.data );
}

static inline void i2cs_process_stop_flag(void)
{
    i2c_req.pktCount = 0;
    i2c_req.expectStop = 0;

    clearStopInterruptFlag();
}

static inline void i2cs_process_address_match(void)
{
    //Read STATUS.DIR register to get read or write request direction (SLAVE_READ or SLAVE_WRITE)
    i2c_req.requestDir = (I2CS_SERCOM->I2CS.STATUS.reg & SERCOM_I2CS_STATUS_DIR) != 0;
    //Read STATUS.SR register to get repeated start or start condition
    i2c_req.repeatedStart = (I2CS_SERCOM->I2CS.STATUS.reg & SERCOM_I2CS_STATUS_SR) != 0;

    //According to PMIC-ETSOC communication protocol, expected scenarios are:
    //master write request at start condition because
    //the first data packet must be the register id
    //OR master read request at repeated start condition
    if((!i2c_req.repeatedStart && (i2c_req.requestDir == SLAVE_READ)) ||
           (i2c_req.repeatedStart && (i2c_req.requestDir == SLAVE_WRITE)))
    {
        // Acknowledge address or ...
        setAck();
    }
    else
    {
        // Send NACK ...
        setNack();
        i2csSetHandlerError(i2c_req.regAddr, (i2c_req.requestDir == SLAVE_READ) ?
                                                 I2CH_RPTD_START_IS_MASTER_WRITE_BIT :
                                                 I2CH_RPTD_START_AFTER_MASTER_READ_BIT);
    }
    //..and clear interrupt flag (errata reference 13574)
    clearAddressMatchInterruptFlag();

    // Reset data packet counter
    i2c_req.pktCount = 0;
}

static inline void i2cs_process_slave_read(void)
{
    /* Set the writeOk flag to 1 to indicate a successful write operation */
    i2c_req.writeOk = 1;

    /* If there's only one packet in the request: */
    if (i2c_req.pktCount == 1)
    {
        /* The first packet is the register address, so save it */
        i2c_req.regAddr = i2c_slave_read();

        /* Clear the data accumulator.
        * In the next packet master should send data accociated with write request
        * or repeated start condition for data read request. */
        i2c_req.data = 0;
        I2CS_Set_Slave_Ready();
    }
    else
    {
        /* Read data from slave registers */
        i2c_req.data = i2c_slave_read();

        /* write data into RAM address depending of register address - all writes are
        * single data byte */
        i2c_process_command();
    }

    /* Acknowledge and clear interrupt flag (errata reference 13574) */
    setAck();
    clearDataReadyInterruptFlag();
}

static inline void i2cs_process_slave_write(void)
{
    /* MASTER READ - 1st byte read request */
    if (i2c_req.pktCount == 1)
    {
        /* fetch data to send to master */
        bool ret = getSpCmdReadData(i2c_req.regAddr, &i2c_req.regData, &i2c_req.dataLen);
        if(!ret)
        {
            setNack();
            releaseDataLine();
            I2CS_Set_Slave_Ready();
            return;
        }
        /* process read command */
        i2c_process_command();

        /* Send data to slave if packet count is less than or equal to i2c_req.dataLen */

        /* Extract byte from data and write to DATA register */
        i2c_slave_write(DATA_BYTE(i2c_req.regData, (i2c_req.pktCount - 1)));
    }
    else if (i2c_req.pktCount > 1 && (i2c_req.pktCount <= i2c_req.dataLen))
    {
        /* Send data to slave if packet count is less than or equal to i2c_req.dataLen */

        /* Extract byte from data and write to DATA register */
        i2c_slave_write(DATA_BYTE(i2c_req.regData, (i2c_req.pktCount - 1)));

        I2CS_Set_Slave_Ready();
    }
    /* Release data line if the last byte was sent */
    else if (i2c_req.pktCount == (i2c_req.dataLen + 1))
    {
        setNack();
        releaseDataLine();
        I2CS_Set_Slave_Ready();
        return;
    }
}

static inline void i2cs_process_data_ready(void)
{
    /* Increment packet count*/
    ++i2c_req.pktCount;

    if (i2c_req.requestDir == SLAVE_READ)
    {
        i2cs_process_slave_read();
    } /* end MASTER WRITE */
    else if (i2c_req.requestDir == SLAVE_WRITE)
    {
        i2cs_process_slave_write();
    } /* end MASTER READ */
}

#endif /*  USE_I2CS */
