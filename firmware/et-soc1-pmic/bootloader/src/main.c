/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file main.c
    \brief Bootloader main.
*/
/***********************************************************************/

#ifndef BOOTLOADER
#error define BOOTLOADER in makefile
#endif

#include <stdbool.h>
#include <stdint.h>

#include "version.h"
#include "gpio.h"
#include "system_clock.h"
#include "config.h"
#include "error_codes.h"
#include "image_checksum.h"
#include "hw_encoding.h"
#include "boardinfo.h"
#include "flash_access.h"

/***********************************************************************
* Macros:             Defininition of local macros
***********************************************************************/
#define countof(x) (sizeof(x) / sizeof(*x))
#define endof(x)   (&x[sizeof(x) / sizeof(*x)])

#define BLMAGICNUM 0xB007B007 //used to determine which fw to boot - bootloader, slot0, or slot1

#define IVT_OFFSET 0x0

/***********************************************************************
* Data types:         Defininition of local data types
***********************************************************************/
typedef enum {
    IMT_slot0 = BOOT_SLOT_0,
    IMT_slot1 = BOOT_SLOT_1,
    IMT_bootloader = 3,
    IMT_none = 4,
} t_IMT;

typedef enum {
    Ecksum_Ok = 0,
    Ecksum_Corrupt = 1,
} t_Ecksum;

typedef void (*command_t)(char const *p1, char const *p2);

typedef struct {
    command_t cmd;
    const char *name;
    const char *help;
} commandList_t;

/***********************************************************************
* Functions:          Declaration of callback functions
***********************************************************************/
static void cmdHelp(char const *p1, char const *p2);
static void cmdCheckImage(char const *p1, char const *p2);
static void cmdBootImage(char const *p1, char const *p2);
static void cmdReboot(char const *p1, char const *p2);

/***********************************************************************
* Variables:          Defininition of local variables and constats
***********************************************************************/
/* Read-only metadata containing properties of the image.
   Image metadata is at offset 0x100 */
__attribute__((section(".blmetadata"))) bootloaderMetadata_t const volatile blMetadata = {
    .bl_fw_version = (BL_FW_VERSION_H << 16) | (BL_FW_VERSION_M << 8) | (BL_FW_VERSION_L),
    .hash = { COMMITID },
};

uint32_t const imageAddrs[2] = { SLOT_0_IMGOFFSET, SLOT_1_IMGOFFSET };
uint32_t const maxImageSize = IMAGESIZE;

static const char *cksumMsg[] = { "OK", "CORRUPT" };

uint32_t startupFlag; // set in startup_samd20.c

// clang-format off
commandList_t const commandList[] = {
    { cmdHelp,       "h  - prints help",   "h - prints help" },
    { cmdHelp,       "?  - prints help",   "? - prints help" },
    { cmdCheckImage, "ci - check images",  "ci" },
    { cmdBootImage,  "bi - boot to image", "bi 0 for slot 0, 1 for slot 1" },
    { cmdReboot,     "rbt - reboot board", "rbt" },
};
// clang-format on

/***********************************************************************
* Functions:          Declaration of local functions
***********************************************************************/
void __libc_init_array(void);

static void consoleLoop(void);
static void bootTo(t_IMT imageSlotNum);
static bool verifyFwImage(t_IMT imageSlotNum);
static void putchar1(char const c);
static int puts1(char const *s);
static void putNl(void);

static t_Ecksum cksumOk(t_IMT imageSlotNum);
static void updateFailedBootCounter(uint32_t cnt);
static void updateSlotNumber(uint32_t slot);
static uint32_t getBootloaderFwVersion(void);
static const char *getBootloaderHash(void);

static char getChar(void);
static char *getLine(bool echo);
static bool strSameUpToSpace(char const *p1, char const *p2);
static void printHexNumber(uint32_t v, int d);
static void printDecNumber(uint32_t v);
static void reboot(void);

