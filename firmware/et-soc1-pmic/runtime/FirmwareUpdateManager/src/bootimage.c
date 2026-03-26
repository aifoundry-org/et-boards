/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file bootimage.c
    \brief Bootloader/image update related functions
*/
/***********************************************************************/

#include "system_clock.h"
#include "bootimage.h"
#include "commitinfo.h"
#include "CLITask.h"
#include "etsoc_cmd_handler_task.h"
#include "IoTask.h"
#include "flash_access.h"
#include "config.h"
#include "error_codes.h"
#include "image_checksum.h"
#include "common_defs.h"
#include "image_metadata.h"
#include "hw_encoding.h"

// clang-format off

/***********************************************************************
 * Data types:      Defininition of local data types
***********************************************************************/
enum {
    E_SC_dataread = 0,
    E_SC_update = 1,
    E_SC_boardtype = 2,
    E_SC_chksumread = 3,
    E_SC_hashread = 4,
    E_SC_versionbytes = 5,
    E_SC_address = 6,
    E_SC_currentSlot = 7,
    E_SC_setDfltBootSlot = 8,
    E_SC_setBootCounter = 9,
    E_SC_blVersion = 10,
    E_SC_metadataVersion = 11,
};

enum {
    E_FUB_subcommandBeg = 0,
    E_FUB_subcommandEnd = 4,
    E_FUB_slot = E_FUB_subcommandEnd,
    E_FUB_updating = 5,
    E_FUB_illegal = 6,
    E_FUB_busy = 7,
};

typedef union {
    struct {
        uint8_t subcommand : 4;
        uint8_t slot : 1;
        uint8_t updating : 1;
        uint8_t illegal : 1;
        uint8_t busy : 1;
    } bit;
    uint8_t reg;
} flashUpdate_t;

// clang-format on

/***********************************************************************
 * Variables:  Defininition of local variables and constants
***********************************************************************/
uint32_t *flashAddress = (uint32_t *)SLOT_0_IMGOFFSET;
//double buffer used to store one page of data before it's written to flash memory
uint32_t fwUpdateBuffer[2][NVM_PAGE_WORDS];
static uint32_t fwUpdateBufActiveIndex = 0;
static uint32_t wordCnt = 0;
static uint32_t fw_data_size = 0;
static uint8_t oldUpdateCmd = 0;
static bool checksumDone = false;

/***********************************************************************
 * Functions:   Declaration of local functions
***********************************************************************/
static void setIllegal(void);
static int bgStartUpdate(void);
static int bgWordUpdate(void);
static int bgCompleteUpdate(void);
static void wordUpdate(uint32_t data);
static void addressUpdate(const flashUpdate_t *pFflashUpdate, uint32_t data);
static void fwUpdateManagerSendEvent(fwUpdateManagerEvent_t fwUpdateManagerEvent);
static uint32_t doChecksum(void);
static bool isRequestedUpdateSlotAllowed(uint8_t slot);

/***********************************************************************
 * GLOBAL Functions:   Defininition of global functions
***********************************************************************/
void fwUpdateManagerTask(void *pvParameters)
{
    (void)pvParameters;
    int status = STATUS_SUCCESS;
    fwUpdateManagerEvent_t fwUpdateManagerEvent;
    flashUpdate_t *pFflashUpdate =
        (flashUpdate_t *)(getRegisterAddress(FW_UPDATE_CMD)); //todo: don't use direct access to buffer
    while (1)
    {
        status = STATUS_SUCCESS;
        if (xQueueReceive(globals.fwUpdateManagerTaskQueueHandle, &fwUpdateManagerEvent, portMAX_DELAY) != pdPASS)
        {
            status = FW_UPDATE_ERROR_QUEUE_RECEIVE;
        }

        pFflashUpdate->bit.busy = 1;
        switch (fwUpdateManagerEvent.eventType)
        {
            case E_BC_startUpdate:
                status = bgStartUpdate();
                break;
            case E_BC_completeUpdate:
                status = bgCompleteUpdate();
                break;
            case E_BC_wordUpdate:
                status = bgWordUpdate();
                break;
            case E_BC_chksumread:
                status = setRegisterValue(FW_UPDATE_DATA, doChecksum());
                break;
            case E_BC_setDfltBootSlot:
                status = Config_Set_Boot_Slot(fwUpdateManagerEvent.data);
                interruptingPrintf("After cmd set boot slot to %d: boot slot = %d, failed boot cnt = %d\n",
                    fwUpdateManagerEvent.data, Config_Get_Boot_Slot_From_Config_Header(),
                    Config_Get_Failed_Boot_Cnt_From_Config_Header());
                break;
            case E_BC_setBootCounter:
                status = Config_Set_Failed_Boot_Cnt(fwUpdateManagerEvent.data);
                interruptingPrintf("After cmd set failed boot cnt to %d: boot slot = %d, failed boot cnt = %d\n",
                    fwUpdateManagerEvent.data, Config_Get_Boot_Slot_From_Config_Header(),
                    Config_Get_Failed_Boot_Cnt_From_Config_Header());
                break;
            default:
                status = FW_UPDATE_ERROR_INVALID_CMD;
                break;
        }
        pFflashUpdate->bit.busy = 0;

        if (status != STATUS_SUCCESS)
        {
            interruptingPrintf("%s: Error %d\n", __FUNCTION__, status);
        }
    }
}

