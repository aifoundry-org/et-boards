# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

[[_TOC_]]

## [Unreleased]
### Added
### Changed
 - [SW-21990] Change PCL max voltage to be consistent with pmic
 - [SW-22053] Set config registres back to init values before etsoc reset
 - [SW-22152] Use average power to determine over-power event
### Deprecated
### Removed
### Fixed
### Security

## [1.6.0] - 2024-09-25
### Added
### Changed
- Updated version to 1.6.0
- Change PCL max voltage value to match limits on SP side
- [SW-21004] Don't handle PERST down if SOC reset is already deasserted
- [SW-21046] Refactor PERST handling
### Deprecated
### Removed
### Fixed
- [SW-21872] Add counter for handling consecutive ADC alarm events
### Security

## [1.5.0] - 2024-05-17
### Added
### Changed
- Updated version to 1.5.0
- [CI] updated docker tag
### Deprecated
### Removed
### Fixed
### Security

## [1.4.0] - 2024-04-29
### Added
 - [SW-15170] Add handling of power, voltage, and temperature alarms
 - [SW-16650] Add handling of i2cs and i2cm errors
### Changed
- Change Docker image from CentOS to Ubuntu
### Deprecated
### Removed
### Fixed
### Security

## [1.3.0] - 2024-03-27
### Added
- [CS-224] Added support of floating Board ID pins
- [SW-20104] Protect regulator page access
- [SW-20181] Add additional Regulator ReadWrite voltage tests
### Changed
- [SW-20007] Update README.md with latest PMIC FW development instructions
- Moving PCL_LOGIC_EN ahead in the power up sequence, before MNN/NOC
- [SW-20412] Update bootloader fw version to 1.0.1 
- Minor fixes to I2C master init sequence
- [SW-20417] Update regulator Min/Max values with Production ready ranges
### Deprecated
### Removed
### Fixed
- [SW-20599] Fix QLP to address marginal DDR training
- [SW-20379] Update default Boot Value to match SOC spec
- [SW-18835] Update regulator conversion tables
- [SW-20314] Update power up sequence delay
- Fixed some code smells
- Update fw version to 1.3.0
- [SW-20418] Fix SP failing to boot for specific parts
- [SW-20412] Fix updating config header in release build 
### Security

## [1.2.0] - 2024-01-30
### Added
- [CS-220] Added support of FRU information storage
### Changed
- Changed BUB V2 board ID from 7 to 6
### Deprecated
### Removed
### Fixed
### Security

## [1.1.0] - 2023-12-15
### Added
### Changed
### Deprecated
### Removed
### Fixed
- Fix the version define name for deploy job.
- [SW-19410] Fix check regulator power good on IO exander.
- [SW-19429] Improve power down and power cycle sequence.
- [SW-19263] Fix I2C slave synchronization between write adnd read requests.
- [SW-18988] Fix for MNN and NOC override each other.
### Security

## [1.0.0] - 2023-11-07
### Added
- [SW-18226] Added bootloader fw version vs. app fw version compatibility check
- [SW-18215] PMBus - Updated regulators to use PMBus command defines
    - PMBus register definitions and field definitions
- [SW-18575] Fixed CLI processing for empty line
- Added IMAGE_DIR parameter in Makefile
- [SW-18760] Added CMake target to build PMIC images
- [SW-18884] Build PMIC standalone images also with the cmake target
- [SW-18883] Export cmake path variables for PMIC slot and standalone images
- [SW-18674] Added support for failed boot counter.
- Added new encoding for BUB without workaround
- [SW-19089] Metadata check suppport, add hash to bootloader metadata.
- [SW-18673] Use PMIC OTA to update PMIC firmware in CI jobs
- [SW-18872] Added support for Penguin RevB
### Changed
- Only deploy the release images if repository tag is created.
- Enabled clang-format CI job.
- Change Docker verion to support CLANG
- Rename output image to `pmic_{bub|pcie}_slot{0|1}_image.bin` in accordance with target name pmic-slot-image
- [SW-18828] Save non-zero values for PMB stats.
- [SW-18592] Refactored gpio driver
- [SW-18592] Remove compile-time board type dependency
- Changed BUB ID to reflect the new value
- [SW-19028] Change version to 1.0.0 to provide FOTA compatibility
- Renamed Application to Runtime to match the actual functionality of this Firmware
- Reverting the sw-platform branch.
- [SW-19029] Update metadata and add new SP commands
- [SW-19089] Rework failed boot sequence.
### Deprecated
### Removed
- [SW-18226] Removed unnused bootloader cli commands
- [SW-18673] Removed flasher based programming of PMIC firmware from CI jobs
### Fixed
- Removed legacy Bootloader checks for MCU and Uart console
- Fixing the PMIC FW version to always read from metadata.
- Fixed `image` build parameter rename to `IMAGE` in CI
- [SW-18686] Fixed code smells
- Make default bootslot 0 if none is configured in config header
- [SW-19094] Fix etsoc_cmd_handler init
### Security

