/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file globals.h
    \brief Globaly used types and definitions.
*/

#ifndef GLOBALS_H_
#define GLOBALS_H_

#include <string.h>
#include "FreeRTOS.h"
#include "stream_buffer.h"
#include "semphr.h"
#include "iostructs.h"
#include "features.h"

/***********************************************************************
 * GLOBAL Macros:        Defininition of global macros
***********************************************************************/
#define ETSOC_CMD_HANDLER_TASK_QUEUE_LENGTH 512
#define POWER_MANAGEMENT_TASK_QUEUE_LENGTH  256
#define FW_UPDATE_MANAGER_TASK_QUEUE_LENGTH 64

/***********************************************************************
 * GLOBAL data types:  Defininition of global data types
***********************************************************************/
typedef enum { eSS_None, eSS_Wait, eSS_Active } eSS_t;

typedef struct {
    char const *fileName;
    char const *fcnName;
    int lineNum;
    char const *threadName;
    eSS_t eSS;
} srcLine_t;

typedef struct {
    srcLine_t cur;
    srcLine_t prev;
} srcLine2_t;

typedef struct {
    char const *name;
    uint32_t *beg;
    uint32_t *end;
} stackInfo_t;

typedef struct {
    stackInfo_t main;
    stackInfo_t idle;
    stackInfo_t cli;
    stackInfo_t powerManager;
    stackInfo_t fast;
    stackInfo_t etsocCmdHandler;
    stackInfo_t fwUpdateManager;
    stackInfo_t i2cm2;
    stackInfo_t debug;
} allStacksInfo_t;

enum { TH_CLI, TH_POWER_MANAGER, TH_FASTTASK, TH_ETSOC_CMD_HANDLER_TASK, TH_FW_UPDATE_MANAGER, TH_I2CM2, TH_DEBUG };

typedef struct {
    char const *threadName;
    uint32_t const *stackBot;
    uint8_t threadId;
} threadLocal_t;

typedef struct {
    srcLine2_t print;
    srcLine2_t io;
    srcLine2_t fasttask;
    srcLine2_t intrNotify;
    srcLine2_t updateRelease;
    srcLine2_t updateDone;
    srcLine2_t t4Protect;
    srcLine2_t i2cm2Done;
    srcLine2_t ioxpRead;
} semInfo_t;

typedef struct {
    uint8_t promptState;
    bool needPrefix;
    uint8_t intrPrintDepth;
} cliGlobals_t;

typedef struct {
    char const *prompt;
    char *line;
    uint8_t state;
    uint8_t eMcode;
    uint8_t curpos;
    uint8_t curlen;
    uint8_t bufsize;
    bool newLine;
    char *historyBuf;
    uint16_t historyBufNextIdx, historyBufSize;
    uint16_t *lineBegIdxBuf;
    uint16_t lineBegIdxBufSize, firstLineBufIdx, endLineBufIdx;
    uint16_t thisHistLine;
    char const *histCmdName;
    char const *histCmdAltName;
} getline_t;

typedef enum {
    RESERVED = 0,
    FIRMWARE_VERSION,
    BOARD_TYPE,
    VOLTAGE_IN,
    POWER_IN,
    AVERAGE_POWER,
    SYSTEM_TEMPERATURE,
    POWER_ALARM,
    TEMPERATURE_ALARM,
    INTERRUPT_CONF,
    INTERRUPT_CAUSE,
    REGULATOR_FAULT,
    REGULATOR_COMM_FAIL,
    ETSOC_COMM_FAIL,
    WATCHDOG_CONF,
    WATCHDOG_RESET,
    RESET_CAUSE,
    RESET_CMD,
    RESET_CONTROL,
    GPIO_CONF,
    GPIO_CONTROL,
    PMB_STATS,
    FW_UPDATE_CMD,
    FW_UPDATE_DATA,
    DDQLP_VOLTAGE,
    L2_CACHE_VOLTAGE,
    DDR_VOLTAGE,
    DDQ_VOLTAGE,
    PCIE_LOGIC_VOLTAGE,
    PCIE_VOLTAGE,
    MAXION_VOLTAGE,
    NOC_VOLTAGE,
    ALL_MINION_VOLTAGE,
    MINION_G1_VOLTAGE,
    MINION_G2_VOLTAGE,
    MINION_G3_VOLTAGE,
    MINION_G4_VOLTAGE,
    MINION_G5_VOLTAGE,
    MINION_G6_VOLTAGE,
    MINION_G7_VOLTAGE,
    MINION_G8_VOLTAGE,
    MINION_G9_VOLTAGE,
    MINION_G10_VOLTAGE,
    MINION_G11_VOLTAGE,
    MINION_G12_VOLTAGE,
    MINION_G13_VOLTAGE,
    MINION_G14_VOLTAGE,
    MINION_G15_VOLTAGE,
    MINION_G16_VOLTAGE,
    MINION_G17_VOLTAGE,
    FRU_OPS_CMD,
    FRU_OPS_DATA,

    NUMBER_OF_REGISTERS, //must be the last
} ETSOCVirtualRegisterIdx_t;

