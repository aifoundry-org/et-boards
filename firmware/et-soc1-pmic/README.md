# ET-SOC1-PMIC

## This is the repository for storing the ET-SOC-1 PMIC controller source code.

The PMIC controller is an Atmel M0 processor that handles all the low level real-time control of the voltage regulators for the ET-SOC-1.  

See license.md for library license info.

### Building Firmware Images

This is cross compiled using the arm-none-eabi-gcc and arm-none-eabi-* binutils with GNU make.

Compiling a project requires `git clone` of this project. Enter repository root directory `et-soc1-pmic`, and run a docker container:

```
cd et-soc1-pmic
git submodule update --init --recursive
./dock.py prompt
```
Project is built from the docker container. In root directory `et-soc1-pmic`, the same level as the `Makefile`

#### Building Full Firmware Image (bootloader + slot specific runtime)

To generate image suitable for programming microcontroller, which would contain both the bootloader and runtime code (in either slot 0 or slot 1 position) in a single image file, run:
```
make clean full-pmic-flash-image BUILD_DIR=<build_dir> IMAGE_DIR=<image_dir> BUILD_TYPE=<build_type> SLOT=<slot>
```
* BUILD_DIR / IMAGE_DIR are the respective output directories for the build artifacts and image
* BUILD_TYPE can be either "Debug" or "Release" (defaults to "Debug").
* SLOT specifies the runtime slot, either 0 or 1

Image `pmic_flash_image_<build_type>.bin` is placed in `et-soc1-pmic/<image_dir>` and build artifacts are placed in `et-soc1-pmic/<build_dir>`

`pmic_flash_image_<build_type>.bin` is a unified FW that supports both BUB and PCIe.  This is handled by BOARD_ID, which is specific to the hardware that the FW is running on.  This allows for boot sequences or other hardware specific routines to differ depending on the physical hardware, but still use the same FW image

`The ``et-soc1-pmic/<build_dir>` contains the build artifacts for both the `bootloader` and `runtime` in their respective directory.  Each of them contain \*.elf and \*.bin image files that can be programmed to the microcontroller. These directories also hold dependency files, intermediate object files and \*.lss, \*.map and \*.sym files which are disassembly, identifier map file, and symbol file, respectively, for debugging.

#### Building Firmware Update Image (slot specific runtime)

Next to the bootloader, MCU flash memory has two runtime partitions (slots), and while runtime is running from the active slot, passive slot can be used to store new runtime firmware update image.

To generate image suitable for firmware update, run:
```
make clean pmic-slot-image BUILD_DIR=<build_dir> IMAGE_DIR=<image_dir> BUILD_TYPE=<build_type> SLOT=<slot>
```
Update image `pmic_slot<slot>_image_<build_type>.bin` is placed in the `et-soc1-pmic/<image_dir>` directory.

Executables aren't generated as position independent code, therefore slot 0 and slot 1 images are not interchangeable, and slot must be specified.

### Flashing the PMIC
Once the image or update is generated, it needs to be programmed onto the board
#### Over the Air Update
Over the air (OTA) updates can be used to update systems remotely while they are still active.  This uses the `et_dm_service` utility to interact with the target card via the host computer.

As of now OTA updates support ETSOC+PMIC images.  To generate this combined image, refer to building the sw-platform FW

That combined image is then passed as the argument to this et_dm_service command to update OTA
```
et_dm_service -m DM_CMD_SET_FIRMWARE_UPDATE -n <device_id> -i <firmware_path>
```
In order to update the PMIC on actively running cards, we make use of a Ping Pong update pattern.  This means that for each update, the target slot ping pongs between 0 and 1.  This allows us to update the non active slot, while the active slot is still running.  Then when the PMIC is reset it will boot into the newly programmed slot.

There are some additional requirements for the update to actually take effect.

### Interacting With PMIC
#### Via UART
The PMIC can be directly communicated with over the UART ports.  This requires a "mini" USB cable connection to the card on the UART port.