## [0.9.0] - 2023-09-01
### Added
- Adding logging to I2Cs driver.
- [SW-18068] Added update-image make target and consolidated update scripts.
- [SW-18128] Added checksum verification to the bootloader.
- [SW-17350] Separate bootloader for main app fw memory regions. Move confgHeader struct to userPage section.
- [SW-15352] Add checking board id pins.
### Changed
- Clock EIC from 12 MHz source in bootloader.
### Deprecated
### Removed
### Fixed
- [MFGOPS-75] Fixed SCW PWM mode to enable High current mode
- Fix the flash-image organization in genFlashImage.py script.
### Security

## [0.8.0] - 2023-08-03
### Added
- [SW-16375] Event handlers
- [SW-16807] Add script in CI to execute smoke test and save output to logfile
- [SW-16371] PMIC flash image size to be in limits
- [SW-17110] Flash image build using make file, use flash image to run smoke test
- [SW-16875] Addition of I2C slave init and deinit functions
- [SW-17165] BUB smoke test in PMIC CI
- [SW-17139] Add double buffer to store fw image data before writing to flash
- [SW-17166] Add CI job for DM tests
- [SW-17112] Update configuration header with PMIC firmware image information
- [SW-17159] Added switch-slot command to switch the boot slot.
- [SW-17375] Adding PMIC image metadata at 0x100 offset.
- [SW-17374] Error codes: CMD handler component
- [SW-17377] Add post build script to insert PMIC image metadata.
- [SW-17788] Error codes: Bootloader component
- [SW-17787] Add error codes to IO Task and Power manager component
- [SW-17789] Added error codes to utils component
- [SW-18013] Added error codes to CLI component
### Changed
- Starting 0.8.0 development
- Replacing -x with -s to verify the binary images in CI.
- [sw-16849] Changed MIN/MAX voltages to enable characterization work
- [SW-16490] PMICTask renamed to etsoc_command_handler_task and refactored
- [SW-16848] Integrating ETSOC event handler changes with cleaned up I2C driver
- [SW-16740] ETSOC command handler refactoring follow up
- Extended the deploy image CI job to upload the PCIe debug flash image as well.
- [SW-16507] Moved PMIC RDY to I2C slave driver
- [SW-17072] Reduced image size by optimizing smoke test
- [SW-16780] Removed interrupt locks from I2C driver
- [SW-17044] Add pmic_smoke_test script and shift its CI to mv-dev-rst01
- [SW-17160] Verify image size only by excluding 8k for bootloader after build
- [SW-17044b] pmic_smoke_test: host_power_cycle to restore the host on completion
- Fixed BOOTLOADER_IMG_SIZE is not evaluated properly
- Improve Register format prints for FS-1406
- [SW-17206] Workaround: NVM controller ready - use polling instead of interrupt
- [SW-17166c] Restore PMIC FW on BUB after completion of DM CI
- [SW-17166d] Run integration-tests CI job only when all jobs of .pre stage complete
- [SW-17373] Updating Pointer to use develop/system for DM regressions
- Add dependency on test stage instead of specific jobs.
- [SW-17843] Run restore jobs only if smoke test jobs are completed (with success or failure)
- [SW-17859] Changing default NOC voltage to 425 mV.
- [SW-18215] PMBus - Updated regulators to use PMBus command defines
### Removed
- Remove large PDF files - which taking up alot of space
### Fixed
- Fix the version file path in gitlab CI deploy job.
- Fixed the intermittent boot time issues.
- Fixed the SP command extra offset max range.
- Fixed multiple bugs with Regulator Error handling
- Fixed update image writing to the flash
- [SW-17072] Fixing the sizes for flash image checks in makefile.
- [SW-17047] Fix execution blocking during fw update image transfer from SP
- Fixed the array size of voutDisabled to match Vout
- [SW-17166b] handle rebooting of hosts containing bub devices
- [SW-17159] Added and fixed the support of building FW slot 0 and 1 image from make.
- [SW-17383] Integerate usb_reset.pl script in PMIC CI to fix intermittent PMIC UART hang.
- [SW-17342] Fixed FOTA by fixing I2C register syncronization and increasing update task priority
- [SW-17463] Fixed MAJOR sonarqube code smells.
- [SW-17349] Added wait for flash ready before sending write or erase commands
- [SW-17463b] Fixed SonarQube code duplication
- [SW-17455] Adding script for generating FW update image and fixed FW version and git hash commands.
- [SW-17565] Fixed I2C slave driver (SAMD20 errata workaround).
- [SW-17453] Fixing the config header states.
- [SW-17505] Set SP registers only if SP command is successfully finished.
- [SW-17356] Fixed updating configuration header with memcpy
- [SW-17722] Fix fast task queue overflow.
- [SW-17042] Smoke test fixes and improvements.
- [SW-17164] Fixed code smells
- [SW-17838] ADC voltage calculation cleanup.
- [SW-17901] Fix for receiving i2cs interrupts with flag 0.
- [SW-17768] Fix i2cm timeout in readPmbStats.
- [SW-17837] Fix MNN and NOC addresses used to get detail status.
- Moving to gitlab
- [SW-17964] Fixed PCL default votlage to address PCIe violations
- [SW-16885] Fixed bad regulator data at startup
### Security

