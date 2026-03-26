/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file main.c
    \brief Main application C file
*/
/***********************************************************************/

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <compiler.h>
#include <string.h>
#include "main.h"
#include "FreeRTOS.h"
#include "hooks.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "globals.h"
#include "board_defs.h"
#include "common_defs.h"
#include "gpio.h"
#include "cli.h"
#include "utils.h"
#include "i2cm.h"
#include "i2cs.h"
#include "system_clock.h"
#include "etsoc_cmd_handler_event.h"
#include "etsoc_cmd_handler_task.h"
#include "CLITask.h"
#include "iostructs.h"
#include "IoTask.h"
#include "chipio.h"
#include "version.h"
#include "features.h"
#include "bootimage.h"
#include "hw_encoding.h"

/***********************************************************************
 * Macros:        Defininition of local macros
***********************************************************************/
#define DEBUGTASK_PRIORITY              (tskIDLE_PRIORITY + 0)
#define IOTASK_PRIORITY                 (tskIDLE_PRIORITY + 1)
#define CLITASK_PRIORITY                (tskIDLE_PRIORITY + 2)
#define FASTTASK_PRIORITY               (tskIDLE_PRIORITY + 3)
#define ETSOC_CMD_HANDLER_TASK_PRIORITY (tskIDLE_PRIORITY + 4)
#define I2CM2TASK_PRIORITY              (tskIDLE_PRIORITY + 5)
#define UPDATETASK_PRIORITY             (tskIDLE_PRIORITY + 6)

#define DEBUG_STACK_SIZE             (configMINIMAL_STACK_SIZE + 0)
#define IO_STACK_SIZE                (configMINIMAL_STACK_SIZE + 192)
#define CLI_STACK_SIZE               (configMINIMAL_STACK_SIZE + 192)
#define FAST_STACK_SIZE              (configMINIMAL_STACK_SIZE + 128)
#define ETSOC_CMD_HANDLER_STACK_SIZE (configMINIMAL_STACK_SIZE + 64)
#define I2CM2TASK_STACK_SIZE         (configMINIMAL_STACK_SIZE + 64)
#define FW_UPDATE_MANAGER_STACK_SIZE (configMINIMAL_STACK_SIZE + 64)

/***********************************************************************
 * Data types:      Defininition of local data types
***********************************************************************/
typedef struct {
    uint32_t volatile r0;
    uint32_t volatile r1;
    uint32_t volatile r2;
    uint32_t volatile r3;
    uint32_t volatile r12;
    uint32_t volatile lr;
    uint32_t volatile pc;
    uint32_t volatile psr;
} faultRegs_t;

/***********************************************************************
 * Functions:   Declaration of local functions
***********************************************************************/
void CHardFaultHandler(uint32_t *rsp);
void debugTask(void *pvParameters);