/***********************************************************************
* GLOBAL Functions:   Defininition of global functions
***********************************************************************/
int main(void)
{
    t_IMT imageSlotNum = IMT_none;
    uint32_t failedBootAttemptCnt;

    puts1("Bootloader up!\r\n");

    puts1("Bootloader firmware version: ");
    printDecNumber(GET_VERSION_MAJOR(getBootloaderFwVersion()));
    puts1(".");
    printDecNumber(GET_VERSION_MINOR(getBootloaderFwVersion()));
    puts1(".");
    printDecNumber(GET_VERSION_PATCH(getBootloaderFwVersion()));
    puts1("\r\n");
    puts1("Bootloader firmware hash: ");
    puts1(getBootloaderHash());
    puts1("\r\n\n");

    Hw_Encoding_Read_And_Store_Hw_Version_Info();

    puts1("Board ID: ");
    putchar1(gpio_get_tri_char(BRDID_2));
    putchar1(gpio_get_tri_char(BRDID_1));
    putchar1(gpio_get_tri_char(BRDID_0));
    puts1("\r\n");
    puts1("Bootloader: Board type name/revision: ");
    puts1(Hw_Encoding_Get_Hw_Board_Name());
    puts1(" ");
    printDecNumber(Hw_Encoding_Get_Hw_Design_Revision());
    puts1(".");
    printDecNumber(Hw_Encoding_Get_Hw_Modification_Revision());
    puts1("\r\n");

    /* Get fw image slot */
    if (startupFlag == BLMAGICNUM)
    {
        imageSlotNum = IMT_bootloader;
        puts1("BL forced: don't boot, enter console\r\n");
        consoleLoop();
    }
    else if (startupFlag == BLMAGICNUM + 1)
    {
        imageSlotNum = IMT_slot0;
        puts1("BL forced: boot to SLOT_0\r\n");
    }
    else if (startupFlag == BLMAGICNUM + 2)
    {
        imageSlotNum = IMT_slot1;
        puts1("BL forced: boot to SLOT_1\r\n");
    }
    else
    {
        /* Fetch boot slot from configuration header */
        imageSlotNum = (Config_Get_Boot_Slot_From_Config_Header() == BOOT_SLOT_0) ?
                           IMT_slot0 :
                           (Config_Get_Boot_Slot_From_Config_Header() == BOOT_SLOT_1) ? IMT_slot1 : IMT_none;
        puts1("Bootloader: Selected image number (allowed image numbers: 0 or 1): ");
        printDecNumber(imageSlotNum);
    }

    /* Fetch failed boot counter from configuration header */
    failedBootAttemptCnt = Config_Get_Failed_Boot_Cnt_From_Config_Header();

    /* Handle invalid or corrupted slot value */
    if (imageSlotNum == IMT_none)
    {
        puts1("\r\nERROR: Selected image number is not valid, increase failed boot attempt counter.\r\n ");
        failedBootAttemptCnt++;
        updateFailedBootCounter(failedBootAttemptCnt);

        /* Verify images in slot 0 and 1 and boot from any valid slot */
        if (verifyFwImage(IMT_slot0))
        {
            puts1("\r\nWarning: Forced boot from SLOT_0...\r\n ");
            updateSlotNumber(IMT_slot0);
            bootTo(IMT_slot0);
        }
        else if (verifyFwImage(IMT_slot1))
        {
            puts1("\r\nWarning: Forced boot from SLOT_1...\r\n ");
            updateSlotNumber(IMT_slot1);
            bootTo(IMT_slot1);
        }
        else
        {
            puts1("\r\nERROR: None of the slots are valid, entering console.\r\n ");
            consoleLoop();
        }
    }
    else if ((imageSlotNum == IMT_slot0) || (imageSlotNum == IMT_slot1))
    {
        puts1("\r\n\nBootloader: Transferring to ");
        puts1(imageSlotNum == IMT_slot0 ? "SLOT_0" : "SLOT_1");
        if (verifyFwImage(imageSlotNum))
        {
            /* If an image slot has been selected and fw image has been verified, then proceed to boot */
            bootTo(imageSlotNum);
        }
        else
        {
            puts1("\r\nERROR: Fw image verification failed, increase failed boot attempt counter.\r\n");
            failedBootAttemptCnt++;
            updateFailedBootCounter(failedBootAttemptCnt);

            puts1("\r\nBootloader: Attempt to boot from alternative image slot...\r\n\n\n");
            uint32_t newImageSlotNum = (imageSlotNum == IMT_slot0) ? IMT_slot1 : IMT_slot0;
            updateSlotNumber(newImageSlotNum);

            if (verifyFwImage(newImageSlotNum))
            {
                bootTo(newImageSlotNum);
            }
            else
            {
                puts1("\r\nERROR: Fw image verification failed, increase failed boot attempt counter.\r\n");
                failedBootAttemptCnt++;
                updateFailedBootCounter(failedBootAttemptCnt);

                puts1("\r\nERROR: None of the slots are valid, entering console.\r\n ");
                consoleLoop();
            }
        }
    }
    else
    {
        /* Valid image slot hasn't been selected, enter bootloader console */
        consoleLoop();
    }
}