## [0.7.1] - 2023-03-01
### Added
- Support for debug and release builds.
### Changed
- [SW-16227] Reducing the size of FW image by using Floats instead of double.
-  Modifying CI jobs to have debug and release variants.
- [SW-16056] Task files cleanup.
### Removed
### Fixed
- Fixed SPIP command to enable the 1V8
- [SW-16251] Fix the offset of slot1 image for flash image.
- Fixed intermittent SRAM boot issue
### Security

## [0.7.0] - 2023-02-16
### Added
- [sw-16169] - fixed boardchipinfo.h
- [sw-16169] - remove from common/chips/ MAX77714.h, MAX77812.h, TPS546D24A.h
- /sw-16124] tranafer fic back to master in branch mmas/sw16124-2
- Enabled a BUB2 Makefile and CI stage
- [SW-15859] Add Esperanto Copyright notes and update documentation
- [SW-15693] Adding genFlashImage.py script to generate flash image including bootloader.
- [SW-15911] Adding config header place holder in bootloader
- [SW-15932] Reorganize code in sections - file group around main file; fix SonarQube issues
- [SW-15910] Removing post processing of IVT while generating flash image and adding IVT in BL image
- [SW-15911] Adding support for config header for bootloader.
- [SW-15911] More cleanups and complying to config header fields.
- [SW-15932] Update folder structure
- [SW_16207] PMIC fw boot from slot defined in configuration header
### Changed
- Fix version update to SemVersion convention
- Enable back CI job for version check
### Deprecated
### Removed
- Removing .svn directory from FreeRTOS
### Fixed
- [sw-15171] Add initializations to all LTM4680 registers
- BUB2 related fixes
- [SW-15658] Fix SonarQube bugs and code smells
- [SW-15489] Fix code smells
- SW-16124 - Fix race condition in shutting down regulators
### Security

## [0.6.8] - 2023-01-10
### Added
- Added Sonar Code Analysis stage to CI pipeline
- Added verify stage to CI pipeline
- Added deploy stage to CI pipeline to release PMIC FW image
### Changed
- Enabled Linux based compile flow
- Closing development for 0.6.8
- Combined pcie3 and bub2 source code, fixed bug in bub2 PG sensing, changed code in IoTask.c necessary to allow it to work in both cases.
### Deprecated
### Removed
### Fixed
- Fix Blocker and Critical Sonar Analysis failures
### Security

## [0.6.7] - 2022-12-28
### Added
### Changed
- Removed redundant files, and clean up directory structure
### Deprecated
### Removed
### Fixed
### Security

## [0.6.6] - 2022-09-11
### Added
- Initial PMIC FW source ported from GITHUB.
### Changed
### Deprecated
### Removed
### Fixed
### Security