typedef struct {
    QueueHandle_t etsocCmdHandlerTaskQueueHandle;
    QueueHandle_t powerManagementTaskQueueHandle;
    QueueHandle_t fwUpdateManagerTaskQueueHandle;
    TaskHandle_t CLITaskHandle;
    TaskHandle_t IOTaskHandle;
    TaskHandle_t fastTaskHandle;
    TaskHandle_t etsocCmdHandlerTaskHandle;
    TaskHandle_t I2cm2TaskHandle;
    TaskHandle_t debugTaskHandle;
    TaskHandle_t fwUpdateManagerTaskHandle;
    SemaphoreHandle_t printSemaphore;
    SemaphoreHandle_t ioSemaphore;
    SemaphoreHandle_t fastTaskSemaphore;
    SemaphoreHandle_t t4ProtectSemaphore;
    SemaphoreHandle_t t4ReleaseSemaphore;
    SemaphoreHandle_t ioxpReadSemaphore;
    SemaphoreHandle_t i2cm2StartSemaphore;
    SemaphoreHandle_t i2cm2DoneSemaphore;
    cliGlobals_t cli;
    getline_t *pgl;
    semInfo_t semaphoreInfo;
    bool putchByIntr;
    bool blockPTFail;
    StreamBufferHandle_t volatile streamHandle;
    commonData_t commonData;
    uint16_t hardFaultTestFlags;
} globals_t;

/***********************************************************************
 * GLOBAL variables:  Defininition of global variables and constants
***********************************************************************/
extern allStacksInfo_t allStacksInfo;
extern globals_t globals;

extern TaskHandle_t pxCurrentTCB;

/***********************************************************************
 * GLOBAL functions:  Declaration of global functions
***********************************************************************/
inline void recordSemTake(srcLine2_t *psl, SemaphoreHandle_t sem, char const *fileName, char const *fcnName,
    int lineNum) //TODO: used in globals.h
{
    if (fileName)
    { // if not NULL, this is a take
        if (uxSemaphoreGetCount(sem) == 0)
        { // if already taken
            memcpy(&psl->prev, &psl->cur, sizeof(srcLine_t));
        }
        psl->cur.fileName = fileName;
        psl->cur.fcnName = fcnName;
        psl->cur.lineNum = lineNum;
        psl->cur.threadName = ((threadLocal_t *)pvTaskGetThreadLocalStoragePointer(NULL, 0))->threadName;
        psl->cur.eSS = eSS_Wait;
    }
    else
    {                                                     // this is a give
        memcpy(&psl->cur, &psl->prev, sizeof(srcLine_t)); // move prev back to cur
        memset(&psl->prev, 0, sizeof(srcLine_t));         // zero prev
        psl->cur.eSS = eSS_None;
    }
}

//#define DEBUG_SEMS /* save debug info */

#ifndef DEBUG_SEMS

#define PRINTSEM_TAKE                                          \
    {                                                          \
        xSemaphoreTake(globals.printSemaphore, portMAX_DELAY); \
    }
#define PRINTSEM_GIVE                           \
    {                                           \
        xSemaphoreGive(globals.printSemaphore); \
    }

#define IOSEM_TAKE                                          \
    {                                                       \
        xSemaphoreTake(globals.ioSemaphore, portMAX_DELAY); \
    }
#define IOSEM_GIVE                           \
    {                                        \
        xSemaphoreGive(globals.ioSemaphore); \
    }

#define FASTTASKSEM_TAKE                                          \
    {                                                             \
        xSemaphoreTake(globals.fastTaskSemaphore, portMAX_DELAY); \
    }
#define FASTTASKSEM_GIVE                           \
    {                                              \
        xSemaphoreGive(globals.fastTaskSemaphore); \
    }

#define T4PROTECTSEM_TAKE                                          \
    {                                                              \
        xSemaphoreTake(globals.t4ProtectSemaphore, portMAX_DELAY); \
    }
#define T4PROTECTSEM_GIVE                           \
    {                                               \
        xSemaphoreGive(globals.t4ProtectSemaphore); \
    }

#define IOXPREADSEM_TAKE                                          \
    {                                                             \
        xSemaphoreTake(globals.ioxpReadSemaphore, portMAX_DELAY); \
    }