/***********************************************************************
* Functions:          Defininition of local functions
***********************************************************************/
static void updateFailedBootCounter(uint32_t cnt)
{
    puts1("\r\nBootloader: Number of failed boot attempts: ");
    printDecNumber(cnt);

    if (Config_Set_Failed_Boot_Cnt(cnt) != 0)
    {
        puts1("\r\nERROR: Update failed boot counter - config header update failed.\r\n");
    }
}

static void updateSlotNumber(uint32_t slot)
{
    if (Config_Set_Boot_Slot(slot) != 0)
    {
        puts1("\r\nWarning: Update slot number - config header update failed.\r\n");
    }
}

static uint32_t getBootloaderFwVersion(void)
{
    return blMetadata.bl_fw_version;
}

static const char *getBootloaderHash(void)
{
    return (const char *)(blMetadata.hash);
}

static inline void prompt(void)
{
    puts1("BL> ");
}

static void consoleLoop(void)
{
    puts1("\r\n\n");
    puts1("Bootloader Board/Design Rev.: ");
    puts1(Hw_Encoding_Get_Hw_Board_Name());
    puts1(" ");
    printDecNumber(Hw_Encoding_Get_Hw_Design_Revision());
    puts1(".");
    printDecNumber(Hw_Encoding_Get_Hw_Modification_Revision());
    putNl();
    puts1("Bootloader firmware version: ");
    printDecNumber(GET_VERSION_MAJOR(getBootloaderFwVersion()));
    puts1(".");
    printDecNumber(GET_VERSION_MINOR(getBootloaderFwVersion()));
    puts1(".");
    printDecNumber(GET_VERSION_PATCH(getBootloaderFwVersion()));
    puts1("\r\n\n");
    puts1("PMIC Bootloader console.\r\n\n");

    while (1)
    {
        prompt();
        char *line = getLine(1);

        // break into pieces and record start of each
        char *pp[3];
        uint32_t i = 0;
        bool wasSpace = 1;
        for (char *p = line; *p; ++p)
        {
            if (*p == ' ')
            {
                *p = '\0';
                wasSpace = 1;
                continue;
            }
            if (wasSpace)
            {
                pp[i++] = p;
                wasSpace = 0;
                if (i >= countof(pp))
                    break;
            }
        }
        while (i < countof(pp))
            pp[i++] = NULL;

        commandList_t const *pcmd;
        for (pcmd = commandList; pcmd < endof(commandList); ++pcmd)
        {
            if (strSameUpToSpace(pcmd->name, pp[0]))
            {
                (*pcmd->cmd)(pp[1], pp[2]);
                break;
            }
        }
        if (pcmd == endof(commandList) && line[0] != '\0')
        {
            puts1("Bad Cmd\n");
        }
        putNl();
    }
}

static void cmdHelp(char const *p1, char const *p2)
{
    for (commandList_t const *pcmd = commandList; pcmd < endof(commandList); ++pcmd)
    {
        puts1("    ");
        int n = 25 - puts1(pcmd->name);
        while (n--)
            putchar1(' ');
        puts1(pcmd->help);
        puts1("\r\n");
    }
    puts1(" -- All numbers in hex (input and output)\r\n");
}

