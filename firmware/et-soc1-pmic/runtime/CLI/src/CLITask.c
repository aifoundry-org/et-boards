/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file CLITask.c
    \brief C file for command line interface
*/
/***********************************************************************/

#define INSTANTIATE_DEVINFO

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <compiler.h>
#include <string.h>
#include <stdarg.h>

#include "main.h"
#include "board_defs.h"
#include "FreeRTOS.h"
#include "hooks.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "features.h"
#include "globals.h"
#include "gpio.h"
#include "console.h"
#include "utils.h"
#include "system_clock.h"
#include "etsoc_cmd_handler_task.h"
#include "etsoc_cmd_handler_event.h"
#include "CLITask.h"
#include "cli.h"
#include "vreg.h"
#include "IoTask.h"
#include "i2cm.h"
#include "chipio.h"
#include "fruinfo.h"
#include "boardchipinfo.h"
#include "scripts.h"
#include "bootimage.h"
#include "flash_access.h"
#include "commitinfo.h"
#include "i2cs.h"
#include "error_codes.h"
#include "image_checksum.h"
#include "config.h"
#include "version.h"
#include "image_metadata.h"
#include "hw_encoding.h"

#define SMOKE_ASSERT(condition, msg)                                 \
    if (!(condition))                                                \
    {                                                                \
        interruptingPrintfOneLine(" SMOKE TEST FAILED at: %s", msg); \
        return;                                                      \
    }

#define CHECK_VALUE_WITH_TOLERANCE(value, ref, resolution_mul, resolution_div) \
    ((value <= ((ref * resolution_div + resolution_mul) / resolution_div)) &&  \
        (value >= ((ref * resolution_div - resolution_mul) / resolution_div)))

#define VERIFY_VOLTAGE(id, value, ref, resolution_mul, resolution_div)                                                \
    do                                                                                                                \
    {                                                                                                                 \
        interruptingPrintf("%s: set: Voltage %d read voltage: %d.\n", id, value, ref);                                \
        SMOKE_ASSERT(CHECK_VALUE_WITH_TOLERANCE(value, ref, resolution_mul, resolution_div) == true, "Set voltage "); \
    } while (0);

#define MINION_BOOT_VOLTAGE 0x32
#define SRAM_BOOT_VOLTAGE   0x64
#define NOC_BOOT_VOLTAGE    0x2D
#define DDR_BOOT_VOLTAGE    0x6E
#define MXN_BOOT_VOLTAGE    0x46

#define CHECK_ETSOC_RESET_STATE()                                                                   \
    do                                                                                              \
    {                                                                                               \
        SMOKE_ASSERT(getRegisterValue(getRegisterIndexFromSpCmdIndex(0x19)) == SRAM_BOOT_VOLTAGE,   \
            "Reset SOC cmd - set register 0x19");                                                   \
        SMOKE_ASSERT(getRegisterValue(getRegisterIndexFromSpCmdIndex(0x1A)) == DDR_BOOT_VOLTAGE,    \
            "Reset SOC cmd - set register 0x1A");                                                   \
        SMOKE_ASSERT(getRegisterValue(getRegisterIndexFromSpCmdIndex(0x1E)) == MXN_BOOT_VOLTAGE,    \
            "Reset SOC cmd - set register 0x1E");                                                   \
        SMOKE_ASSERT(getRegisterValue(getRegisterIndexFromSpCmdIndex(0x1F)) == NOC_BOOT_VOLTAGE,    \
            "Reset SOC cmd - set register 0x1F");                                                   \
        SMOKE_ASSERT(getRegisterValue(getRegisterIndexFromSpCmdIndex(0x20)) == MINION_BOOT_VOLTAGE, \
            "Reset SOC cmd - set register 0x20");                                                   \
    } while (0);

/* Device serial number */
#define DEVICE_SERIAL_W0 0x0080A00C
#define DEVICE_SERIAL_W1 0x0080A040
#define DEVICE_SERIAL_W2 0x0080A044
#define DEVICE_SERIAL_W3 0x0080A048

typedef struct {
    uint32_t w[4];
} t_uniqueCpuId;

void cmdSmokeTest(const char *itr, uint8_t *pMenuType, const t_cprf *pCprf, powerState_t *pps);
void cmdSwitchSlot(const char *slot_num);

static bool checkSum(uint32_t const *sa, uint32_t slot)
{
    bool result = false;
    uint32_t sum = 0;
    uint32_t length = Config_Get_Image_Size_From_Metadata(slot);
    if (length >= IMAGESIZE || length < 0x100)
    {
        return false;
    }

    if (calculateChecksum(sa, length, &sum) == STATUS_SUCCESS)
    {
        result = (sum == 0);
    }

    return result;
}

static t_uniqueCpuId *uniqueCpuId(void)
{
    static uint32_t const *addrs[4] = {
        (uint32_t const *)DEVICE_SERIAL_W0,
        (uint32_t const *)DEVICE_SERIAL_W1,
        (uint32_t const *)DEVICE_SERIAL_W2,
        (uint32_t const *)DEVICE_SERIAL_W3,
    };
    static t_uniqueCpuId id;
    for (uint32_t i = 0; i < countof(addrs); ++i)
    {
        id.w[i] = *addrs[i];
    }
    return &id;
}

static void printMetadataInfo(uint32_t imageSlot);
static void printMetadataInfo(uint32_t imageSlot)
{
    interruptingPrintf("\tFirmware version = %d.%d.%d\n", GET_VERSION_MAJOR(Config_Get_Fw_Ver_From_Metadata(imageSlot)),
        GET_VERSION_MINOR(Config_Get_Fw_Ver_From_Metadata(imageSlot)),
        GET_VERSION_PATCH(Config_Get_Fw_Ver_From_Metadata(imageSlot)));
    interruptingPrintf("\tSupported board types = 0x%x\n", Config_Get_Supported_Board_Types_From_Metadata(imageSlot));
    interruptingPrintf("\tGit commit = %s\n", Config_Get_Image_Hash_From_Metadata(imageSlot));
    interruptingPrintf("\tChecksum = 0x%x\n", Config_Get_Checksum_From_Metadata(imageSlot));
    interruptingPrintf("\tImage size = %d\n", Config_Get_Image_Size_From_Metadata(imageSlot));
    interruptingPrintf("\tCompatible bootloader version = %d.%d.%d\n",
        GET_VERSION_MAJOR(Config_Get_Compatible_Bl_Ver_From_Metadata(imageSlot)),
        GET_VERSION_MINOR(Config_Get_Compatible_Bl_Ver_From_Metadata(imageSlot)),
        GET_VERSION_PATCH(Config_Get_Compatible_Bl_Ver_From_Metadata(imageSlot)));
    interruptingPrintf("\tSP-PMIC interface version = %d.%d.%d\n",
        GET_VERSION_MAJOR(Config_Get_Sp_Pmic_Interface_Ver_From_Metadata(imageSlot)),
        GET_VERSION_MINOR(Config_Get_Sp_Pmic_Interface_Ver_From_Metadata(imageSlot)),
        GET_VERSION_PATCH(Config_Get_Sp_Pmic_Interface_Ver_From_Metadata(imageSlot)));
    interruptingPrintf("\tMetadata version = %d.%d.%d\n", GET_VERSION_MAJOR(Config_Get_Metadata_Ver(imageSlot)),
        GET_VERSION_MINOR(Config_Get_Metadata_Ver(imageSlot)), GET_VERSION_PATCH(Config_Get_Metadata_Ver(imageSlot)));
    uint32_t buildType = Config_Get_Build_Type_From_Metadata(imageSlot);
    interruptingPrintf("\tBuild type = %s\n", Image_Metadata_Decode_Build_Type((buildType_t)buildType));
}