/**
 * @fn bootToImage
 * @brief Forced one time reboot to selected image target.
 * 
 * Boot target is selected in bootloader by passing corresponding magic number.
 * Boot slot in config header is not changed.
 *
 * @param[in] target 0/1/2 = bootloader/slot0/slot1
 */
void bootToImage(bootloaderTarget_t target)
{
    cmdPowerOff(&globals.commonData.powerState);
    interruptingPrintfOneLine("Entering Bootloader");

    if (target > BT_unsupported)
        return;

    OSDELAY_MS(500)

    disable_irq();
    systemDisableHwInterrupts();

    // move system clock away from GCLK_GENCTRL_SRC_DFLL48M which will be reprogrammed
    // to the stable GCLK_GENCTRL_SRC_OSC8M
    gclk_write_GENDIV(GCLK_GENDIV_ID_GCLK0 | GCLK_GENDIV_DIV(1));
    gclk_write_GENCTRL(GCLK_GENCTRL_ID_GCLK0 | GCLK_GENCTRL_GENEN | GCLK_GENCTRL_SRC_OSC8M);
    gclk_wait_for_sync();

    register uint32_t const *ivtCopyt = 0;
    register uint32_t stackStart = ivtCopyt[0];
    register uint32_t resetVector = ivtCopyt[1];

    __set_MSP(stackStart);
    asm volatile("movs r0, #0       \n" /* Switch to the msp stack. */
                 "msr  CONTROL, r0  \n");
    ((void (*)(uint32_t))resetVector)(BLMAGICNUM + target);
}

