#------------------------------------------------------------------------------
# Copyright (c) 2025 Ainekko, Co.
# SPDX-License-Identifier: Apache-2.0
#------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.15)
project(EsperantoPMIC VERSION 1.7.0 LANGUAGES C ASM)

message(STATUS "PROJECT_VERSION ${PROJECT_VERSION}")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

message(STATUS "CMAKE_MODULE_PATH: ${CMAKE_MODULE_PATH}")
message(STATUS "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")

if (DEFINED GIT_HASH_STRING)
    set(GIT_HASH_STRING "\"${GIT_HASH_STRING}\"")
endif()
if (DEFINED GIT_VERSION_STRING)
    set(GIT_VERSION_STRING "\"${GIT_VERSION_STRING}\"")
endif()
message(STATUS "GIT_HASH_STRING:    ${GIT_HASH_STRING}")
message(STATUS "GIT_HASH_ARRAY:     ${GIT_HASH_ARRAY}")
message(STATUS "GIT_VERSION_STRING: ${GIT_VERSION_STRING}")
message(STATUS "GIT_VERSION_ARRAY:  ${GIT_VERSION_ARRAY}")

set(ET_DEFAULT_BUILD_TYPE "Debug")

# TODO: SW-18823: Remove the override. CMAKE_BUILD_TYPE is overriden with Debug because Release build has issues
set(CMAKE_BUILD_TYPE ${ET_DEFAULT_BUILD_TYPE})

########## Project options ###################################

option(ENABLE_STRICT_BUILD_TYPES "Whether project configuration should fail if build_type is not one of the allowed values: Debug Release RelWithDebInfo MinSizeRel" ON)
option(ENABLE_REPORT_GLOBAL_FLAGS "Wether global flags should be shown when configuring the project" ON)
include(CommonProjectSettings)
include(DeviceUtils)
include(GNUInstallDirs)

cmake_policy(SET CMP0076 NEW)

option(ENABLE_WARNINGS_AS_ERRORS "Treat warnings as errors" ON)
if(ENABLE_WARNINGS_AS_ERRORS)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")
endif()

option(BUILD_DOC "Build documentation" OFF)

########## Dependencies #####################################

# installation related vars (to replace in the future for GnuInstallDirs)
set(DOCUMENTATION_INSTALL_DIR  "${CMAKE_INSTALL_PREFIX}" CACHE STRING "Documentation installation path")
set(BIN_INSTALL_DIR bin CACHE STRING "Bin install dir")
set(INCLUDE_INSTALL_DIR include/ CACHE STRING "Include install dir")
set(LIB_INSTALL_DIR  lib/  CACHE STRING "Library install dir")
set(SYSCONFIG_INSTALL_DIR etc/et-fw/ CACHE STRING "Config install dir")

########## Documentation #####################################

if (BUILD_DOC)
    # check if Doxygen is installed
    find_package(Doxygen REQUIRED)

    add_subdirectory(doc)
endif()

############################ FW build Rules ###############

# TODO: Convert this repository to use CMAKE instead of Makefiles
# TODO: Support parallel build. Enforced j1 here since Makefiles do not support parallel build
string(TOLOWER ${CMAKE_BUILD_TYPE} BUILD_TYPE_LOWER)

# Re-builds if there is change in following source files
file(GLOB_RECURSE ET_PMIC_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR} *.[ch] Makefile)

# generate bootloader image
add_custom_command(
    DEPENDS ${ET_PMIC_SOURCE_FILES}
    COMMAND $(MAKE) -C ${CMAKE_CURRENT_SOURCE_DIR} -j1 clean-build bootloader BUILD_DIR=$(PWD)/pmic-bootloader BUILD_TYPE=${CMAKE_BUILD_TYPE}
    COMMAND mkdir -p pmic-fw && cp pmic-bootloader/bootloader/bootloader.bin -t pmic-fw
    OUTPUT  pmic-fw/bootloader.bin
    VERBATIM
)

# generate flash image (built only for default slot (slot0))
set(build_dir pmic-flash-image)
add_custom_command(
    DEPENDS ${ET_PMIC_SOURCE_FILES}
    COMMAND $(MAKE) -C ${CMAKE_CURRENT_SOURCE_DIR} -j1 clean-build full-pmic-flash-image BUILD_DIR=$(PWD)/${build_dir} BUILD_TYPE=${CMAKE_BUILD_TYPE} SLOT=0 IMAGE_DIR=$(PWD)/pmic-fw
    OUTPUT  pmic-fw/pmic_flash_image_${BUILD_TYPE_LOWER}.bin
    VERBATIM
)