void cmdVersion(const char *p)
{
    uint8_t mode = 0;
    if (!p)
    {
        interruptingPrintf(
            "ver 0 - summary\nver 1 - commit info\nver 2 - post commit diffs\nver 3 - CPU info\nver 4 - metadata and config header info\n\n");
    }
    else
    {
        bool ok = 1;
        int v = atodec(p, &ok);
        if (!ok || ((v < 0) || (v > 4)))
        {
            interruptingPrintf("must be 0, 1, 2, 3, or 4");
            return;
        }
        mode = (uint8_t)v;
    }

    uint32_t devsig = DSU->DID.reg;
    if (mode == 3)
    {
        uint32_t const *pUniqueCpuId = &uniqueCpuId()->w[0];
        interruptingPrintf("CPU signature %08X\nCPU unique ID", devsig);
        uint32_t cpuHash = 0;
        for (uint32_t i = 0; i < 4; ++i)
        {
            uint32_t t = *pUniqueCpuId++;
            interruptingPrintf(" %08X", t);
            cpuHash ^= t;
        }
        interruptingPrintf("\nCPU ID hash: %08X\n", cpuHash);
        return;
    }

    /* Display the current version of running firmware */
    uint32_t currentVer = Image_Metadata_Get_Curr_Fw_Ver();
    interruptingPrintf("Build type: %s, FW Version: v%d.%d.%d\n\n",
        Image_Metadata_Decode_Build_Type(Image_Metadata_Get_Curr_Build_Type()), GET_VERSION_MAJOR(currentVer),
        GET_VERSION_MINOR(currentVer), GET_VERSION_PATCH(currentVer));

    uint32_t metadataVer = Config_Get_Metadata_Ver(BOOT_IMAGE_SLOT);
    interruptingPrintf("Metadata version: v%d.%d.%d\n", GET_VERSION_MAJOR(metadataVer), GET_VERSION_MINOR(metadataVer),
        GET_VERSION_PATCH(metadataVer));

    uint32_t compatibleBlVer = Config_Get_Compatible_Bl_Ver_From_Metadata(BOOT_IMAGE_SLOT);
    interruptingPrintf("Compatible BL version: v%d.%d.%d\n", GET_VERSION_MAJOR(compatibleBlVer),
        GET_VERSION_MINOR(compatibleBlVer), GET_VERSION_PATCH(compatibleBlVer));

    uint32_t spPmicInterfaceVer = Config_Get_Sp_Pmic_Interface_Ver_From_Metadata(BOOT_IMAGE_SLOT);
    interruptingPrintf("SP-PMIC interface version: v%d.%d.%d\n", GET_VERSION_MAJOR(spPmicInterfaceVer),
        GET_VERSION_MINOR(spPmicInterfaceVer), GET_VERSION_PATCH(spPmicInterfaceVer));

    uint32_t designRevision = Hw_Encoding_Get_Hw_Design_Revision();
    interruptingPrintf("Platform: %s, REV %d, processor type: J%d\n", Hw_Encoding_Get_Hw_Board_Name(), designRevision,
        18 - (devsig & 0xFF));

    bool isBootloaderPresent = (BOOT_IMAGE_SLOT != BOOT_STANDALONE) ? true : false;
    if (isBootloaderPresent)
    {
        interruptingPrintf("%-20sSLOT_%d\n%-20s", "Image type:", BOOT_IMAGE_SLOT, "Checksum");
        const uint32_t *sa = ((uint32_t *)(START_OFFSET));
        interruptingPrintf(checkSum(sa, BOOT_IMAGE_SLOT) ? "Valid" : "Invalid");
        interruptingPrintf("\r\n\n");

        bool imageSlot0Present = checkSum((uint32_t const *)SLOT_0_IMGOFFSET, BOOT_SLOT_0);
        bool imageSlot1Present = checkSum((uint32_t const *)SLOT_1_IMGOFFSET, BOOT_SLOT_1);

        interruptingPrintf("bootloader   present TRUE\nSLOT_0 image present %s\nSLOT_1 image present %s\n\n",
            imageSlot0Present ? "TRUE" : "FALSE", imageSlot1Present ? "TRUE" : "FALSE");

        if (mode == 4)
        {
            interruptingPrintf("Current bootloader version = %d.%d.%d\n", GET_VERSION_MAJOR(Config_Get_Curr_Bl_Ver()),
                GET_VERSION_MINOR(Config_Get_Curr_Bl_Ver()), GET_VERSION_PATCH(Config_Get_Curr_Bl_Ver()));
            interruptingPrintf("Config header: boot slot = %d\n", Config_Get_Boot_Slot_From_Config_Header());
            interruptingPrintf(
                "Config header: failed boot counter = %d\n", Config_Get_Failed_Boot_Cnt_From_Config_Header());

            interruptingPrintf("SLOT_0 metadata%s", imageSlot0Present ? ":\n" : " (not present):\n");
            printMetadataInfo(BOOT_SLOT_0);

            interruptingPrintf("SLOT_1 metadata%s", imageSlot1Present ? ":\n" : " (not present):\n");
            printMetadataInfo(BOOT_SLOT_1);
            interruptingPrintf("\n");
        }
    }
    else //standalone image
    {
        interruptingPrintf("%-20sSTANDALONE\n%-20s", "Image type:", "Checksum");
        const uint32_t *sa = ((uint32_t *)(STANDALONE_IMGOFFSET));
        interruptingPrintf(checkSum(sa, BOOT_IMAGE_SLOT) ? "Valid" : "Invalid");
        interruptingPrintf("\r\n\n");

        bool imagePresent = checkSum((uint32_t const *)STANDALONE_IMGOFFSET, BOOT_STANDALONE);

        interruptingPrintf(
            "bootloader       present FALSE\nSTANDALONE image present %s\n\n", imagePresent ? "TRUE" : "FALSE");

        interruptingPrintf("Metadata%s", imagePresent ? ":\n" : " (not present):\n");
        printMetadataInfo(BOOT_IMAGE_SLOT);
        interruptingPrintf("\n");
    }

    interruptingPrintf("Commit info: %s, branch: %s, hash: %s\n", LASTCOMMITTIME, CURRENT_BRANCH,
        Config_Get_Image_Hash_From_Metadata(BOOT_IMAGE_SLOT));

    if (mode == 0 || mode == 4)
    {
        return;
    }
    if (mode < 2)
    {
        if (UNCOMMITTED_CHANGES)
        {
            interruptingPrintf("!! Changes since last commit !!\n");
        }
        else
        {
            interruptingPrintf("No changes since last commit.\n");
        }
        if (mode == 1)
        {
            interruptingPrintf("LASTCOMMITMSG:  %s\n", LASTCOMMITMSG);
            interruptingPrintf("LASTCOMMITID:   %s\n", LASTCOMMITID);
            interruptingPrintf("COMPILETIME:    %s\n", COMPILETIME);
        }
    }
    else
    {
        interruptingPrintf("%s", diffinfo);
    }
}

