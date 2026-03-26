# configure a header file to pass some of the CMake settings
# to the source code
configure_file (
    ${CMAKE_CURRENT_SOURCE_DIR}/crypto_api_config.h.in
    ${PROJECT_BINARY_DIR}/crypto_api_config.h
)

target_sources(cryptoApi PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/crypto_api.c
    ${CMAKE_CURRENT_SOURCE_DIR}/crypto_api_load_file.c
    ${CMAKE_CURRENT_SOURCE_DIR}/crypto_api_load_json.c
    ${CMAKE_CURRENT_SOURCE_DIR}/crypto_api_parse_json.c
    ${CMAKE_CURRENT_SOURCE_DIR}/crypto_api_der_decoder.c
    ${CMAKE_CURRENT_SOURCE_DIR}/crypto_api_view_data.c
    ${PROJECT_BINARY_DIR}/crypto_api_config.h
)