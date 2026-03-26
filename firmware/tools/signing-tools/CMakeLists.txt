cmake_minimum_required (VERSION 3.28)

project(EsperantoSignELF VERSION 0.1.0 LANGUAGES C)

include(GNUInstallDirs)

set(CMAKE_BUILD_TYPE Debug)
set(BIN_INSTALL_DIR bin CACHE STRING "Bin install dir")
set(INCLUDE_INSTALL_DIR include/ CACHE STRING "Include install dir")
set(LIB_INSTALL_DIR  lib/  CACHE STRING "Library install dir")


file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tools")

find_package(cryptoApi REQUIRED)

include_directories("${PROJECT_BINARY_DIR}")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g \
-Wall -Wextra -Werror -Wnull-dereference \
-Wduplicated-branches -Wduplicated-cond -Wshadow -Wpointer-arith -Wundef \
-Wbad-function-cast -Wcast-qual -Wcast-align -Wconversion -Wlogical-op \
-Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/tools)

add_executable(esperanto_sign_elf esperanto_sign_elf.c esperanto_load_elf.c )
target_link_libraries(esperanto_sign_elf PRIVATE cryptoApi::cryptoApi ${CMAKE_DL_LIBS})
install(TARGETS esperanto_sign_elf EXPORT EtCodeSigningToolsTargets)

add_executable(esperanto_sign_raw esperanto_sign_raw.c )
target_link_libraries(esperanto_sign_raw PRIVATE cryptoApi::cryptoApi ${CMAKE_DL_LIBS})
install(TARGETS esperanto_sign_raw EXPORT EtCodeSigningToolsTargets)