void cmdStack(void)
{
    interruptingPrintBegin();
    for (stackInfo_t const *pi = (stackInfo_t const *)&allStacksInfo;
         pi < &((stackInfo_t const *)&allStacksInfo)[sizeof(allStacksInfo_t) / sizeof(stackInfo_t)]; ++pi)
    {
        const uint32_t *pd = pi->beg;
        while (pd < pi->end)
        {
            if (*(++pd) != 0xA5A5A5A5)
                break;
        }
        interruptingPrintf("%-8s: %3d unused uint32 out of %3d (E:%08x, B:%08x)\n", pi->name, pd - pi->beg - 1,
            pi->end - pi->beg, pi->beg, pi->end);
    }
    interruptingPrintEnd();
}

void doPrompt(char const *prompt)
{
    if (globals.cli.promptState != PS_needed)
        return;
    printf("\n%s", prompt);
    globals.cli.promptState = PS_none;
}

static void donePrompt(void)
{
    globals.cli.promptState =
        globals.cli.promptState == PS_alreadyThere || globals.cli.promptState == PS_interrupted ? PS_none : PS_needed;
}

static void interruptingPrefix(void)
{
    const threadLocal_t *pThreadLocal = pvTaskGetThreadLocalStoragePointer(NULL, 0);
    if (pThreadLocal->threadId != TH_CLI)
        printf("*%s:  ", pThreadLocal->threadName);
}

void interruptingPrintBegin()
{
    const threadLocal_t *pThreadLocal = pvTaskGetThreadLocalStoragePointer(NULL, 0);

    if (pThreadLocal->threadId == TH_CLI)
    {
        return;
    }

    if (globals.cli.intrPrintDepth++ > 0)
        return;

    if (pThreadLocal->threadId != TH_CLI)
    { // CLI externally enclose all commands in printSemaphore
        PRINTSEM_TAKE
    }
    undoGetline();
    if (globals.cli.promptState != PS_interrupted)
    {
        globals.cli.promptState = PS_interrupted;
        puts("++++++++++\n");
    }
    globals.cli.needPrefix = 1;
}

void interruptingPrintEnd(void)
{
    const threadLocal_t *pThreadLocal = pvTaskGetThreadLocalStoragePointer(NULL, 0);

    if (pThreadLocal->threadId == TH_CLI)
    {
        return;
    }

    if (--globals.cli.intrPrintDepth > 0)
        return;

    puts("\n");

    restoreGetline();
    globals.cli.promptState = globals.cli.promptState == PS_interrupted ? PS_interrupted : PS_alreadyThere;
    globals.cli.needPrefix = 0;
    if (pThreadLocal->threadId != TH_CLI)
    {
        PRINTSEM_GIVE
    }
}

static void afterNlHook(char ch)
{
    if (ch > 0)
    {
        if (globals.cli.needPrefix)
        {
            interruptingPrefix();
            globals.cli.needPrefix = 0;
        }
        globals.cli.needPrefix = ch == '\n';
    }
}

static void interruptingPrintfArgs(const char *format, va_list args)
{
    const threadLocal_t *pThreadLocal = pvTaskGetThreadLocalStoragePointer(NULL, 0);
    if ((pThreadLocal->threadId != TH_CLI) && (globals.cli.needPrefix))
    {
        interruptingPrefix();
        globals.cli.needPrefix = 0;
    }
    printArgs(NULL, afterNlHook, format, args);
}

void interruptingPrintfGrouped(bool *pSavePrinted, const char *format, ...)
{
    if (!*pSavePrinted)
        interruptingPrintBegin();
    *pSavePrinted = 1;
    va_list args;
    va_start(args, format);
    interruptingPrintfArgs(format, args);
}

void interruptingPrintfGroupedFirstOnly(bool *pSavePrinted, const char *format, ...)
{
    if (!*pSavePrinted)
    {
        interruptingPrintBegin();
        va_list args;
        va_start(args, format);
        interruptingPrintfArgs(format, args);
    }
    *pSavePrinted = 1;
}

void i2cPErrorGrouped(bool *pSavePrinted, i2cmErr_t rv, char const *msg)
{
    if (rv == 0)
        return;
    if (!*pSavePrinted)
        interruptingPrintBegin();
    *pSavePrinted = 1;
    i2cPError(rv, msg);
}

void interruptingPrintfGroupedEnd(bool *pSavePrinted)
{
    if (*pSavePrinted)
        interruptingPrintEnd();
}

void interruptingPrintf(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    interruptingPrintfArgs(format, args);
}

void interruptingPrintfOneLine(const char *format, ...)
{
    va_list args;
    va_start(args, format);

    interruptingPrintBegin();
    interruptingPrintfArgs(format, args);
    puts("\n");
    interruptingPrintEnd();
}

void interruptingPrintNL(void)
{
    puts("\n");
    globals.cli.needPrefix = 1;
}

void checkEndInterrupted(void)
{
    if (globals.cli.promptState != PS_interrupted)
        return;
    globals.cli.promptState = PS_none;
    undoGetline();
    puts("----------\n");
    restoreGetline();
}
enum { eM_Top, eM_VReg, eM_SP, NPROMPTS };
static char const *const curPrompt[NPROMPTS] = { "> ", "V> ", "SP> " };

// clang-format off
commandList_t const commandList[] = {
    {CMD_I2CW,              "i2c-w",        "iw",   "Write to the I2C Bus <adr> <reg> [data [data]...]" },
    {CMD_I2CR,              "i2c-r",        "ir",   "Read from the I2C Bus <adr> <reg> <byte count>" },
    {CMD_I2CRR,             "i2c-rr",       "irr",  "Read repeatedly from the I2C Bus <adr> <reg> <byte count>" },
    {CMD_I2CWR,             "i2c-wr",       "iwr",  "Generic I2C Master Write-Read <adr> <read count> [wr data ...]" },
    {CMD_SET_GPIO,          "set-gpio",     "sg",   "Set/Show GPIO <name> [value], omit value for read else [0|1|in|out]" },
    {CMD_I2CSCAN,           "i2c-scan",     "is",   "Perform alien probe" },
    {CMD_SET_CHIP,          "set-c",        "sc",   "Set/Show Chip [chip], omit chip for info" },
    {CMD_SET_REGISTER,      "set-r",        "sr",   "Set/Show Register [regid] [value], omit regid for info, include value to set" },
    {CMD_READ_REGISTER,     "read-r",       "rr",   "Read selected register" },
    {CMD_REGISTER_VALUE,    "set-rv",       "sv",   "Set local register value" },
    {CMD_SET_FIELD,         "set-f",        "sf",   "Set/Show Field [fldid], omit fldid for info" },
    {CMD_MODIFY_FIELD,      "mod-f",        "mf",   "Set Register <value> in hex" },
    {CMD_MODIFY_FIELD_DEC,  "mod-fd",       "mfd",  "Set Register <value> in decimal" },
    {CMD_WRITEOUT_REGISTER, "write-r",      "wr",   "Write out Register [r] (r causes readback)" },
    {CMD_RUNSCRIPT,         "run_script",   "rs",   "Run Script [scriptnum], omit scriptnum for info" },
    {CMD_PWRON,             "pwr-on",       "on",   "Program and enable all regulators" },
    {CMD_PWROFF,            "pwr-off",      "off",  "Disable all regulators"},
    {CMD_PWROFF2,           "pwr-off2",     "off2",  "Disable all regulators but 1P8V for console input"},
    {CMD_RSOC,              "soc-reset",    "rsoc", "toggle SOC reset" },
    {CMD_SPI_PWR,           "spi_pwr",      "spip", "Power 1.8V only for SPI programming" },
    {CMD_PMB_READREGS,      "pmb_readregs", "prr",  "Read PMB regulators" },
    {CMD_STACK,             "stack",        "stk",  "show stack sizes" },
    {CMD_REBOOT,            "reboot",       "rbt",  "reboot PMIC" },
    {CMD_BI,                "bootimage",    "bi",   "reboot to image: 0=Slot0, 1=Slot1, 2=Bootloader" },
    {CMD_RPMB,              "read_pmb",     "rpm",  "Read info <reg_id>, [1] (1 for dump)"},
    {CMD_CHK_REGU,          "chkregs",      "rgc",  "check SMB regulators" },
    {CMD_FS1406_READREGS,   "tdk_readregs", "trr",  "dump FS1406 regulators" },
    {CMD_SET_REGCHK,        "set-rg",       "rgs",  "set check regulators off/on" },
    {CMD_DMP_FAN,           "dumpfan",      "fand", "dump fan registers" },
    {CMD_FRU_READ,          "fru-read",     "fr",   "read fru data <offset>" },
    {CMD_FRU_WRITE,         "fru-write",    "fw",   "write fru data <offset> [data [data]...]. fw FF FF FF to commit" },
    {CMD_FRU_PRINT,         "fru-print",    "fp",   "print fru info" },
    {CMD_T,                 "temp",         "t",    "Temporarily useful command" },
    {CMD_U,                 "temp2",        "u",    "2nd Temporarily useful command" },
    {CMD_FS,                "fsprog",       "fs",   "program FS1406 for 1.8V" },
    {CMD_USERROW,           "userrow",      "ur",   "read/write user row, up offset [ value lng]"},
    {CMD_TIMEDISP,          "timedisp",     "dt",   "turn display timestamp off(0) or on(1)"},
    {CMD_VERSION,           "version",      "ver",  "version: 0=summary, 1=commit info, 2=post commit diffs" },
    {CMD_SMOKE_TEST,        "smoke-test",   "test", "smoke test" },
    {CMD_SWITCH_BOOT_SLOT,  "switch-slot",  "switch", "Command to switch bootslot" },

    {CMD_LISTV,             "listv",        "vl",   "List programmable regulators" },
    {CMD_VREG_MENU,         "vreg_menu",    "vm",   "Enter voltage regulator menu" },
    {CMD_SP_MENU,           "sp_menu",      "spm",  "Enter sp command menu" },
    {CMD_HISTORY,           "history",       "hi",  "Show command history" },
    {CMD_HELP,              "help",          "?",   "Print Help Information" }
};
// clang-format on