/***********************************************************************
 * GLOBAL Functions:   Defininition of global functions
***********************************************************************/
int main(void)
{
    memset(&globals.commonData, 0, sizeof(commonData_t));
    globals.commonData.powerState.hwUp = false;
    globals.commonData.analogData.initV = false;
    globals.putchByIntr = false;
    globals.blockPTFail = true; // ignore PTFail interrupts until power is up

    system_init2();
    recordTime("after sysinit");

    static StaticSemaphore_t printSemaphoreBuffer;
    globals.printSemaphore = xSemaphoreCreateBinaryStatic(&printSemaphoreBuffer); // initially free.
    xSemaphoreGive(globals.printSemaphore);

    static StaticSemaphore_t ioSemaphoreBuffer;
    globals.ioSemaphore = xSemaphoreCreateBinaryStatic(&ioSemaphoreBuffer); // initially blocked.  Released by IOTask

    static StaticSemaphore_t fastTaskSemaphoreBuffer;
    globals.fastTaskSemaphore =
        xSemaphoreCreateBinaryStatic(&fastTaskSemaphoreBuffer); // initially blocked.  Released by IOTask

    static StaticSemaphore_t t4ProtectSemaphoreBuffer;
    globals.t4ProtectSemaphore = xSemaphoreCreateBinaryStatic(&t4ProtectSemaphoreBuffer);
    xSemaphoreGive(globals.t4ProtectSemaphore);

    static StaticSemaphore_t t4ReleaseSemaphoreBuffer;
    globals.t4ReleaseSemaphore = xSemaphoreCreateBinaryStatic(&t4ReleaseSemaphoreBuffer);

    static StaticSemaphore_t ioxpReadSemaphoreBuffer;
    globals.ioxpReadSemaphore = xSemaphoreCreateBinaryStatic(&ioxpReadSemaphoreBuffer);
    xSemaphoreGive(globals.ioxpReadSemaphore);

    static StaticSemaphore_t i2cm2StartSemaphoreBuffer;
    globals.i2cm2StartSemaphore = xSemaphoreCreateBinaryStatic(&i2cm2StartSemaphoreBuffer); // initially blocked.

    static StaticSemaphore_t i2cm2DoneSemaphoreBuffer;
    globals.i2cm2DoneSemaphore = xSemaphoreCreateBinaryStatic(&i2cm2DoneSemaphoreBuffer); // initially blocked.

    static StaticQueue_t etsocCmdHandlerTaskQueueState;
    static uint8_t
        etsocCmdHandlerTaskQueueStorageArea[ETSOC_CMD_HANDLER_TASK_QUEUE_LENGTH * sizeof(etsocCommandEvent_t)];
    globals.etsocCmdHandlerTaskQueueHandle = xQueueCreateStatic(ETSOC_CMD_HANDLER_TASK_QUEUE_LENGTH,
        sizeof(etsocCommandEvent_t), etsocCmdHandlerTaskQueueStorageArea, &etsocCmdHandlerTaskQueueState);

    static StaticQueue_t powerManagementQueueState;
    static uint8_t powerManagementQueueStorageArea[POWER_MANAGEMENT_TASK_QUEUE_LENGTH * sizeof(powerManagerEvent_t)];
    globals.powerManagementTaskQueueHandle = xQueueCreateStatic(POWER_MANAGEMENT_TASK_QUEUE_LENGTH,
        sizeof(powerManagerEvent_t), powerManagementQueueStorageArea, &powerManagementQueueState);

    static StaticQueue_t fwUpdateManagerTaskQueueState;
    static uint8_t
        fwUpdateManagerTaskQueueStorageArea[FW_UPDATE_MANAGER_TASK_QUEUE_LENGTH * sizeof(fwUpdateManagerEvent_t)];
    globals.fwUpdateManagerTaskQueueHandle = xQueueCreateStatic(
        1, sizeof(fwUpdateManagerEvent_t), fwUpdateManagerTaskQueueStorageArea, &fwUpdateManagerTaskQueueState);

    StaticTask_t *pxIdleTaskTCBBuffer;
    uint32_t ulIdleTaskStackSize;
    vApplicationGetIdleTaskMemory(&pxIdleTaskTCBBuffer, &allStacksInfo.idle.beg, &ulIdleTaskStackSize);
    allStacksInfo.idle.end = &allStacksInfo.idle.beg[ulIdleTaskStackSize];
    allStacksInfo.idle.name = "idle";

    allStacksInfo.main.name = "main";
    allStacksInfo.main.beg = &_sstack;
    allStacksInfo.main.end = &_estack;

    static StaticTask_t CLITaskBuffer;
    static StackType_t CLITaskStack[CLI_STACK_SIZE];
    allStacksInfo.cli.beg = CLITaskStack;
    allStacksInfo.cli.end = endof(CLITaskStack);
    memset(allStacksInfo.cli.beg, 0xA5, (uint32_t)allStacksInfo.cli.end - (uint32_t)allStacksInfo.cli.beg);
    static threadLocal_t CLITaskLocalStorage;
    allStacksInfo.cli.name = CLITaskLocalStorage.threadName = "CLI";
    CLITaskLocalStorage.threadId = TH_CLI;
    CLITaskLocalStorage.stackBot = CLITaskStack;
    globals.CLITaskHandle = xTaskCreateStatic(CLITask, CLITaskLocalStorage.threadName, countof(CLITaskStack),
        (void *)&globals.commonData, CLITASK_PRIORITY,
        CLITaskStack,    // Pointer to the buffer that the task being created will use as its stack.
        &CLITaskBuffer); // Pointer to a StaticTask_t structure for use as the memory require by the task.
    vTaskSetThreadLocalStoragePointer(globals.CLITaskHandle, 0, &CLITaskLocalStorage);

    static StaticTask_t IOTaskBuffer;
    static StackType_t IOTaskStack[IO_STACK_SIZE];
    allStacksInfo.powerManager.beg = IOTaskStack;
    allStacksInfo.powerManager.end = endof(IOTaskStack);
    memset(allStacksInfo.powerManager.beg, 0xA5,
        (uint32_t)allStacksInfo.powerManager.end - (uint32_t)allStacksInfo.powerManager.beg);
    static threadLocal_t IOTaskLocalStorage;
    allStacksInfo.powerManager.name = IOTaskLocalStorage.threadName = "IO";
    IOTaskLocalStorage.stackBot = IOTaskStack;
    IOTaskLocalStorage.threadId = TH_POWER_MANAGER;
    globals.IOTaskHandle = xTaskCreateStatic(IOTask, IOTaskLocalStorage.threadName, countof(IOTaskStack),
        (void *)&globals.commonData, IOTASK_PRIORITY,
        IOTaskStack,    // Pointer to the buffer that the task being created will use as its stack.
        &IOTaskBuffer); // Pointer to a StaticTask_t structure for use as the memory require by the task.
    vTaskSetThreadLocalStoragePointer(globals.IOTaskHandle, 0, &IOTaskLocalStorage);

    static StaticTask_t fastTaskBuffer;
    static StackType_t fastTaskStack[FAST_STACK_SIZE];
    allStacksInfo.fast.beg = fastTaskStack;
    allStacksInfo.fast.end = endof(fastTaskStack);
    memset(allStacksInfo.fast.beg, 0xA5, (uint32_t)allStacksInfo.fast.end - (uint32_t)allStacksInfo.fast.beg);
    static threadLocal_t fastTaskLocalStorage;
    allStacksInfo.fast.name = fastTaskLocalStorage.threadName = "Fast";
    fastTaskLocalStorage.stackBot = fastTaskStack;
    fastTaskLocalStorage.threadId = TH_FASTTASK;
    globals.fastTaskHandle = xTaskCreateStatic(fastTask, fastTaskLocalStorage.threadName, countof(fastTaskStack),
        (void *)&globals.commonData, FASTTASK_PRIORITY,
        fastTaskStack,    // Pointer to the buffer that the task being created will use as its stack.
        &fastTaskBuffer); // Pointer to a StaticTask_t structure for use as the memory require by the task.
    vTaskSetThreadLocalStoragePointer(globals.fastTaskHandle, 0, &fastTaskLocalStorage);

    static StaticTask_t etsocCmdHandlerTaskBuffer;
    static StackType_t etsocCmdHandlerTaskStack[ETSOC_CMD_HANDLER_STACK_SIZE];
    allStacksInfo.etsocCmdHandler.beg = etsocCmdHandlerTaskStack;
    allStacksInfo.etsocCmdHandler.end = endof(etsocCmdHandlerTaskStack);
    memset(allStacksInfo.etsocCmdHandler.beg, 0xA5,
        (uint32_t)allStacksInfo.etsocCmdHandler.end - (uint32_t)allStacksInfo.etsocCmdHandler.beg);
    static threadLocal_t etsocCmdHandlerTaskLocalStorage;
    allStacksInfo.etsocCmdHandler.name = etsocCmdHandlerTaskLocalStorage.threadName = "ETSOC_CMD_HANDLER";
    etsocCmdHandlerTaskLocalStorage.stackBot = etsocCmdHandlerTaskStack;
    etsocCmdHandlerTaskLocalStorage.threadId = TH_ETSOC_CMD_HANDLER_TASK;
    globals.etsocCmdHandlerTaskHandle = xTaskCreateStatic(etsocCmdHandlerTask,
        etsocCmdHandlerTaskLocalStorage.threadName, countof(etsocCmdHandlerTaskStack), (void *)&globals.commonData,
        ETSOC_CMD_HANDLER_TASK_PRIORITY,
        etsocCmdHandlerTaskStack,    // Pointer to the buffer that the task being created will use as its stack.
        &etsocCmdHandlerTaskBuffer); // Pointer to a StaticTask_t structure for use as the memory require by the task.
    vTaskSetThreadLocalStoragePointer(globals.etsocCmdHandlerTaskHandle, 0, &etsocCmdHandlerTaskLocalStorage);

    static StaticTask_t fwUpdateManagerTaskBuffer;
    static StackType_t fwUpdateManagerTaskStack[FW_UPDATE_MANAGER_STACK_SIZE];
    allStacksInfo.fwUpdateManager.beg = fwUpdateManagerTaskStack;
    allStacksInfo.fwUpdateManager.end = endof(fwUpdateManagerTaskStack);
    memset(allStacksInfo.fwUpdateManager.beg, 0xA5,
        (uint32_t)allStacksInfo.fwUpdateManager.end - (uint32_t)allStacksInfo.fwUpdateManager.beg);
    static threadLocal_t fwUpdateManagerTaskLocalStorage;
    allStacksInfo.fwUpdateManager.name = fwUpdateManagerTaskLocalStorage.threadName = "UPDATE";
    fwUpdateManagerTaskLocalStorage.stackBot = fwUpdateManagerTaskStack;
    fwUpdateManagerTaskLocalStorage.threadId = TH_FW_UPDATE_MANAGER;
    globals.fwUpdateManagerTaskHandle = xTaskCreateStatic(fwUpdateManagerTask,
        fwUpdateManagerTaskLocalStorage.threadName, countof(fwUpdateManagerTaskStack), (void *)&globals.commonData,
        UPDATETASK_PRIORITY,
        fwUpdateManagerTaskStack,    // Pointer to the buffer that the task being created will use as its stack.
        &fwUpdateManagerTaskBuffer); // Pointer to a StaticTask_t structure for use as the memory require by the task.
    vTaskSetThreadLocalStoragePointer(globals.fwUpdateManagerTaskHandle, 0, &fwUpdateManagerTaskLocalStorage);

    static StaticTask_t I2cm2TaskBuffer;
    static StackType_t I2cm2TaskStack[I2CM2TASK_STACK_SIZE];
    allStacksInfo.i2cm2.beg = I2cm2TaskStack;
    allStacksInfo.i2cm2.end = endof(I2cm2TaskStack);
    memset(allStacksInfo.i2cm2.beg, 0xA5, (uint32_t)allStacksInfo.i2cm2.end - (uint32_t)allStacksInfo.i2cm2.beg);
    static threadLocal_t I2cm2TaskLocalStorage;
    allStacksInfo.i2cm2.name = I2cm2TaskLocalStorage.threadName = "I2cm2";
    I2cm2TaskLocalStorage.stackBot = I2cm2TaskStack;
    I2cm2TaskLocalStorage.threadId = TH_I2CM2;
    globals.I2cm2TaskHandle = xTaskCreateStatic(I2cm2Task, I2cm2TaskLocalStorage.threadName, countof(I2cm2TaskStack),
        NULL, I2CM2TASK_PRIORITY,
        I2cm2TaskStack,    // Pointer to the buffer that the task being created will use as its stack.
        &I2cm2TaskBuffer); // Pointer to a StaticTask_t structure for use as the memory require by the task.
    vTaskSetThreadLocalStoragePointer(globals.I2cm2TaskHandle, 0, &I2cm2TaskLocalStorage);

    static StaticTask_t debugTaskBuffer;
    static StackType_t debugTaskStack[DEBUG_STACK_SIZE];
    allStacksInfo.debug.beg = debugTaskStack;
    allStacksInfo.debug.end = endof(debugTaskStack);
    memset(allStacksInfo.debug.beg, 0xA5, (uint32_t)allStacksInfo.debug.end - (uint32_t)allStacksInfo.debug.beg);
    static threadLocal_t debugTaskLocalStorage;
    allStacksInfo.debug.name = debugTaskLocalStorage.threadName = "Debug";
    debugTaskLocalStorage.stackBot = debugTaskStack;
    debugTaskLocalStorage.threadId = TH_DEBUG;
    globals.debugTaskHandle = xTaskCreateStatic(debugTask, debugTaskLocalStorage.threadName, countof(debugTaskStack),
        NULL, DEBUGTASK_PRIORITY,
        debugTaskStack,    // Pointer to the buffer that the task being created will use as its stack.
        &debugTaskBuffer); // Pointer to a StaticTask_t structure for use as the memory require by the task.
    vTaskSetThreadLocalStoragePointer(globals.debugTaskHandle, 0, &debugTaskLocalStorage);

    /* Start the scheduler */
    vTaskStartScheduler();

    while (1)
        ;
}

