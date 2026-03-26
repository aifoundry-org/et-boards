cmake_minimum_required(VERSION 3.8)
project(test_package C)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(cryptoApi REQUIRED)

add_executable(${PROJECT_NAME} test_package.c)
target_compile_features(${PROJECT_NAME} PRIVATE c_std_11)
target_link_libraries(${PROJECT_NAME} PRIVATE cryptoApi::cryptoApi)