static int cliProcess(char *line, void *pvParameters, t_cprf *pCprf, uint8_t *pMenuType);
void CLITask(void *pvParameters)
{
    static char line[LINE_BUFFER_SIZE];
    static char historyBuf[HISTORY_BUFFER_SIZE];
    static uint16_t lineBegIdxBuf[LINEBEGIDX_BUFFER_SIZE];
    getline_t gl;
    globals.pgl = &gl; // initialize global pglGbl

    // find entry for CMD_HISTORY - used to recognize history console cmd
    commandList_t const *pHistCmdItem = commandList;
    while (pHistCmdItem->id != CMD_HISTORY)
        ++pHistCmdItem;

    glInit(&gl, line, countof(line), // initialize persistent info for CliGetLine
        historyBuf, countof(historyBuf), lineBegIdxBuf, countof(lineBegIdxBuf), pHistCmdItem->name,
        pHistCmdItem->altname);

    t_cprf cprf = { 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0, 0, 0, 0, 0, 0, NULL };
    uint8_t menuType = eM_Top;

    // hang until IOTask has initialized things
    IOSEM_TAKE
    IOSEM_GIVE

    checkEndInterrupted();
    globals.cli.promptState = PS_needed;
    while (true)
    {
        globals.pgl->prompt = curPrompt[menuType];
        doPrompt(globals.pgl->prompt);
        if (CliGetLine(&gl, NULL))
        {
            for (char *p = line; *p; ++p)
            {
                if (*p == '#')
                {
                    *p = '\0'; // remove comments
                }
            }
            // hold semaphore during command execution
            PRINTSEM_TAKE
            cliProcess(line, pvParameters, &cprf, &menuType);
            donePrompt();
            PRINTSEM_GIVE
        }
    }
}

static int cliProcess(char *line, void *pvParameters, t_cprf *pCprf, uint8_t *pMenuType)
{
    char *p;
    char *p1;
    char *p2;
    char *p3;
    int cmd;

    if ((line == NULL) || (pCprf == NULL) || (pMenuType == NULL) || (pvParameters == NULL))
    {
        return INVALID_ARGUMENT;
    }

    commonData_t *pCommonData = (commonData_t *)pvParameters;
    powerState_t *pps = &pCommonData->powerState;
    const analogData_t *pad = &pCommonData->analogData;
    (void)pad;

    char *linep = line;

    if (*pMenuType == eM_VReg)
    {
        vRegMenu(linep, pMenuType, pps);
        return STATUS_SUCCESS;
    }
    if (*pMenuType == eM_SP)
    {
        spMenu(linep, pMenuType);
        return STATUS_SUCCESS;
    }

    p = nextToken(&linep);

    // searches the command list for the command
    cmd = searchCommand(p, commandList, sizeof(commandList) / sizeof(commandList_t));
    p1 = nextToken(&linep);
    toupperstr(p1);
    if ((cmd != CMD_I2CW) && (cmd != CMD_FRU_WRITE))
    {
        p2 = nextToken(&linep);
        toupperstr(p2);
        if (cmd != CMD_I2CWR)
        {
            p3 = nextToken(&linep);
            toupperstr(p3);
        }
    }

    switch (cmd)
    {
        case CMD_HELP:
            printf("All parameters are unsigned hexadecimal preceded by 0x or decimal default\n");
            printHelp(commandList, sizeof(commandList) / sizeof(commandList_t));
            break;
        case CMD_SET_GPIO:
            cmdSetGpio(p1, p2);
            break;
        case CMD_SET_CHIP:
            cmdSetChip(p1, pCprf);
            break;
        case CMD_SET_REGISTER:
            cmdSetReg(p1, p2, pCprf);
            break;
        case CMD_READ_REGISTER:
            cmdReadRegister(pCprf);
            break;
        case CMD_REGISTER_VALUE:
            cmdSetRegisterValue(p1, pCprf);
            break;
        case CMD_SET_FIELD:
            cmdSetField(p1, pCprf);
            break;
        case CMD_MODIFY_FIELD:
            cmdModifyField(p1, pCprf);
            break;
        case CMD_MODIFY_FIELD_DEC:
            cmdModifyFieldDec(p1, pCprf);
            break;
        case CMD_WRITEOUT_REGISTER:
            cmdWriteoutRegister(p1, pCprf);
            break;
        case CMD_RUNSCRIPT:
            runScript(p1, pCprf, pps);
            break;
        case CMD_I2CW:
            doI2cWrite(p1, &linep);
            break;
        case CMD_I2CR:
            doI2cRead(p1, p2, p3, 0);
            break;
        case CMD_I2CRR:
            doI2cRead(p1, p2, p3, 1);
            break;
        case CMD_I2CWR:
            doI2cWriteRead(p1, p2, &linep);
            break;
        case CMD_I2CSCAN:
            cmdI2cScan();
            break;
        case CMD_PWRON:
            cmdPowerOn(pps);
            break;
        case CMD_PWROFF:
            cmdPowerOff(pps);
            break;
        case CMD_PWROFF2:
            cmdSpiPwr(pps);
            break;
        case CMD_RSOC:
            cmdResetSoc();
            break;
        case CMD_SPI_PWR:
            cmdSpiPwr(pps);
            break;
        case CMD_PMB_READREGS:
        {
            bool ok = 0;
            uint32_t v;
            if (!p1)
            {
                printf("missing address\n");
                break;
            }
            v = atohex(p1, &ok);
            if (!ok || v > 0xFF)
            {
                printf("bad address: %s\n", p);
                break;
            }
            pmbReadback((uint8_t)v);
        }
        break;
        case CMD_CHK_REGU:
            checkRegu();
            break;
        case CMD_RPMB:
            cmdRPmb(p1, p2);
            break;
        case CMD_FS1406_READREGS:
            dumpFS1406Regu();
            break;
        case CMD_SET_REGCHK:
            setRegChk(p1, pps);
            break;
        case CMD_DMP_FAN:
            dumpFan();
            break;
        case CMD_FRU_READ:
            cmdFruRead();
            break;
        case CMD_FRU_WRITE:
            cmdFruWrite(p1, &linep);
            break;
        case CMD_FRU_PRINT:
            cmdFruPrint();
            break;
        case CMD_T:
            cmdT(p1);
            break;
        case CMD_U:
            cmdU(p1);
            break;
        case CMD_FS:
            cmdFS(p1, p2);
            break;
        case CMD_STACK:
            cmdStack();
            break;
        case CMD_REBOOT:
            cmdPowerOff(pps);
            OSDELAY_MS(100)
            reboot();
            break;
        case CMD_TIMEDISP:
            cmdTimedisp(p1);
            break;
        case CMD_USERROW:
            cmdUserRow(p1, p2, p3);
            break;
        case CMD_BI:
            cmdBootToImage(p1);
            break;
        case CMD_LISTV:
            cmdListv();
            break;
        case CMD_VREG_MENU:
            vRegMenu(NULL, pMenuType, pps);
            break;
        case CMD_SP_MENU:
            spMenu(NULL, pMenuType);
            break;
        case CMD_VERSION:
            cmdVersion(p1);
            break;
        case CMD_SMOKE_TEST:
            cmdSmokeTest(p1, pMenuType, pCprf, pps);
            break;
        case CMD_SWITCH_BOOT_SLOT:
            cmdSwitchSlot(p1);
            break;
        default:
            if (p && cmd < 0)
                interruptingPrintfOneLine("Unknown Command: %s\n", p);
            break;
    } //switch

    return STATUS_SUCCESS;
}

