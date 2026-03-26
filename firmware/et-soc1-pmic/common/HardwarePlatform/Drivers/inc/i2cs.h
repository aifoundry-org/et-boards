/***********************************************************************
 *
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *
 ************************************************************************/

/***********************************************************************/
/*! \file i2cs.h
    \brief include definitions for I2CS SERCOM
*/
/***********************************************************************/

#ifndef __I2CS_SLAVE_H__
#define __I2CS_SLAVE_H__

/**
 * Function pointer to handle I2C read or write command.
 *
 * is_read: True if the command is a read, false if it's a write.
 * cmdIdx: The I2C command index.
 * data: parameter containing data.
 */
typedef int (*i2c_cmd_handler_callback)(bool is_read, uint8_t cmdIdx, uint32_t data);

/* Structure to hold an I2C command handler.*/
typedef struct {
    i2c_cmd_handler_callback i2c_command_handler;
    /* SW-16875: More elements to be added to this control block */
} i2c_cb_t;

int I2CS_Init(void);
int I2CS_Deinit(void);
void I2CS_Set_Slave_Ready(void);
void I2CS_Set_Slave_Busy(void);
bool I2CS_Get_Slave_Busy_Status(void);
int I2CS_Write(void);
int I2CS_Read(void);
void I2CSRegisterCMDHandler(i2c_cmd_handler_callback cmd_handler);

#endif //__I2CS_SLAVE_H__
