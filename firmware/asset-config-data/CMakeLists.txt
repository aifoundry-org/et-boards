#------------------------------------------------------------------------------
# Copyright (c) 2025 Ainekko, Co.
# SPDX-License-Identifier: Apache-2.0
#------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.5)

project(EsperantoDeviceArtifactsAssetConfigData VERSION 0.1.0)

set(FW_RELEASE_VER_MAJOR      1)
set(FW_RELEASE_VER_MINOR      4)
set(FW_RELEASE_VER_REVISION   0)

 math(EXPR FW_RELEASE_VERSION
    "(${FW_RELEASE_VER_MAJOR} << 24) + \
     (${FW_RELEASE_VER_MINOR} << 16) + \
     (${FW_RELEASE_VER_REVISION} << 8)" OUTPUT_FORMAT HEXADECIMAL )

add_custom_command(
    OUTPUT asset-config-data.img
    COMMAND
        ${DEVICE_ARTIFACTS_TOOLS_DIR}/make-asset-config-data.py
          -o asset-config-data.img
	      --fw_release_rev  ${FW_RELEASE_VERSION}
          --scp_size        80
          --l2_size         16
          --l3_size         32
          --version         2
)

add_custom_target(asset-config-data ALL
    DEPENDS asset-config-data.img
)

install(
	FILES ${CMAKE_CURRENT_BINARY_DIR}/asset-config-data.img
    DESTINATION ${DEVICE_ARTIFACTS_LIB_DIR}/asset-config-data
    COMPONENT asset-config-data
)