// clang-format off
const commandList_t commandListV[] = {
    { CMD_LISTV,            "list",         "l",    "List programmable regulators" },
    { CMD_READV,            "read_v",       "r",    "Read voltage [<reg_id>] skip reg_id to read all" },
    { CMD_SETV,             "set_v",        "s",    "Set voltage <reg_id>, <mV>"},
    { CMD_CLPMB,            "clr_pmb",      "cpm",  "Clear PMB - min, max, ave"},
    { CMD_RPMB,             "read_pmb",     "rpm",  "Read info <reg_id>, [1] (1 for dump)"},
    { CMD_EXIT,             "x",            "q",    "Exit to Main Menu"  },
    { CMD_HISTORY,          "history",      "hi",  "Show command history" },
    { CMD_HELP,             "help",         "?",    "Print Help Information" }
};
// clang-format on

void cmdListv(void);

void vRegMenu(char *line, uint8_t *pMenuType, const powerState_t *pps)
{
    char *linep;
    char *p;
    char *p1;
    char *p2;
    int cmd;
    if (!line)
    {
        *pMenuType = eM_VReg;
        interruptingPrintfOneLine("\n? for help, Q to return to main menu\n");
        return;
    }
    linep = line;
    p = nextToken(&linep);
    cmd = searchCommand(p, commandListV, sizeof(commandListV) / sizeof(commandList_t));
    p1 = nextToken(&linep);
    toupperstr(p1);

    switch (cmd)
    {
        case CMD_LISTV:
            cmdListv();
            break;
        case CMD_READV:
            cmdReadv(p1);
            break;
        case CMD_SETV:
            p2 = nextToken(&linep);
            toupperstr(p2);
            cmdSetv(p1, p2, pps);
            break;
        case CMD_RPMB:
            p2 = nextToken(&linep);
            cmdRPmb(p1, p2);
            break;
        case CMD_CLPMB:
            initPmbStats();
            interruptingPrintfOneLine("PMB stats cleared");
            break;
        case CMD_HELP:
            printHelp(commandListV, sizeof(commandListV) / sizeof(commandList_t));
            break;

        case CMD_EXIT:
            *pMenuType = eM_Top;
            break;

        default:
            if (p && cmd < 0)
                interruptingPrintfOneLine("Unknown Command: %s\n", p);
            break;
    }
}

typedef enum { CMD_BYCODE, CMD_HELP_SP, CMD_EXIT_SP } spcommand_t;

// clang-format off
const commandList_t commandListSpMenu[] = { // SP CLI commands
    {CMD_BYCODE,            "set",          "s",    "Read/Set by code [code(hex)] [val(hex)]"   },
    {CMD_EXIT_SP,           "x",            "q",    "Exit to Main Menu"   },
    {CMD_HISTORY,           "history",      "hi",   "Show command history"  },
    {CMD_HELP_SP,           "help",         "?",    "Print Help Information" }
};
// clang-format on

static int cmdByCode(char const *p1, char const *p2);

void spMenu(char *line, uint8_t *pMenuType)
{
    char *linep;
    char *p;
    char *p1;
    const char *p2;
    int cmd;
    if (!line)
    {
        *pMenuType = eM_SP;
        interruptingPrintfOneLine("\n? for help, Q to return to main menu\n");
        return;
    }
    linep = line;
    p = nextToken(&linep);
    cmd = searchCommand(p, commandListSpMenu, sizeof(commandListSpMenu) / sizeof(commandList_t));
    p1 = nextToken(&linep);
    toupperstr(p1);
    p2 = nextToken(&linep);

    switch (cmd)
    {
        case CMD_HELP_SP:
            printHelp(commandListSpMenu, sizeof(commandListSpMenu) / sizeof(commandList_t));
            break;

        case CMD_EXIT_SP:
            *pMenuType = eM_Top;
            break;

        case CMD_BYCODE:
            cmdByCode(p1, p2);
            break;
        default:
            if (p && cmd < 0)
                interruptingPrintfOneLine("Unknown Command: %s\n", p);
            break;
    }
}