To open the PMIC console on the computer that connects to the UART cable, run:
```
minicom
```
This will open the PMIC UART which can be interfaced with.  Below are some useful commands for most debug / development.  A full list of commands can be seen by running:
```
> help
All parameters are unsigned hexadecimal preceded by 0x or decimal default
i2c-w            | iw      -> Write to the I2C Bus <adr> <reg> [data [data]...]
i2c-r            | ir      -> Read from the I2C Bus <adr> <reg> <byte count>
i2c-rr           | irr     -> Read repeatedly from the I2C Bus <adr> <reg> <byte count>
i2c-wr           | iwr     -> Generic I2C Master Write-Read <adr> <read count> [wr data ...]
set-gpio         | sg      -> Set/Show GPIO <name> [value], omit value for read else [0|1|in|out]
i2c-scan         | is      -> Perform alien probe
...
```
In the following, we will be using the shorthand command names, listed to the right of the pipes, either can be used.

Here we can check PMIC FW version / commit hash.  This is useful to confirm that a PMIC FW update worked, or to get the current version of an unknown FW.
```
> ver 0
FW Version: v1.1.0

Metadata version: v1.0.0
Compatible BL version: v1.0.0
SP-PMIC interface version: v0.0.0
Platform: PCIE, REV 3, processor type: J18
Image type:         SLOT_0
Checksum            Valid

bootloader   present TRUE
SLOT_0 image present TRUE
SLOT_1 image present FALSE

Commit info: Fri Dec 15 18:56:43 2023 +0000, branch: (HEAD detached at eef102b), hash: eef102b1
> 
```
We can also send SOC Reset to the card, which causes the SOC to reboot
```
> rsoc
SOC reset
>
```
A useful utility is the Voltage Monitor or VM. Inside the VM, we can directly interact with the different regulators that drive power.  Sending "help (or ?)" will give different commands than before
```
> vm
V> help
list             | l       -> List programmable regulators
read_v           | r       -> Read voltage [<reg_id>] skip reg_id to read all
set_v            | s       -> Set voltage <reg_id>, <mV>
clr_pmb          | cpm     -> Clear PMB - min, max, ave
read_pmb         | rpm     -> Read info <reg_id>, [1] (1 for dump)
x                | q       -> Exit to Main Menu
history          | hi      -> Show command history
help             | ?       -> Print Help Information
```

