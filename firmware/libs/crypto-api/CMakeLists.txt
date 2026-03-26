cmake_minimum_required (VERSION 3.17)
project(cryptoApi VERSION 1.5.0 LANGUAGES C)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_CURRENT_SOURCE_DIR})

message(STATUS "CMAKE_MODULE_PATH: ${CMAKE_MODULE_PATH}")
message(STATUS "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")

option(CRYPTO_API_DEPRECATED "Enable deprecated functionality" ON)
option(CRYPTO_API_WARNINGS_AS_ERRORS "Enable warnings as errors" OFF)

if (CRYPTO_API_DEPRECATED)
    set(BIN_INSTALL_DIR bin CACHE STRING "Bin install dir")
    set(INCLUDE_INSTALL_DIR include/ CACHE STRING "Include install dir")
    set(LIB_INSTALL_DIR  lib/  CACHE STRING "Library install dir")
    set(SYSCONFIG_INSTALL_DIR etc/et-fw/ CACHE STRING "Config install dir")
endif()

include(GNUInstallDirs)
include(CompilerWarnings)

find_package(signedImageFormat REQUIRED)
find_package(json-c REQUIRED)
find_package(OpenSSL REQUIRED COMPONENTS Crypto SSL)
find_package(pkcs11 REQUIRED)


add_library(cryptoApi STATIC)
add_library(cryptoApi::cryptoApi ALIAS cryptoApi)
target_set_project_warnings(cryptoApi)
target_include_directories(cryptoApi
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/esperanto/crypto-api>
)
target_link_libraries(cryptoApi
    PUBLIC signedImageFormat::signedImageFormat json-c::json-c pkcs11::pkcs11
    PRIVATE OpenSSL::Crypto OpenSSL::SSL
)

set_target_properties(cryptoApi PROPERTIES PUBLIC_HEADER "include/crypto_api.h;include/crypto_api_der_decoder.h;include/crypto_api_linked_list.h;include/crypto_api_load_file.h;include/crypto_api_load_json.h;include/crypto_api_parse_json.h;include/crypto_api_pkcs11.h;include/crypto_api_view_data.h")

add_subdirectory(src)

install(TARGETS cryptoApi
    EXPORT cryptoApiTargets
    RUNTIME       DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY       DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE       DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/esperanto/crypto-api
)

install(EXPORT cryptoApiTargets
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cryptoApi
    NAMESPACE cryptoApi::
)

include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cryptoApiConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/cryptoApiConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cryptoApi
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/cryptoApiConfigVersion.cmake
    VERSION ${VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/cryptoApiConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/cryptoApiConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cryptoApi
)


if (CRYPTO_API_DEPRECATED)
    install(TARGETS cryptoApi
          EXPORT  EsperantoCryptoAPITargets
          LIBRARY DESTINATION lib
          PUBLIC_HEADER DESTINATION include
          COMPONENT crypto_api
    )
    install(DIRECTORY include/ DESTINATION ${INCLUDE_INSTALL_DIR})

    ### Add package information
    include(CMakePackageConfigHelpers)

    # Export the package for use from the build-tree
    # (this registers the build-tree with a global CMake-registry)
    export(PACKAGE EsperantoCryptoAPI)

    # Create the *-config.cmake and *-config-version.cmake files
    configure_package_config_file(EsperantoCryptoAPIConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/EsperantoCryptoAPIConfig.cmake
        INSTALL_DESTINATION ${LIB_INSTALL_DIR}/EsperantoCryptoAPI/cmake
        PATH_VARS INCLUDE_INSTALL_DIR SYSCONFIG_INSTALL_DIR LIB_INSTALL_DIR BIN_INSTALL_DIR
    )

    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/EsperantoCryptoAPIConfigVersion.cmake
        VERSION ${VERSION}
        COMPATIBILITY SameMajorVersion
    )

    # Install the *Config.cmake and *ConfigVersion.cmake
    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/EsperantoCryptoAPIConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/EsperantoCryptoAPIConfigVersion.cmake"
        DESTINATION ${LIB_INSTALL_DIR}/cmake/EsperantoCryptoAPI
        COMPONENT crypto_api
    )

    # Install the export set for use with the install-tree
    install(EXPORT EsperantoCryptoAPITargets
        DESTINATION "${LIB_INSTALL_DIR}/cmake/EsperantoCryptoAPI"
        COMPONENT crypto_api
    )

endif ()