void printRealValueFromCmdRegisterValue(uint32_t cmd, uint32_t regVal);
void printRealValueFromCmdRegisterValue(uint32_t cmd, uint32_t regVal)
{
    int t;

    switch (cmd)
    {
        case voltageInSpCmd:
            t = regVal * 50; // mV
            interruptingPrintfOneLine("%d.%02d volts", t / 1000, (t % 1000) / 10);
            break;
        case powerInSpCmd:
        case averagePowerSpCmd:
            t = regVal * 10; // mW
            interruptingPrintfOneLine("%d.%02d watts", t / 1000, (t % 1000) / 10);
            break;
        case powerAlarmSpCmd:
            t = regVal * 250; // mW
            interruptingPrintfOneLine("%d.%02d watts", t / 1000, (t % 1000));
            break;
        case systemTempSpCmd:
        case tempAlarmSpCmd:
            interruptingPrintfOneLine("%dC degrees", regVal);
            break;
        case DDQLPVoltageSpCmd:
        case DDQVoltageSpCmd:
            interruptingPrintfOneLine("%d mV", 250 + 10 * regVal);
            break;
        case L2CacheVoltageSpCmd:
        case DDRVoltageSpCmd:
        case maxionVoltageSpCmd:
        case NOCVoltageSpCmd:
        case allMinionVoltageSpCmd:
        case minionG1VoltageSpCmd:
        case minionG2VoltageSpCmd:
        case minionG3VoltageSpCmd:
        case minionG4VoltageSpCmd:
        case minionG5VoltageSpCmd:
        case minionG6VoltageSpCmd:
        case minionG7VoltageSpCmd:
        case minionG8VoltageSpCmd:
        case minionG9VoltageSpCmd:
        case minionG10VoltageSpCmd:
        case minionG11VoltageSpCmd:
        case minionG12VoltageSpCmd:
        case minionG13VoltageSpCmd:
        case minionG14VoltageSpCmd:
        case minionG15VoltageSpCmd:
        case minionG16VoltageSpCmd:
        case minionG17VoltageSpCmd:
            interruptingPrintfOneLine("%d mV", 250 + 5 * regVal);
            break;
        case PCIeLogicVoltageSpCmd:
            interruptingPrintfOneLine("%d mV", 600 + (625 * regVal) / 100);
            break;
        case PCIeVoltageSpCmd:
            interruptingPrintfOneLine("%d mV", 600 + (125 * regVal) / 10);
            break;
        default:
            break;
    }
}

void printCmdRegisterValue(uint32_t cmd, uint32_t regVal);
void printCmdRegisterValue(uint32_t cmd, uint32_t regVal)
{
    if (cmd == firmwareVersionSpCmd)
    {
        interruptingPrintf("0x%02X %-21s : %x\n", cmd, getCmdName(cmd), regVal);
    }
    else
    {
        interruptingPrintf("0x%02X %-21s : 0x%0*X%s\n", cmd, getCmdName(cmd), getNumberOfBytes(cmd) * 2, regVal,
            (isCleanAfterReadRequired(cmd)) ? "   (console read does not autoclear)" : "");
    }
}

static int cmdByCode(char const *p1, char const *p2)
{
    bool ok = true;
    uint32_t data;
    spCommandIndex_t cmd = reservedSpCmd;
    int status = STATUS_SUCCESS;

    interruptingPrintBegin();
    if (!p1)
    {
        // Command is called without arguments, print all registers
        for (uint32_t i = reservedSpCmd + 1; i < numberOfSpCommands; i++)
        {
            status = getRegisterValue(i, &data);
            if (status == STATUS_SUCCESS)
            {
                printCmdRegisterValue(i, data);
            }
            else
            {
                interruptingPrintf("cmdByCode: Error get register value %d\n", status);
                return status;
            }
        }
        return status;
    }

    // Get SP command index from the first argument
    cmd = atohex(p1, &ok);
    if (!ok || (cmd == 0) || (cmd >= numberOfSpCommands))
    {
        interruptingPrintf("bad code: %s\n", p1);
        return CLI_TASK_ERROR_INVALID_CMD_CODE;
    }

    if (p2)
    {
        // Get data for SP command write request from the second argument
        uint32_t val = 0;
        val = atohex(p2, &ok);
        if (!ok)
        {
            interruptingPrintf("bad val: %s\n", p2);
            return CLI_TASK_ERROR_INVALID_VALUE;
        }

        if (!isAccessTypeReadOnly(cmd))
        {
            // Post write request to ETSOC command handler
            status = etsocCommandSendEvent(false, (uint8_t)cmd, val);
        }
        else
        {
            interruptingPrintf("Console can set read only command 0x%02X %-21s\n", cmd, getCmdName(cmd));
            status = setRegisterValue(getRegisterIndexFromSpCmdIndex(cmd), val);
        }
        if (status != STATUS_SUCCESS)
        {
            interruptingPrintf("cmdByCode: Error sending event: %d\n", status);
        }

        return status;
    }

    // Second argument is not provided, SP command is read request
    if (!isAccessTypeWriteOnly(cmd))
    {
        status = getRegisterValue(getRegisterIndexFromSpCmdIndex(cmd), &data);
        if (status == STATUS_SUCCESS)
        {
            printCmdRegisterValue(cmd, data);
            printRealValueFromCmdRegisterValue(cmd, data);
            handlePostReadUpdate(cmd);
        }
        if (status != STATUS_SUCCESS)
        {
            interruptingPrintf("cmdByCode: Error occured. status: %d\n", status);
        }
    }
    else
    {
        interruptingPrintf("0x%02X %-21s write only\n", cmd, getCmdName(cmd));
    }

    return status;
}

void dumpFan(void)
{
    for (t_structRegInfo const *p = structRegInfo_MAX6660; p < &structRegInfo_MAX6660[MAXREG_MAX6660]; ++p)
    {
        uint8_t v = 0;
        uint8_t rreg = p->regnum;
        if (rreg == 0xff)
            continue;
        i2cmErr_t r = i2cm_read8(structChipInfo[eID_MAX6660].addr7, rreg, &v);
        interruptingPrintfOneLine("%d %02X (%02X): %-10s %02X", r, rreg, p->wrtieRegnum, p->regName, v);
    }
}

void cmdTimedisp(char const *p1)
{
    bool ok = 0;
    uint32_t v = atohex(p1, &ok);
    if (!ok)
        return;
    pfiShowTime = v != 0;
    restartPrintfFromInterrupt();
}

#if 0
#include "FS1406.h"
void cmdT( char const * p1 )
{
    for( t_structRegInfo const * p = structRegInfo_FS1406; p<&structRegInfo_FS1406[MAXREG_FS1406]; ++p ) {
        uint8_t v=0;
        uint8_t rreg = p->regnum;
        if( rreg == 0xff ) continue;
        i2cmErr_t r= i2cm_read8( 0x08, rreg, &v );
        interruptingPrintfOneLine( "%d %02X (%02X): %-10s %02X", r, rreg, p->wrtieRegnum, p->regName, v );
    }
}
#else
void printTiReadIdxs(void);
void printTiReadIdxs(void)
{
    interruptingPrintf("next: s %d, d %d, r %d\n", pmbStatsReadStat, pmbStatsReadDevc, pmbStatsReadReg);
}

