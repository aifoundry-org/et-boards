cmake_minimum_required(VERSION 3.28)

project(et-board VERSION 0.0)

# Use et-platform cmake utilities. Assumes
# et-platform is installed in /opt/et
#
# If directory is different use:
#
#   -DETPLATFORM_PATH=<et install dir>
#
# When running CMAKE.
if (NOT DEFINED ET_PLATFORM_PATH)
   set(ET_PLATFORM_PATH "/opt/et")
endif()

message("Using ET Platform at ${ET_PLATFORM_PATH}")
list(APPEND CMAKE_PREFIX_PATH ${ET_PLATFORM_PATH}/lib/cmake)
list(APPEND CMAKE_MODULE_PATH ${ET_PLATFORM_PATH}/lib/cmake)
include(aifoundry-utils/ProjectFunctions)

#
# Build libraries
#
set(PROJECT_BASE_DIR "libs")
HostProject(crypto-api "")


#
# Build tools
#
set(PROJECT_BASE_DIR "tools")
HostProject(signing-tools "" crypto-api)

set(PROJECT_BASE_DIR ".")

#
# Compile PMIC Firmware.
#
DeviceProject(et-soc1-pmic "")

set(ARTIFACTS_DIR ${CMAKE_BINARY_DIR}/artifacts)
set(TOOLS_DIR ${CMAKE_SOURCE_DIR}/scripts)
set(CMAKE_DIR ${CMAKE_SOURCE_DIR}/cmake)

#
# Assemble asset data.
#
HostProject(asset-config-data
	"-DDEVICE_ARTIFACTS_LIB_DIR=${ARTIFACTS_DIR}
	 -DDEVICE_ARTIFACTS_TOOLS_DIR=${TOOLS_DIR}")

set(SIGN_TOOLS_DIR ${CMAKE_INSTALL_PREFIX}/bin)
set(ARTIFACTS_DIR ${CMAKE_BINARY_DIR}/artifacts)
set(ASSET_CONFIG_DIR ${ARTIFACTS_DIR}/asset-config-data)
set(PMIC_FW_DIR ${CMAKE_INSTALL_PREFIX}/lib/pmic-fw)

set(CMAKE_VERBOSE_MAKEFILE ON)

find_package(EsperantoBootLoader REQUIRED)
find_package(EsperantoDeviceMinionRuntime REQUIRED)

get_property(SP_BL1_ELF TARGET EsperantoBootLoader::ServiceProcessorBL1.elf PROPERTY LOCATION)
get_property(SP_BL2_PROD_ELF TARGET EsperantoBootLoader::ServiceProcessorBL2_production.elf PROPERTY LOCATION)
get_property(SP_BL2_TF_ELF TARGET EsperantoBootLoader::ServiceProcessorBL2_testframework.elf PROPERTY LOCATION)
get_property(SP_BL2_LINUX_ELF TARGET EsperantoBootLoader::ServiceProcessorBL2_linux.elf PROPERTY LOCATION)
get_property(MACHINE_MINION_ELF TARGET EsperantoDeviceMinionRuntime::MachineMinion.elf PROPERTY LOCATION)
get_property(MASTER_MINION_ELF TARGET EsperantoDeviceMinionRuntime::MasterMinion.elf PROPERTY LOCATION)
get_property(MASTER_MINION_TF_ELF TARGET EsperantoDeviceMinionRuntime::MasterMinionTF.elf PROPERTY LOCATION)
get_property(WORKER_MINION_ELF TARGET EsperantoDeviceMinionRuntime::WorkerMinion.elf PROPERTY LOCATION)

configure_file(scripts/sign_fw_artifacts.sh.in ${CMAKE_BINARY_DIR}/scripts/sign_fw_artifacts.sh @ONLY)

set(SIGNED_FW 		${CMAKE_BINARY_DIR})
set(ASSET_CONFIG_IMAGE  ${SIGNED_FW}/signed_asset_config.img)
set(VAULT_FW_ROM_IMAGE  ${SIGNED_FW}/firmware_eip130_ram_2.5.13_rom_keys.sbi)
set(ROM_CERT            ${SIM_HSM_DIR}/certificate-output/production_sp.crt)
set(PCIE_CONFIG_IMAGE   ${SIGNED_FW}/signed_pcie_config_def_phy_fw_gen4.img)
set(BL1_IMAGE           ${SIGNED_FW}/signed_bl1.img)
set(BL2_IMAGE           ${SIGNED_FW}/signed_bl2.img)
set(MACHINE_MIN_IMAGE   ${SIGNED_FW}/signed_machine_minion.img)
set(MASTER_MIN_IMAGE    ${SIGNED_FW}/signed_master_minion.img)
set(WORKER_MIN_IMAGE    ${SIGNED_FW}/signed_worker_minion.img)
set(PMIC_IMAGE_S0       ${SIGNED_FW}/signed_pmic_s0_fw.img)
set(PMIC_IMAGE_S1       ${SIGNED_FW}/signed_pmic_s1_fw.img)

configure_file(templates/flash_32Mbit_unsigned.json.in ${CMAKE_BINARY_DIR}/templates/flash_32Mbit_unsigned.json)