int wrUpdateCmd(ETSOCVirtualRegisterIdx_t regIdx)
{
    //called in context of etsocCmdHandlerTask
    int status = STATUS_SUCCESS;
    fwUpdateManagerEvent_t fwUpdateManagerEvent;

    flashUpdate_t *pFflashUpdate = (flashUpdate_t *)(getRegisterAddress(regIdx));

    if (pFflashUpdate->bit.illegal ||                 // illegal or...
        (pFflashUpdate->bit.subcommand == E_SC_update // update subcommand and...
            && pFflashUpdate->bit.updating))          // already updating
    {
        setIllegal();
        return GENERAL_ERROR;
    }

    // deal with completing update
    if (pFflashUpdate->bit.subcommand != E_SC_update && pFflashUpdate->bit.updating)
    {
        printfFromInterruptNoFlush("completeUpdate %08X, status %02X", (uint32_t)flashAddress, pFflashUpdate->reg);

        //write remaining data from buffer
        if (wordCnt != 0)
        {
            for (uint32_t i = 0; i < wordCnt; i++)
            {
                *flashAddress++ = fwUpdateBuffer[fwUpdateBufActiveIndex][i]; //write data to page buffer
            }

            fwUpdateManagerEvent.eventType = E_BC_completeUpdate;
            fwUpdateManagerSendEvent(fwUpdateManagerEvent);
        }

        /* calculate fw data size which has been transfered */
        fw_data_size = (uint32_t)(
            (uint8_t *)flashAddress - (uint8_t *)(pFflashUpdate->bit.slot ? SLOT_1_IMGOFFSET : SLOT_0_IMGOFFSET));

        pFflashUpdate->bit.updating = 0;
        checksumDone = false;
    }

    // deal with changing slot
    if ((oldUpdateCmd ^ pFflashUpdate->reg) & (1 << E_FUB_slot))
    {
        flashAddress = (uint32_t *)(pFflashUpdate->bit.slot ? SLOT_1_IMGOFFSET : SLOT_0_IMGOFFSET);
        checksumDone = false;
        fw_data_size = Config_Get_Image_Size_From_Metadata(pFflashUpdate->bit.slot);
    }

    // deal with setting data read value, starting update and auto-setting starting address
    switch (pFflashUpdate->bit.subcommand)
    {
        case E_SC_dataread:
            setRegisterValue(FW_UPDATE_DATA, *flashAddress);
            break;
        case E_SC_update:
            if (!isRequestedUpdateSlotAllowed(pFflashUpdate->bit.slot))
            {
                setIllegal();
                return GENERAL_ERROR;
            }
            checksumDone = false;
            fw_data_size = 0;
            wordCnt = 0;
            fwUpdateBufActiveIndex = 0;
            flashAddress = (uint32_t *)(pFflashUpdate->bit.slot ? SLOT_1_IMGOFFSET : SLOT_0_IMGOFFSET);
            pFflashUpdate->bit.updating = 1;
            fwUpdateManagerEvent.eventType = E_BC_startUpdate;
            fwUpdateManagerSendEvent(fwUpdateManagerEvent);
            break;
        case E_SC_boardtype:
            status = setRegisterValue(FW_UPDATE_DATA, (1 << Hw_Encoding_Get_Hw_Version_Encoding()));
            break;
        case E_SC_chksumread:
            fwUpdateManagerEvent.eventType = E_BC_chksumread;
            fwUpdateManagerSendEvent(fwUpdateManagerEvent);
            break;
        case E_SC_hashread:
        {
            const char *hash = Config_Get_Image_Hash_From_Metadata(pFflashUpdate->bit.slot);
            /* Send the 4 bytes of hash */
            status = setRegisterValue(FW_UPDATE_DATA,
                ((uint8_t)hash[3] << 24) | ((uint8_t)hash[2] << 16) | ((uint8_t)hash[1] << 8) | (uint8_t)hash[0]);
            break;
        }
        case E_SC_versionbytes:
            status = setRegisterValue(FW_UPDATE_DATA, Config_Get_Fw_Ver_From_Metadata(pFflashUpdate->bit.slot));
            break;
        case E_SC_address:
            status = setRegisterValue(FW_UPDATE_DATA, (uint32_t)flashAddress);
            break;
        case E_SC_currentSlot:
            status = setRegisterValue(FW_UPDATE_DATA, BOOT_IMAGE_SLOT);
            break;
        case E_SC_setDfltBootSlot:
            status = setRegisterValue(FW_UPDATE_DATA, Config_Get_Boot_Slot_From_Config_Header());
            break;
        case E_SC_setBootCounter:
            status = setRegisterValue(FW_UPDATE_DATA, Config_Get_Failed_Boot_Cnt_From_Config_Header());
            break;
        case E_SC_blVersion:
            status = setRegisterValue(FW_UPDATE_DATA, Config_Get_Curr_Bl_Ver());
            break;
        case E_SC_metadataVersion:
            status = setRegisterValue(FW_UPDATE_DATA, Config_Get_Metadata_Ver(pFflashUpdate->bit.slot));
            break;
        default:
            break;
    }
    oldUpdateCmd = pFflashUpdate->reg;

    return status;
}