#define IOXPREADSEM_GIVE                           \
    {                                              \
        xSemaphoreGive(globals.ioxpReadSemaphore); \
    }

#else /* save debug info */

#define PRINTSEM_TAKE                                                                                          \
    {                                                                                                          \
        recordSemTake(&globals.semaphoreInfo.print, globals.printSemaphore, __FILE__, __FUNCTION__, __LINE__); \
        xSemaphoreTake(globals.printSemaphore, portMAX_DELAY);                                                 \
        globals.semaphoreInfo.print.cur.eSS = eSS_Active;                                                      \
    }

#define PRINTSEM_GIVE                                                                       \
    {                                                                                       \
        recordSemTake(&globals.semaphoreInfo.print, globals.printSemaphore, NULL, NULL, 0); \
        xSemaphoreGive(globals.printSemaphore);                                             \
    }

#define IOSEM_TAKE                                                                                       \
    {                                                                                                    \
        recordSemTake(&globals.semaphoreInfo.io, globals.ioSemaphore, __FILE__, __FUNCTION__, __LINE__); \
        xSemaphoreTake(globals.ioSemaphore, portMAX_DELAY);                                              \
        globals.semaphoreInfo.io.cur.eSS = eSS_Active;                                                   \
    }

#define IOSEM_GIVE                                                                    \
    {                                                                                 \
        recordSemTake(&globals.semaphoreInfo.io, globals.ioSemaphore, NULL, NULL, 0); \
        xSemaphoreGive(globals.ioSemaphore);                                          \
    }

#define FASTTASKSEM_TAKE                                                                                             \
    {                                                                                                                \
        recordSemTake(&globals.semaphoreInfo.fasttask, globals.fastTaskSemaphore, __FILE__, __FUNCTION__, __LINE__); \
        xSemaphoreTake(globals.fastTaskSemaphore, portMAX_DELAY);                                                    \
        globals.semaphoreInfo.fasttask.cur.eSS = eSS_Active;                                                         \
    }

#define FASTTASKSEM_GIVE                                                                          \
    {                                                                                             \
        recordSemTake(&globals.semaphoreInfo.fasttask, globals.fastTaskSemaphore, NULL, NULL, 0); \
        xSemaphoreGive(globals.fastTaskSemaphore);                                                \
    }

#define T4PROTECTSEM_TAKE                                                                                              \
    {                                                                                                                  \
        recordSemTake(&globals.semaphoreInfo.t4Protect, globals.t4ProtectSemaphore, __FILE__, __FUNCTION__, __LINE__); \
        xSemaphoreTake(globals.t4ProtectSemaphore, portMAX_DELAY);                                                     \
        globals.semaphoreInfo.t4Protect.cur.eSS = eSS_Active;                                                          \
    }

#define T4PROTECTSEM_GIVE                                                                           \
    {                                                                                               \
        recordSemTake(&globals.semaphoreInfo.t4Protect, globals.t4ProtectSemaphore, NULL, NULL, 0); \
        xSemaphoreGive(globals.t4ProtectSemaphore);                                                 \
    }

#define I2CM2DONESEM_TAKE                                                                                              \
    {                                                                                                                  \
        recordSemTake(&globals.semaphoreInfo.i2cm2Done, globals.i2cm2DoneSemaphore, __FILE__, __FUNCTION__, __LINE__); \
        xSemaphoreTake(globals.i2cm2DoneSemaphore, portMAX_DELAY);                                                     \
        globals.semaphoreInfo.i2cm2Done.cur.eSS = eSS_Active;                                                          \
    }

#define I2CM2DONESEM_GIVE                                                                           \
    {                                                                                               \
        recordSemTake(&globals.semaphoreInfo.i2cm2Done, globals.i2cm2DoneSemaphore, NULL, NULL, 0); \
        xSemaphoreGive(globals.i2cm2DoneSemaphore);                                                 \
    }

#define IOXPREADSEM_TAKE                                                                                             \
    {                                                                                                                \
        recordSemTake(&globals.semaphoreInfo.ioxpRead, globals.ioxpReadSemaphore, __FILE__, __FUNCTION__, __LINE__); \
        xSemaphoreTake(globals.ioxpReadSemaphore, portMAX_DELAY);                                                    \
        globals.semaphoreInfo.ioxpRead.cur.eSS = eSS_Active;                                                         \
    }

#define IOXPREADSEM_GIVE                                                                          \
    {                                                                                             \
        recordSemTake(&globals.semaphoreInfo.ioxpRead, globals.ioxpReadSemaphore, NULL, NULL, 0); \
        xSemaphoreGive(globals.ioxpReadSemaphore);                                                \
    }

#endif

#endif /* GLOBALS_H_ */