static void cmdCheckImage(char const *p1, char const *p2)
{
    for (t_IMT imageSlotNum = IMT_slot0; imageSlotNum <= IMT_slot1; ++imageSlotNum)
    {
        puts1(imageSlotNum == IMT_slot0 ? "SLOT_0" : "SLOT_1");
        puts1(":\r\n");
        uint32_t supportedBoardType = Config_Get_Supported_Board_Types_From_Metadata(imageSlotNum);
        uint32_t imageLng = Config_Get_Image_Size_From_Metadata(imageSlotNum);
        uint32_t versionBytes = Config_Get_Fw_Ver_From_Metadata(imageSlotNum);
        uint32_t compatibleBlVer = Config_Get_Compatible_Bl_Ver_From_Metadata(imageSlotNum);

        puts1("Supported board types: ");
        printHexNumber(supportedBoardType, 4);

        putNl();
        puts1("Image lng:  ");
        printHexNumber(imageLng, 8);
        putNl();
        t_Ecksum cks = cksumOk(imageSlotNum);
        puts1("Cksum: ");
        puts1(cksumMsg[cks]);
        putNl();
        if (cks == Ecksum_Ok)
        {
            puts1("Version: PMIC - v");
            printDecNumber(GET_VERSION_MAJOR(versionBytes));
            putchar1('.');
            printDecNumber(GET_VERSION_MINOR(versionBytes));
            putchar1('.');
            printDecNumber(GET_VERSION_PATCH(versionBytes));
            putNl();
            puts1("Compatible bootloader version: v");
            printDecNumber(GET_VERSION_MAJOR(compatibleBlVer));
            putchar1('.');
            printDecNumber(GET_VERSION_MINOR(compatibleBlVer));
            putchar1('.');
            printDecNumber(GET_VERSION_PATCH(compatibleBlVer));
            putNl();

            puts1("Hash: ");
            puts1(Config_Get_Image_Hash_From_Metadata(imageSlotNum));
        }
        putNl();
    }
}

static void cmdBootImage(char const *p1, char const *p2)
{
    if (p1[0] == '2')
    {
        puts1("Already in bootloader\r\n");
        return;
    }
    if (!p1 || (p1[0] != '0' && p1[0] != '1'))
    {
        puts1("Bad Arg: 0 for slot 0, 1 for slot 1\r\n");
        return;
    }

    if (verifyFwImage(p1[0] - '0'))
    {
        bootTo(p1[0] - '0'); // doesn't return
    }
    else
    {
        puts1("Fw image verification failed\r\n");
    }
}

static void cmdReboot(char const *p1, char const *p2)
{
    reboot();
}

static t_Ecksum cksumOk(t_IMT imageSlotNum)
{
    const uint32_t *startAddr = (uint32_t *)(imageAddrs[imageSlotNum]);
    uint32_t imageLength = Config_Get_Image_Size_From_Metadata(imageSlotNum);
    uint32_t cksum = 0;

    if (!(0x100 < imageLength && imageLength <= maxImageSize))
        return Ecksum_Corrupt;

    int status = calculateChecksum(startAddr, imageLength, &cksum);

    if ((status != STATUS_SUCCESS) || (cksum != 0))
        return Ecksum_Corrupt;

    return Ecksum_Ok;
}

static bool verifyFwImage(t_IMT imageSlotNum)
{
    uint32_t fwSupportedBoardTypes = Config_Get_Supported_Board_Types_From_Metadata(imageSlotNum);
    uint32_t bootloaderFwVersion = getBootloaderFwVersion();
    uint32_t compatibleBootloaderVersion = Config_Get_Compatible_Bl_Ver_From_Metadata(imageSlotNum);

    if (GET_VERSION_MAJOR(bootloaderFwVersion) != GET_VERSION_MAJOR(compatibleBootloaderVersion))
    {
        puts1("\r\ERROR: Attempt to boot to image which is not compatible with the bootloader version\r\n");
        return false;
    }

    if (((1 << Hw_Encoding_Get_Hw_Version_Encoding()) & fwSupportedBoardTypes) == 0)
    {
        puts1("\r\nERROR: Attempt to boot to image which is not compatible with the board type\r\n");
        return false;
    }

    if (cksumOk(imageSlotNum) != Ecksum_Ok)
    {
        puts1("\r\nERROR: Attempt to boot to bad image, invalid checksum\r\n");
        return false;
    }

    puts1("\r\nBootloader: Fw image in slot ");
    printDecNumber(imageSlotNum);
    puts1(" is successfully verified.");
    puts1("\r\n\n");

    return true;
}

