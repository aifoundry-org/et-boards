/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file error_codes.h
    \brief This file includes error code defines for various components.
*/
/***********************************************************************/

/* General error codes*/
#define STATUS_SUCCESS                           0
#define GENERAL_ERROR                           -1
#define INVALID_ARGUMENT                        -2

/* CLI Task Error Codes*/
#define CLI_TASK_ERROR_INVALID_CMD_CODE         -100
#define CLI_TASK_ERROR_INVALID_VALUE            -101

/* Command Handler Error Codes */
#define CMD_HANDLER_ERROR_INVALID_CMD           -200
#define CMD_HANDLER_ERROR_INVALID_REG_IDX       -201
#define CMD_HANDLER_ERROR_VALUE_OUT_OF_RANGE    -202
#define CMD_HANDLER_ERROR_INVALID_NUM_BYTES     -203
#define CMD_HANDLER_ERROR_REGULATOR_INFO        -204
#define CMD_HANDLER_ERROR_QUEUE_RECIEVE         -205
#define CMD_HANDLER_ERROR_TASK_QUEUE_FULL       -206
#define CMD_HANDLER_ERROR_QUEUE_SEND            -207
#define CMD_HANDLER_ERROR_READ_FORBIDDEN        -208
#define CMD_HANDLER_ERROR_WRITE_FORBIDDEN       -209

/* FW Update Error Codes */
#define FW_UPDATE_ERROR_QUEUE_RECEIVE           -300
#define FW_UPDATE_ERROR_ADDR_NOT_ALIGNED        -301
#define FW_UPDATE_ERROR_INVALID_DATA_SIZE       -302
#define FW_UPDATE_ERROR_INVALID_CMD             -303

/* Power Manager Errors*/
#define PWR_MGR_ERROR_PMB_STATS                 -400
#define PWR_MGR_ERROR_READING_CHANNEL           -401

/* Utils Error codes */
#define UTILS_GENERAL_ERROR                     -700
#define UTILS_ERROR_INAVLID_VALUE               -701
#define UTILS_ERROR_REG_NOT_WRIETABLE           -702
#define UTILS_ERROR_REG_NOT_READABLE            -703
#define UTILS_ERROR_REG_VAL_MISMATCH            -704

/* CLI & Console Error codes*/
#define UTILS_CLI_ERROR_INVALID_ARGUMENT        -800
#define UTILS_CLI_ERROR_INVALID_VALUE           -801
#define UTILS_CLI_ERROR_INVALID_CMD             -802

/* GPIO Error Codes */
#define GPIO_ERROR_INVALID_TYPE                 -1300

/* I2C Master Errors */
#define I2C_MASTER_ERROR_INVALID_ARGUMENT       -1400
#define I2C_MASTER_ERROR_REG_WRITE              -1401
#define I2C_MASTER_ERROR_REG_READ               -1402

/* I2C Slave Errors */
#define I2C_SLAVE_ERROR_INVALID_ARGUMENT        -1500

/* NVM Error Codes */
#define NVM_ERROR_CONTROL_REG                   -1600
#define NVM_ERROR_WAIT_TIMEOUT                  -1601
#define NVM_ERROR_INVALID_DST_ADDR              -1602
#define NVM_ERROR_INVALID_REQUEST               -1603

/* IO Expander Error Codes */
#define IOXP_ERROR_UNSUPPORTED_INDEX -1700