void cmdT(char const *p1)
{
    bool ok = 0;
    uint32_t v = atohex(p1, &ok);
    if (!ok)
        return;
    i2cmErr_t rv = 0;

    interruptingPrintBegin();
    if (v == 0x60)
    {
        printTiReadIdxs();
        uint8_t tx[1];
        uint8_t rx[4];
        tx[0] = 0x15;
        while (!I2CS_Get_Slave_Busy_Status())
            continue;
        rv = i2cm_read(0x42, tx, sizeof(tx), rx, sizeof(rx));
        if (rv != I2CM_OK)
        {
            interruptingPrintf("I2c write Error %s", i2cm_get_error_message(rv));
            interruptingPrintEnd();
            return;
        }
        uint16_t t = (uint16_t)(((uint16_t)rx[1] << 8) | rx[0]);
        interruptingPrintf("%02X %02X, %04X %3d\n", rx[3], rx[2], t, t);
        interruptingPrintEnd();
        return;
    }
    if (v != 0x70)
    {
        rv = i2cm_write8(0x42, 0x15, (uint8_t)v);
        if (rv != I2CM_OK)
        {
            interruptingPrintf("I2c write Error %s\n", i2cm_get_error_message(rv));
        }
        interruptingPrintEnd();
        return;
    }
    static uint16_t x[93];
    int i;
    for (i = 0; i < 93; ++i)
    {
        uint8_t tx[1];
        uint8_t rx[4];
        tx[0] = 0x15;
        while (!I2CS_Get_Slave_Busy_Status())
            continue;
        rv = i2cm_read(0x42, tx, sizeof(tx), rx, sizeof(rx));
        if (rv != I2CM_OK)
        {
            interruptingPrintf("I2c read Error %s at %d\n", i2cm_get_error_message(rv), i);
            interruptingPrintEnd();
            return;
        }
        uint16_t t = (uint16_t)((uint16_t)rx[1] << 8) | (uint16_t)rx[0];
        x[i] = t;
        interruptingPrintf("%3d ", t);
    }
    interruptingPrintNL();
    i = 0;
    for (int chan = 0; chan < 5; ++chan)
    {
        if (chan < 3)
        {
            for (int reg = 0; reg < 7; ++reg)
            {
                interruptingPrintf("ch %d, reg %d:", chan, reg);
                for (int stat = 0; stat < 4; ++stat)
                {
                    interruptingPrintf(" %04X", x[i++]);
                }
                interruptingPrintNL();
            }
        }
        else
        {
            interruptingPrintf("chan %d:", chan);
            for (int stat = 0; stat < 4; ++stat)
            {
                interruptingPrintf(" %04X", x[i++]);
            }
            interruptingPrintNL();
        }
    }
    interruptingPrintEnd();
    FlushPrintfFromInterrupt();
}
#endif

void cmdBootToImage(char const *p1)
{
    bool ok = 0;
    uint32_t v = atohex(p1, &ok);
    if (!ok || v > 2)
    {
        interruptingPrintfOneLine("Bad target: 0 for Slot 0, 1 for Slot 1, 2 for Bootloader");
        return;
    }
    if (v == BOOT_IMAGE_SLOT)
    {
        interruptingPrintfOneLine("Already in Slot %d", BOOT_IMAGE_SLOT);
        return;
    }
    static uint8_t xlate[] = { 1, 2, 0 };
    bootToImage(xlate[v]);
}

void cmdUserRow(char const *p1, char const *p2, char const *p3)
{
    bool ok;
    bool write;
    uint32_t offset;
    uint32_t v;
    uint32_t lng;

    offset = atohex(p1, &ok);
    if (!ok)
    {
        interruptingPrintf("missing offset: up offset [ lng [value] ]\n");
        return;
    }
    lng = atohex(p2, &ok);
    if (!ok)
        lng = 1;
    if (lng == 0 || lng > 4)
    {
        interruptingPrintf("lng must be 1 to 4\n");
        return;
    }

    v = atohex(p3, &write);
    if (write)
    {
        if ((offset + lng) >= NVM_USERROW_SIZE)
        {
            interruptingPrintf("writing offset above 0x08 forbidden.\n");
            return;
        }

        int status = Flash_Access_Write_User_Row(offset, v, lng);
        if (status != STATUS_SUCCESS)
        {
            interruptingPrintf("updateUserRow failed with status \n", status);
            return;
        }
    }
    volatile uint8_t *userRow = Flash_Access_Get_User_Row();
    const volatile uint8_t *p = ((const volatile uint8_t *)(volatile void *)userRow) + offset;
    v = 0;
    for (uint32_t i = 0; i < lng; ++i)
    {
        v <<= 8;
        v |= *p++;
    }
    interruptingPrintf("%d bytes userRow[%02x] is %X (%d)\n", lng, offset, v, v);
}

