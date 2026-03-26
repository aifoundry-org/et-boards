# WHEN USING CONAN THIS SHOULD NOT BE USED
# CMake find_package() Module for json-c library
#
# Example usage:
#
# find_package(json-c)
#
# If successful the following variables will be defined
# - JSON_C_FOUND - System has lz4
# If successful the following targets will be defined
# - json-c
# - json-c::json-c

include(FindPackageHandleStandardArgs)

find_path(JSON_C_INCLUDE_DIR json-c/json.h)
find_library(JSON_C_LIBRARY NAMES json-c.a json-c)

find_package_handle_standard_args(json-c DEFAULT_MSG JSON_C_INCLUDE_DIR JSON_C_LIBRARY)

if (json-c_FOUND)
    add_library(json-c UNKNOWN IMPORTED GLOBAL)
    add_library(json-c::json-c ALIAS json-c)
    target_include_directories(json-c INTERFACE ${JSON_C_INCLUDE_DIR})
    set_target_properties(json-c PROPERTIES IMPORTED_LOCATION ${JSON_C_INCLUDE_DIR})

    mark_as_advanced(JSON_C_INCLUDE_DIR JSON_C_LIBRARY)
endif()