The following reads the MNN supply (Minion), sets it to 500mV, and then reads the MNN again
```
V> r MNN
MNN    SOC MNN          :  400 mV
V> s MNN 500
Writing 0032 to reg: 21  (register index 20, code 32)
V> r MNN
MNN    SOC MNN          :  500 mV
V>
```
The VM can list more detailed regulator information with the following
```
V> l
    Some regulators cannot produce listed command voltages.
    Curr value is nearest possible value in those cases.
    All voltages in millivolts
ID     Description          CurV  Nom  Min  Max  Step
QLP    V0P6 DDQLP            640  640  530  680 10.00
SRM    SOC SRAM              700  700  500 1000  5.00
DDR    SOC DDR               850  850  700 1000  5.00
DDQ    V1P1 DDR VDDQ        1096 1100  600 1170 10.00
PCL    V0P75 PCIE LGC        775  775  625  888  6.25
PCI    V1P5 PCIE            1565 1563 1400 1800 12.50
MXN    SOC MAXION            850  850  600 1000  5.00
NOC    SOC NOC               450  450  305  850  5.00
MNN    SOC MNN               500  400  305  850  5.00


Cur  ID  V_out  A_out  W_out   V_in   A_in   W_in  Deg_C
    MNN  0.494  3.136  1.638 12.156  0.000  0.000 30.531
         xABF5  xC323  xBB47  xD30A  xB800  x8800  xDBD1
    NOC  0.446  1.552  0.530 12.156  0.190  0.000 27.593
         xAB92  xBB1B  xB21F  xD30A  xA30D  x8800  xDB73
    SRM  0.700  0.439  0.307 12.171  0.153  1.861 32.406
         x0B34  xA3CC  xA2A9  xD30B  xA273  xBBB9  xE209

Min  ID  V_out  A_out  W_out   V_in   A_in   W_in  Deg_C
    MNN  0.392  1.568  0.640 12.078  0.000  0.000 29.781
         xAB24  xBB23  xB290  xD305  xB800  x8800  xDBB9
    NOC  0.439  0.784  0.335 12.078  0.000  0.000 26.843
         xAB85  xB323  xAAAF  xD305  xB800  x8800  xDB5B
    SRM  0.671  0.141  0.098 12.093  0.131  1.595 30.312
         x0A4A  xA242  x9B29  xD306  xA219  xBB31  xDBCA

Max  ID  V_out  A_out  W_out   V_in   A_in   W_in  Deg_C
    MNN  0.496 35.062 13.937 12.171  1.974 23.875 31.500
         xABF8  xE231  xD37C  xD30B  xBBF3  xDAFC  xDBF0
    NOC  0.449  3.812  1.714 12.171  1.916 23.062 28.562
         xAB98  xC3D0  xBB6E  xD30B  xBBD5  xDAE2  xDB92
    SRM  0.701  0.540  0.378 12.171  0.170  2.070 32.812
         x0B36  xAA70  xA369  xD30B  xA2BB  xC212  xE20F

Ave  ID  V_out  A_out  W_out   V_in   A_in   W_in  Deg_C
    MNN  0.493  3.327  1.663 12.155  0.179  2.231 30.547
    NOC  0.445  1.400  0.622 12.156  0.187  2.332 27.594
    SRM  0.700  0.312  0.218 12.168  0.152  1.857 32.475

NSamples: 390740, NErrors: 0
Inputs: Volts: 11.587V, Amps:  0.826A, Watts:  9.571W, Average Watts:  9.530W
```
#### Via et_dm_service
To use the UART as above, the UART cable needs to be attached.  On systems where this is not possible (i.e. in a server rack), we can use the et_dm_service to interface with the PMIC instead.  From the host PC run
```
et_dm_service -h
```
This will give a list of valid et_dm_service commands, some of which can be used for 

TODO: Add more detailed information about using et_dm_service to interact with PMIC when UART isn't available -->
#### PMIC to et_dm_service Mapping
The PMIC regulators and et_dm_service components have slightly different naming conventions, here is how they map
```
PMIC| et_dm_service
QLP | VDDQLP
SRM | L2CHACHE
DDR | DDR
DDQ | VDDQ
PCL | PCIE_LOGIC
PCI | PCIE
MXN | MAXION
NOC | NOC
MNN | MINION
```

### Directory structure

* runtime - contains files specific to the firmware runtime
  * CLI - command line interface service in runtime layer that runs on top of RTOS
    * inc and src
  * ETSOCCommandHandler - service that runs on top of RTOS and is responsible for handling the communication between ETSOC and PMIC
    * inc and src
  * FirmwareUpdateManager - service that runs on top of RTOS and is responsible for handling the firmware update process
    * inc and src
  * PowerManager - service that runs on top of RTOS and is responsible for handling voltage, current, and power related measurements and communication with regulators
    * inc and src
  * Startup - files related to starting up the firmware runtime
    * inc and src
* bootloader - contains files specific to the bootloader
  * inc and src - bootloader related code
* common - contains files used by bootloader and runtime fw
  * Config - build configuration related definitions
  * HardwarePlatform
    * BoardSpecific
      * BUB - bring up board specific project files
      * PCIE - pcie project specific files
    * Drivers
      * inc and src - additional drivers and support software for cortex M0 processors, shared among all board variants
  * Linker
  * System - contans system related files
  * Utils - contains all helper functions
  * ThirdParty - third party files used by bootloader and runtime firmware
    * asf3 - atmel software framework - stripped down version of asf3 processor specific include and source
    * FreeRTOS - Real time Operating system for Microcontrollers

### Debug with Logic Analyzer
TODO: Add steps for connecting instruments to PMIC, asked Bruce if document exists
