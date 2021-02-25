cmake_minimum_required(VERSION 3.2)
project(mdv_types)

file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

use_c11()

add_library(mdv_types STATIC ${SOURCES})

add_library(mdv::types ALIAS mdv_types)

target_include_directories(mdv_types PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(mdv_types PUBLIC mdv::zf_log mdv::platform)
