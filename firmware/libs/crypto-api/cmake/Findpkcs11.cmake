# WHEN USING CONAN THIS SHOULD NOT BE USED

configure_file(
    ${PROJECT_SOURCE_DIR}/external/pkcs11.CMakeLists.txt
    ${PROJECT_BINARY_DIR}/pkcs11/CMakeLists.txt
)

# Clone the pkcs11 package
execute_process(COMMAND ${CMAKE_COMMAND} -H. -B. -G "${CMAKE_GENERATOR}" -DGIT_CONFIG_OPTIONS=${GIT_CONFIG_OPTIONS}
    RESULT_VARIABLE result
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/pkcs11)
if (result)
    message(FATAL_ERROR "CMake step for pkcs11 failed: ${result}")
endif()

execute_process(COMMAND ${CMAKE_COMMAND} --build . --config Release
    RESULT_VARIABLE result
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/pkcs11)
if (result)
    message(FATAL_ERROR "Build step for pkcs failed: ${result}")
endif()

add_library(pkcs11 INTERFACE)
add_library(pkcs11::pkcs11 ALIAS pkcs11)
target_include_directories(pkcs11
    INTERFACE
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/pkcs11/src/published/2-40-errata-1>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/esperanto/pkcs11>
)

install(TARGETS pkcs11
    EXPORT cryptoApiTargets
    LIBRARY       DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/esperanto/pkcs11
    COMPONENT pkcs11
)
install(DIRECTORY ${PROJECT_BINARY_DIR}/pkcs11/src/published/2-40-errata-1/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/esperanto/pkcs11
)

if (CRYPTO_API_DEPRECATED)
    install(TARGETS pkcs11
          EXPORT  EsperantoCryptoAPITargets
          LIBRARY DESTINATION lib
          PUBLIC_HEADER DESTINATION include
          COMPONENT pkcs11
    )
endif ()