static void bootTo(t_IMT imageSlotNum)
{
    // move system clock away from GCLK_GENCTRL_SRC_DFLL48M which will be reprogrammed
    // to the stable GCLK_GENCTRL_SRC_OSC8M
    gclk_write_GENDIV(GCLK_GENDIV_ID_GCLK0 | GCLK_GENDIV_DIV(1));
    gclk_write_GENCTRL(GCLK_GENCTRL_ID_GCLK0 | GCLK_GENCTRL_GENEN | GCLK_GENCTRL_SRC_OSC8M);
    gclk_wait_for_sync();

    /* Fetch ivt from flash image which was placed in vectors section */
    register uint32_t const *imageStart = (uint32_t *)(imageAddrs[imageSlotNum]);
    register uint32_t const *ivtCopy = &imageStart[IVT_OFFSET / sizeof(uint32_t)];
    register uint32_t stackStart = ivtCopy[0];
    register uint32_t resetVector = ivtCopy[1];

    /* Set stack start address*/
    __set_MSP(stackStart);

    /* Launch PMIC fw using reset handler from ivt */
    asm volatile("    movs r0, #0            \n" /* Switch to the msp stack. */
                 "    msr  CONTROL, r0    \n");
    ((void (*)(void))resetVector)();
}

static void reboot(void)
{
    __disable_irq();
    NVIC_SystemReset();
    for (;;)
        ;
}

static bool strSameUpToSpace(char const *p1, char const *p2)
{
    while (*p1 && *p2 && *p2 != ' ')
        if ((*p1++ & 0x5F) != (*p2++ & 0x5F))
            return 0;
    return 1;
}

static void printHexNumber(uint32_t v, int d)
{
    v &= ~(((uint32_t)(-1)) << (4 * d));
    while (d--)
    {
        char t = (char)((v >> (4 * d)) & 0x0F);
        putchar1(t < 10 ? '0' + t : 'A' + t - 10);
    }
}

static void printDecNumber(uint32_t v)
{
    uint32_t d = (uint32_t)(1.E9);
    bool p = 0;
    while (d > 0)
    {
        uint32_t g = v / d;
        p |= g > 0 || d < 10;
        if (p)
            putchar1((char)g + '0');
        v = v % d;
        d /= 10;
    }
}

static void putchar1(char const c)
{
    while ((CONSOLE_PORT->USART.INTFLAG.reg & SERCOM_USART_INTENSET_DRE) == 0)
        continue;
    CONSOLE_PORT->USART.DATA.reg = (uint8_t)c;
}

static int puts1(char const *s)
{
    int ret = 0;
    while (*s)
    {
        if (*s == '\n')
            putchar1('\r');
        putchar1(*s++);
        ++ret;
    }
    return ret;
}

static void putNl(void)
{
    puts1("\r\n");
}

static char getChar(void)
{
    while ((CONSOLE_PORT->USART.INTFLAG.reg & SERCOM_USART_INTENSET_RXC) == 0)
        continue;
    return (char)CONSOLE_PORT->USART.DATA.reg;
}

static char *getLine(bool echo)
{
    static char line[80];
    char *p = line;

    while (1)
    {
        char c = getChar();
        if (c == '\r')
        {
            *p = '\0';
            if (echo)
                puts1("\r\n");
            break;
        }
        if (c == '\b' || c == 0x7F)
        {
            if (p > line)
            {
                --p;
                if (echo)
                    puts1("\b \b");
            }
            continue;
        }
        if (p < endof(line) && ' ' <= c && c <= '~')
        {
            *p++ = c;
            if (echo)
                putchar1(c);
        }
    }
    return line;
}

void __libc_init_array(void)
{
    ; /* dummy since libc is omitted */
}