void cmdSmokeTest(const char *itr, uint8_t *pMenuType, const t_cprf *pCprf, powerState_t *pps)
{
    (void)pCprf;

    PRINTSEM_GIVE

    const char *regulatorId[9] = { "QLP", "SRM", "DDR", "DDQ", "PCL", "PCI", "MXN", "NOC", "MNN" };
    const uint16_t defaultVoltage[9] = { 640, 700, 800, 1100, 775, 1500, 600, 450, 400 };
    const uint16_t defaultVoltageCode[26] = { 0x27, 0x5A, 0x6E, 0x55, 0x1C, 0x48, 0x46, 0x28, 0x1E, 0x1E, 0x1E, 0x1E,
        0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E };
    const uint16_t voltageResolution[9][2] = { { 10, 1 }, { 5, 1 }, { 5, 1 }, { 10, 1 }, { 625, 100 }, { 125, 10 },
        { 5, 1 }, { 5, 1 }, { 5, 1 } };

    interruptingPrintfOneLine("\n################# TEST START #################\n");

    /* extract number of iterations*/
    uint16_t testIterations = 0;
    for (int i = 0; itr[i] != '\0'; i++)
    {
        if (itr[i] >= '0' && itr[i] <= '9')
        {
            testIterations = (uint16_t)(testIterations * 10 + (itr[i] - '0'));
        }
        else
        {
            break;
        }
    }

    if (testIterations == 0)
    {
        interruptingPrintf("Missing number of iterations.\n");
        return;
    }

    //enter main menu
    *pMenuType = eM_Top;

    //print version
    cmdVersion("1");
    OSDELAY_MS(10)
    cmdVersion("4");
    OSDELAY_MS(10)

    //check if all supported board types are added to the fw metadata
    interruptingPrintfOneLine(
        "\n Fw supported board types 0x%x", Config_Get_Supported_Board_Types_From_Metadata(BOOT_IMAGE_SLOT));
    SMOKE_ASSERT(
        (Hw_Encoding_Get_Supported_Board_Types() == Config_Get_Supported_Board_Types_From_Metadata(BOOT_IMAGE_SLOT)),
        "Supported board types missmatch");

    //run tests in a loop
    for (uint32_t i = 0; i < testIterations; i++)
    {
        interruptingPrintfOneLine("\n***************** ITERATION %d *****************\n", i + 1);

        //enter main menu
        *pMenuType = eM_Top;

        //1. reset ETSOC
        interruptingPrintfOneLine("\n1. Reset ETSOC...");
        cmdResetSoc();
        OSDELAY_MS(5000)
        //TODO SW-SW-17836 - CHECK_ETSOC_RESET_STATE()

        //2. power off - on
        interruptingPrintfOneLine("\n2. Toggle PMIC output power off/on...");
        cmdPowerOff(pps);
        OSDELAY_MS(5000)
        cmdPowerOn(pps);
        OSDELAY_MS(10000)
        SMOKE_ASSERT(getPowerState() == true, "HW Power UP")
        //TODO SW-SW-17836 -CHECK_ETSOC_RESET_STATE()

        //3. clear pmb stats
        interruptingPrintfOneLine("\n3. Clear PMB statistics...");
        initPmbStats();

        //4. set voltages
        uint16_t readVoltage;
        uint16_t readDefaultVoltage;
        uint16_t voltageToSet;

        // Allocate a buffer for the value string, max 4 characters and one terminating character
        char value[5];

        interruptingPrintfOneLine("\n4. Set regulator voltage...");
        for (uint32_t regIdx = 0; regIdx < 9; regIdx++)
        {
            voltageToSet = (defaultVoltage[regIdx] + 20);
            sprintf(value, "%d", voltageToSet);
            SMOKE_ASSERT(cmdSetv(regulatorId[regIdx], (const char *)value, pps) == true, "CMD Set Voltage")
            readVoltage = cmdReadv(regulatorId[regIdx]);

            sprintf(value, "%d", defaultVoltage[regIdx]);
            SMOKE_ASSERT(cmdSetv(regulatorId[regIdx], (const char *)value, pps) == true, "CMD Set Default Voltage")
            readDefaultVoltage = cmdReadv(regulatorId[regIdx]);

            /* Verify set voltage */
            VERIFY_VOLTAGE(regulatorId[regIdx], readVoltage, voltageToSet, voltageResolution[regIdx][0],
                voltageResolution[regIdx][1])
            VERIFY_VOLTAGE(regulatorId[regIdx], readDefaultVoltage, defaultVoltage[regIdx],
                voltageResolution[regIdx][0], voltageResolution[regIdx][1])
        }

        //5. check SMB regulators
        interruptingPrintfOneLine("\n5. Check SMB & FS1406 regulators...");
        SMOKE_ASSERT(checkRegu() == true, " SMB regulators")

        //5. check FS1406 regulators
        SMOKE_ASSERT(dumpFS1406Regu() == STATUS_SUCCESS, "FS1406 Regulator")

        //6. read/clear pmb statistics 10 times in loop
        interruptingPrintfOneLine("\n6. Read PMB statistics...");
        for (uint32_t j = 0; j < 10; j++)
        {
            SMOKE_ASSERT(cmdRPmb("NOC", "1") == true, "Read NOC stats")
            SMOKE_ASSERT(cmdRPmb("MNN", "1") == true, "Read MNN stats")
            SMOKE_ASSERT(cmdRPmb("SRM0", "1") == true, "Read SRM0 stats")
            SMOKE_ASSERT(cmdRPmb("SRM1", "1") == true, "Read SRM1 stats")
        }

        interruptingPrintfOneLine("\n6a. Verify SRAM Vout...");
        VERIFY_VOLTAGE(
            regulatorId[1], defaultVoltage[1], getPmbStatsSrmVout(), voltageResolution[1][0], voltageResolution[1][1])
        interruptingPrintfOneLine("\n6b. Verify NOC Vout...");
        VERIFY_VOLTAGE(
            regulatorId[7], defaultVoltage[7], getPmbStatsNocVout(), voltageResolution[7][0], voltageResolution[7][1])
        interruptingPrintfOneLine("\n6c. Verify MNN Vout...");
        VERIFY_VOLTAGE(
            regulatorId[8], defaultVoltage[8], getPmbStatsMnnVout(), voltageResolution[8][0], voltageResolution[8][1])

        //SMOKE_ASSERT(getPmbStatsNumOfFails() = 0UL, "Read PMB statistics errors."); //TODO: Fix error counter SW-17774.
        initPmbStats();

        //7. SP commands
        interruptingPrintfOneLine("\n7. Read regulator voltage registers with SP command...");

        /* create a buffer for max 2 digit hex value and one terminating char*/
        char regSpCmdCode[3];
        uint32_t voltageValue;
        for (unsigned int idx = DDQLPVoltageSpCmd; idx < minionG17VoltageSpCmd; idx++)
        {
            sprintf(regSpCmdCode, "%02X", idx);
            SMOKE_ASSERT(cmdByCode((const char *)regSpCmdCode, NULL) == STATUS_SUCCESS, " CMD by Code")
            getRegisterValue(getRegisterIndexFromSpCmdIndex(idx), &voltageValue);
            SMOKE_ASSERT(voltageValue == defaultVoltageCode[idx - DDQLPVoltageSpCmd], "Read voltage SP cmd")
            OSDELAY_MS(100)
        }

        //8. write validation
        interruptingPrintfOneLine("\n8. Write Sweep Validation");

        enum REG_IDX { SRM_IDX = 1, NOC_IDX = 7, MNN_IDX = 8 };
        const uint16_t regs[3] = { SRM_IDX, NOC_IDX, MNN_IDX };
        for (uint16_t idx = 0; idx < 3; idx++)
        {
            interruptingPrintfOneLine("\nSweeping %s", regulatorId[regs[idx]]);
            uint16_t maxVoltage = 0;
            switch (regs[idx])
            {
                case SRM_IDX:
                    maxVoltage = SRM_MAX_VAL_mV;
                    break;
                case NOC_IDX:
                    maxVoltage = NOC_MAX_VAL_mV;
                    break;
                case MNN_IDX:
                    maxVoltage = MNN_MAX_VAL_mV;
                    break;
                default:
                    break;
            }
            uint16_t maxStep = maxVoltage - defaultVoltage[regs[idx]];
            for (uint16_t step = 5; step <= maxStep; step += 5)
            {
                voltageToSet = defaultVoltage[regs[idx]] + step;
                cmdIntSetv(regulatorId[regs[idx]], voltageToSet);
                OSDELAY_MS(150)
                switch (regs[idx])
                {
                    case SRM_IDX:
                        VERIFY_VOLTAGE(regulatorId[regs[idx]], voltageToSet, getPmbStatsSrmVout(),
                            voltageResolution[regs[idx]][0], voltageResolution[regs[idx]][1])
                        break;
                    case NOC_IDX:
                        VERIFY_VOLTAGE(regulatorId[regs[idx]], voltageToSet, getPmbStatsNocVout(),
                            voltageResolution[regs[idx]][0], voltageResolution[regs[idx]][1])
                        break;
                    case MNN_IDX:
                        VERIFY_VOLTAGE(regulatorId[regs[idx]], voltageToSet, getPmbStatsMnnVout(),
                            voltageResolution[regs[idx]][0], voltageResolution[regs[idx]][1])
                        break;
                    default:
                        break;
                }
            }
        }

        uint16_t voltageToSetSRM;
        uint16_t voltageToSetNOC;
        uint16_t voltageToSetMNN;
        interruptingPrintfOneLine("\nSweep Multiple");
        for (uint16_t step = 5; step <= 20; step += 5)
        {
            voltageToSetSRM = defaultVoltage[SRM_IDX] + step;
            voltageToSetNOC = defaultVoltage[NOC_IDX] + step;
            voltageToSetMNN = defaultVoltage[MNN_IDX] + step;
            cmdIntSetv(regulatorId[SRM_IDX], voltageToSetSRM);
            cmdIntSetv(regulatorId[NOC_IDX], voltageToSetNOC);
            cmdIntSetv(regulatorId[MNN_IDX], voltageToSetMNN);
            OSDELAY_MS(450)
            VERIFY_VOLTAGE(regulatorId[SRM_IDX], voltageToSetSRM, getPmbStatsSrmVout(), voltageResolution[SRM_IDX][0],
                voltageResolution[SRM_IDX][1])
            VERIFY_VOLTAGE(regulatorId[NOC_IDX], voltageToSetNOC, getPmbStatsNocVout(), voltageResolution[NOC_IDX][0],
                voltageResolution[NOC_IDX][1])
            VERIFY_VOLTAGE(regulatorId[MNN_IDX], voltageToSetMNN, getPmbStatsMnnVout(), voltageResolution[MNN_IDX][0],
                voltageResolution[MNN_IDX][1])
        }
    }

    interruptingPrintfOneLine("\n################# TEST END #################\n");
}

void cmdSwitchSlot(const char *slot_num)
{
    (void)slot_num; /* Unused param */
    int status = STATUS_SUCCESS;
    uint32_t boot_slot = Config_Get_Boot_Slot_From_Config_Header();

    /* Toggle boot slot */
    boot_slot ^= 1;

    status = Config_Set_Boot_Slot(boot_slot);
    if (status != STATUS_SUCCESS)
    {
        interruptingPrintfOneLine("cmdSwitchSlot failed to update config header: %d", status);
    }

    interruptingPrintfOneLine("\nChanged to boot slot: %d", boot_slot);
}