void wrUpdateData(uint32_t data)
{
    //called in context of etsocCmdHandlerTask

    fwUpdateManagerEvent_t fwUpdateManagerEvent;
    const flashUpdate_t *pFflashUpdate = (flashUpdate_t *)(getRegisterAddress(FW_UPDATE_CMD));
    if (pFflashUpdate->bit.illegal)
    {
        printfFromInterruptNoFlush(
            "enter wrUpdateData, when ERROR, in cmd %02X, status %02X", pFflashUpdate->reg, pFflashUpdate->reg);
        setIllegal();
        return;
    }
    switch (pFflashUpdate->bit.subcommand)
    {
        case E_SC_update:
            wordUpdate(data); // write data to flash
            break;
        case E_SC_address:
            addressUpdate(pFflashUpdate, data);
            break;
        case E_SC_setDfltBootSlot:
            fwUpdateManagerEvent.eventType = E_BC_setDfltBootSlot;
            fwUpdateManagerEvent.data = data;
            //perserve updated boot slot by writing to flash
            fwUpdateManagerSendEvent(fwUpdateManagerEvent);
            break;
        case E_SC_setBootCounter:
            fwUpdateManagerEvent.eventType = E_BC_setBootCounter;
            fwUpdateManagerEvent.data = data; //perserve updated boot count by writing to flash
            fwUpdateManagerSendEvent(fwUpdateManagerEvent);
            break;
        default: // write when read only  --> illegal
            setIllegal();
            break;
    }
}

void rdUpdatePostCmd(void)
{
    //called in context of etsocCmdHandlerTask

    flashUpdate_t *pFflashUpdate = (flashUpdate_t *)(getRegisterAddress(FW_UPDATE_CMD));
    pFflashUpdate->bit.illegal = 0;
}

void rdUpdatePostData(void)
{
    //called in context of etsocCmdHandlerTask

    const flashUpdate_t *pFflashUpdate = (flashUpdate_t *)(getRegisterAddress(FW_UPDATE_CMD));
    if (pFflashUpdate->bit.subcommand == E_SC_dataread)
    {
        setRegisterValue(FW_UPDATE_DATA, *(++flashAddress));
    }
}

/***********************************************************************
 * Functions:   Defininition of local functions
***********************************************************************/
static void setIllegal(void)
{
    //called in wrUpdateCmd() and wrUpdateData()
    //which are called in context of etsocCmdHandlerTask

    flashUpdate_t *pFflashUpdate = (flashUpdate_t *)(getRegisterAddress(FW_UPDATE_CMD));

    interruptingPrintfOneLine("Bootimage: illegal request status = %02X", pFflashUpdate->reg);

    pFflashUpdate->reg &= 0x10; // leave slot bit
    pFflashUpdate->bit.illegal = 1;
}

static void fwUpdateManagerSendEvent(fwUpdateManagerEvent_t fwUpdateManagerEvent)
{
    //called in context of etsocCmdHandlerTask

    flashUpdate_t *pFflashUpdate =
        (flashUpdate_t *)(getRegisterAddress(FW_UPDATE_CMD)); //todo: don't use direct access to buffer
    if (uxQueueMessagesWaiting(globals.fwUpdateManagerTaskQueueHandle) == FW_UPDATE_MANAGER_TASK_QUEUE_LENGTH)
    {
        printfFromInterrupt("UpdateTask task queue full (%d), event can't be handled",
            uxQueueMessagesWaiting(globals.fwUpdateManagerTaskQueueHandle), 0);
        pFflashUpdate->bit.illegal = 1;
        return;
    }

    pFflashUpdate->bit.busy = 1;
    xQueueSendToBack(globals.fwUpdateManagerTaskQueueHandle, &fwUpdateManagerEvent, portMAX_DELAY);
}

static void wordUpdate(uint32_t data)
{
    //called in wrUpdateData()
    //which is called in context of etsocCmdHandlerTask

    fwUpdateManagerEvent_t fwUpdateManagerEvent;

    checksumDone = false;

    fwUpdateBuffer[fwUpdateBufActiveIndex][wordCnt++] = data;

    if (wordCnt == NVM_PAGE_WORDS)
    {
        for (uint32_t i = 0; i < NVM_PAGE_WORDS; i++)
        {
            //write data to page buffer
            *flashAddress++ = fwUpdateBuffer[fwUpdateBufActiveIndex][i];
        }
        //queue NVM command to write one page to flash
        fwUpdateManagerEvent.eventType = E_BC_wordUpdate;
        fwUpdateManagerSendEvent(fwUpdateManagerEvent);
        //update page word counter and active buffer index
        wordCnt = 0;
        fwUpdateBufActiveIndex ^= 1;
    }
}

