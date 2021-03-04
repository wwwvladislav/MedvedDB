cmake_minimum_required(VERSION 3.4)
project(mdv_tests)

file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

use_c11()

add_executable(mdv_tests ${SOURCES})

target_compile_options(mdv_tests PRIVATE -Wall -Wextra)

target_include_directories(mdv_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(mdv_tests PRIVATE mdv::platform mdv::core mdv::types mdv::crypto mdv::storage)