# generate standalone image
set(build_dir pmic-standalone-image)
add_custom_command(
    DEPENDS ${ET_PMIC_SOURCE_FILES}
    COMMAND $(MAKE) -C ${CMAKE_CURRENT_SOURCE_DIR} -j1 clean-build full-pmic-flash-image BUILD_DIR=$(PWD)/${build_dir} BUILD_TYPE=${CMAKE_BUILD_TYPE} SLOT=2 IMAGE_DIR=$(PWD)/pmic-fw
    OUTPUT  pmic-fw/pmic_standalone_image_${BUILD_TYPE_LOWER}.bin
    VERBATIM
)

# generate images for slot0 and slot1
FOREACH(slot 0 1)
    set(build_dir pmic-slot${slot})
    add_custom_command(
        DEPENDS ${ET_PMIC_SOURCE_FILES}
        COMMAND $(MAKE) -C ${CMAKE_CURRENT_SOURCE_DIR} -j1 clean-build runtime BUILD_DIR=$(PWD)/${build_dir} BUILD_TYPE=${CMAKE_BUILD_TYPE} SLOT=${slot}
        COMMAND mkdir -p pmic-fw && cp ${build_dir}/runtime/runtime.bin pmic-fw/runtime_slot${slot}.bin
        OUTPUT  pmic-fw/runtime_slot${slot}.bin
        VERBATIM
    )
    set(build_dir pmic-slot${slot}-image)
    add_custom_command(
        DEPENDS ${ET_PMIC_SOURCE_FILES}
        COMMAND $(MAKE) -C ${CMAKE_CURRENT_SOURCE_DIR} -j1 clean-build pmic-slot-image BUILD_DIR=$(PWD)/${build_dir} BUILD_TYPE=${CMAKE_BUILD_TYPE} SLOT=${slot} IMAGE_DIR=$(PWD)/pmic-fw
        OUTPUT  pmic-fw/pmic_slot${slot}_image_${BUILD_TYPE_LOWER}.bin
        VERBATIM
    )
ENDFOREACH()

add_custom_target(build-et-pmic-all ALL
    DEPENDS pmic-fw/bootloader.bin
    DEPENDS pmic-fw/runtime_slot0.bin
    DEPENDS pmic-fw/runtime_slot1.bin
    DEPENDS pmic-fw/pmic_flash_image_${BUILD_TYPE_LOWER}.bin
    DEPENDS pmic-fw/pmic_slot0_image_${BUILD_TYPE_LOWER}.bin
    DEPENDS pmic-fw/pmic_slot1_image_${BUILD_TYPE_LOWER}.bin
    DEPENDS pmic-fw/pmic_standalone_image_${BUILD_TYPE_LOWER}.bin
    VERBATIM
)

########## Installation #####################################

install(DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/pmic-fw
    DESTINATION ${LIB_INSTALL_DIR}
)

install(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/scripts/pmic_helper.sh
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    RENAME pmic-helper
)

include(CMakePackageConfigHelpers)

# Create EsperantoPMIC interface library
add_library(EsperantoPMIC INTERFACE)
target_include_directories(EsperantoPMIC
    INTERFACE
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

install(TARGETS EsperantoPMIC
    EXPORT EsperantoPMICTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/et-pmic
)

# Export the package for use from the build-tree
# (this registers the build-tree with a global CMake-registry)
export(PACKAGE EsperantoPMICTargets)

# Install the export set for use with the install-tree
install(EXPORT EsperantoPMICTargets
	NAMESPACE EsperantoPMIC::
	DESTINATION "${LIB_INSTALL_DIR}/cmake/EsperantoPMIC"
)

# Create the *-config.cmake and *-config-version.cmake files
configure_package_config_file(
	${CMAKE_CURRENT_SOURCE_DIR}/EsperantoPMICConfig.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/EsperantoPMICConfig.cmake
	INSTALL_DESTINATION ${LIB_INSTALL_DIR}/cmake/EsperantoPMIC
    PATH_VARS INCLUDE_INSTALL_DIR SYSCONFIG_INSTALL_DIR LIB_INSTALL_DIR BIN_INSTALL_DIR
)
write_basic_package_version_file(
	${CMAKE_CURRENT_BINARY_DIR}/EsperantoPMICConfigVersion.cmake
    VERSION ${VERSION}
    COMPATIBILITY SameMajorVersion
)

# Install the *Config.cmake and *ConfigVersion.cmake
install(FILES
	${CMAKE_CURRENT_BINARY_DIR}/EsperantoPMICConfig.cmake
	${CMAKE_CURRENT_BINARY_DIR}/EsperantoPMICConfigVersion.cmake
	DESTINATION ${LIB_INSTALL_DIR}/cmake/EsperantoPMIC
    COMPONENT pmic
)