static void addressUpdate(const flashUpdate_t *pFflashUpdate, uint32_t data)
{
    uint32_t imageOffset = pFflashUpdate->bit.slot ? SLOT_1_IMGOFFSET : SLOT_0_IMGOFFSET;
    if (imageOffset > data || data >= (imageOffset + IMAGESIZE))
    {
        setIllegal();
        return;
    }

    flashAddress = (uint32_t *)data; // set address
}

static int bgStartUpdate(void)
{
    int status = STATUS_SUCCESS;
    printfFromInterruptNoFlush("enter bgStartUpdate %08X", (uint32_t)flashAddress, 0);

    // erase first row
    status = Flash_Access_Erase_Row(flashAddress);
    if (status != STATUS_SUCCESS)
    {
        printfFromInterruptNoFlush("bgStartUpdate: error erasing flash at %08X", (uint32_t)flashAddress, 0);
    }

    return status;
}

static int bgWordUpdate(void)
{
    int status = FW_UPDATE_ERROR_ADDR_NOT_ALIGNED;
    if (Flash_Access_Is_Addr_Page_Aligned((uint32_t)flashAddress))
    {
        status = Flash_Access_Write_Page();
        if (status == STATUS_SUCCESS)
        {
            const flashUpdate_t *pFflashUpdate =
                (flashUpdate_t *)(getRegisterAddress(FW_UPDATE_CMD)); //todo: don't use direct access to buffer
            uint32_t imageEndOffset = pFflashUpdate->bit.slot ? (SLOT_1_IMGOFFSET + IMAGESIZE) :
                                                                (SLOT_0_IMGOFFSET + IMAGESIZE);
            if (Flash_Access_Is_Row_Erase_Required((uint32_t)flashAddress, imageEndOffset))
            {
                status = Flash_Access_Erase_Row(flashAddress);
            }

            if (status != STATUS_SUCCESS)
            {
                printfFromInterruptNoFlush("wordUpdate: error %d erasing flash at %p", status, (uint32_t)flashAddress);
            }
        }
        else
        {
            printfFromInterruptNoFlush(
                "wordUpdate: error: %d programming flash at %08X", status, (uint32_t)flashAddress);
        }
    }

    return status;
}

static int bgCompleteUpdate(void)
{
    int status = STATUS_SUCCESS;

    printfFromInterruptNoFlush("pageWrite %08X", (uint32_t)flashAddress, 0);

    status = Flash_Access_Write_Page();
    if (status != STATUS_SUCCESS)
    {
        printfFromInterruptNoFlush("bgCompleteUpdate: error programming flash at %08X", (uint32_t)flashAddress, 0);
    }

    return status;
}

static uint32_t doChecksum(void)
{
    static bool checksumOk = 0;
    int status = STATUS_SUCCESS;
    const flashUpdate_t *pFflashUpdate =
        (flashUpdate_t *)(getRegisterAddress(FW_UPDATE_CMD)); //todo: don't use direct access to buffer
    const uint32_t *fwData = (uint32_t *)(pFflashUpdate->bit.slot ? SLOT_1_IMGOFFSET : SLOT_0_IMGOFFSET);
    if ((fw_data_size >= IMAGESIZE) || (fw_data_size != Config_Get_Image_Size_From_Metadata(pFflashUpdate->bit.slot)))
    {
        return FW_UPDATE_ERROR_INVALID_DATA_SIZE;
    }

    if (!checksumDone)
    {
        uint32_t sum = 0;
        status = calculateChecksum(fwData, fw_data_size, &sum);
        if (status == STATUS_SUCCESS)
        {
            printfFromInterruptNoFlush("checksum = 0x%x", sum, 0);
            checksumOk = (sum == 0); //checksum complement is used, so total sum computes 0 if correct
            checksumDone = true;
        }
        else
        {
            printfFromInterruptNoFlush("doChecksum error %d", status, 0);
        }
    }
    return checksumOk;
}

static bool isRequestedUpdateSlotAllowed(uint8_t slot)
{
    if (BOOT_IMAGE_SLOT == BOOT_STANDALONE)
    {
        printfFromInterruptNoFlush("Standalone image, OTA updates are not supported", 0, 0);
        return false;
    }
    else if (BOOT_IMAGE_SLOT == slot)
    {
        printfFromInterruptNoFlush("Requested slot to update %d is equal to current slot %d", slot, BOOT_IMAGE_SLOT);
        return false;
    }
    else
    {
        return true;
    }
}