// https://www.segger.com/downloads/application-notes/AN00016
void HardFault_Handler(void)
{
    asm volatile("   movs R0, #4   \n"
                 "   mov R1, LR    \n"
                 "   tst R0, R1    \n" // Check EXC_RETURN in Link register bit 2.
                 "   bne usepsp    \n"
                 "   mrs R0, MSP   \n" // Stacking was using MSP.
                 "   b common      \n"
                 "usepsp:          \n"
                 "   mrs R0, PSP   \n" // Stacking was using PSP.
                 "common:          \n"
                 "   ldr R2,=CHardFaultHandler \n"
                 "   bx R2         \n" // Stack pointer passed through R0.
    );
}

/***********************************************************************
 * Functions:   Defininition of local functions
***********************************************************************/
void debugTask(void *pvParameters)
{
    (void)pvParameters;
    while (1)
    {
        const semInfo_t volatile *pSemaphoreInfo = &globals.semaphoreInfo;
        OSDELAY_MS(500)
        (void)pSemaphoreInfo; // view info here
        taskYIELD();          // set breakpoint here to see pSemaphoreInfo in debugger
    }
}

void CHardFaultHandler(uint32_t *rsp)
{
    faultRegs_t volatile t;
    t.r0 = rsp[0]; // for debugger
    t.r1 = rsp[1];
    t.r2 = rsp[2];
    t.r3 = rsp[3];
    t.r12 = rsp[4];
    t.lr = rsp[5];
    t.pc = rsp[6];
    t.psr = rsp[7];
    (void)t;

    uint8_t cf = (uint8_t)globals.hardFaultTestFlags;
    while ((cf & 0x01) == 0)
        continue; // loop forever here

    uint8_t ff = cf >> 4;
    uint8_t pci = cf & 0x0E; // always even
    if (ff & (1 << 0))
        rsp[0] = (uint8_t)(globals.hardFaultTestFlags >> 8);
    if (ff & (1 << 1))
        rsp[6] += pci;
    globals.hardFaultTestFlags = 0;
}
