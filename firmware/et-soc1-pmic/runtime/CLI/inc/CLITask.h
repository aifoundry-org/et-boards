/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file CLITask.h
    \brief Command Line Interface Task
*/
/***********************************************************************/

#ifndef __CLITASK_H__
#define __CLITASK_H__

#include "i2cm2.h"
#include "globals.h"

enum { PS_none, PS_needed, PS_alreadyThere, PS_interrupted };

typedef enum {
    INVALID_CMD = -1,
    CMD_HISTORY,
    CMD_I2CW,
    CMD_I2CR,
    CMD_I2CRR,
    CMD_I2CWR,
    CMD_PWRON,
    CMD_PWROFF,
    CMD_PWROFF2,
    CMD_I2CSCAN,
    CMD_SET_GPIO,
    CMD_STATUS,
    CMD_HELP,
    CMD_SET_CHIP,
    CMD_SET_REGISTER,
    CMD_READ_REGISTER,
    CMD_REGISTER_VALUE,
    CMD_SET_FIELD,
    CMD_MODIFY_FIELD,
    CMD_MODIFY_FIELD_DEC,
    CMD_WRITEOUT_REGISTER,
    CMD_RUNSCRIPT,
    CMD1P8_ONLY,
    CMD_T,
    CMD_FS,
    CMD_RSOC,
    CMD_SPI_PWR,
    CMD_EXIT,
    CMD_PMB_READREGS,
    CMD_VREG_MENU,
    CMD_SP_MENU,
    CMD_LISTV,
    CMD_READV,
    CMD_SETV,
    CMD_CLPMB,
    CMD_RPMB,
    CMD_STACK,
    CMD_CHK_REGU,
    CMD_FS1406_READREGS,
    CMD_DMP_FAN,
    CMD_FRU_READ,
    CMD_FRU_WRITE,
    CMD_FRU_PRINT,
    CMD_SET_REGCHK,
    CMD_VERSION,
    CMD_REBOOT,
    CMD_BI,
    CMD_U,
    CMD_TIMEDISP,
    CMD_USERROW,
    CMD_SMOKE_TEST,
    CMD_SWITCH_BOOT_SLOT
} command_t;

typedef struct {
    int const id;
    const char *name;
    const char *altname;
    const char *help;
} commandList_t;

extern commandList_t const commandList[];

void interruptingPrintBegin(void);
void interruptingPrintNL(void);
void interruptingPrintEnd(void);
void interruptingPrintf(const char *format, ...);
void interruptingPrintfGrouped(bool *pSavePrinted, const char *format, ...);
void interruptingPrintfGroupedFirstOnly(bool *pSavePrinted, const char *format, ...);
void i2cPErrorGrouped(bool *pSavePrinted, i2cmErr_t rv, char const *msg);
void interruptingPrintfGroupedEnd(bool *pSavePrinted);
void interruptingPrintfOneLine(const char *format, ...);
void checkEndInterrupted(void);
void restoreGetline(void);
void undoGetline(void);
void doPrompt(char const *);
void cmdBootToImage(char const *p1);
void cmdPi(void);
void cmdUserRow(char const *p1, char const *p2, char const *p3);

void cmdListv(void);
uint16_t cmdReadv(const char *id);
bool cmdSetv(const char *id, const char *mvstr, const powerState_t *pps);
bool cmdIntSetv(const char *id, const uint16_t mV);
bool cmdRPmb(const char *id, const char *mv);
void cmdVersion(const char *p);
char *summaryHash(char *buf, int buf_size);

/**
 * @fn CLITask
 * @brief FreeRTOS task that provides UART command line access to PMIC
 *
 * @param[in] pvParameters unused
 */
void CLITask(void *pvParameters);
void vRegMenu(char *line, uint8_t *pMenuType, const powerState_t *pps);
void spMenu(char *line, uint8_t *pMenuType);

#endif // __CLITASK_H__
