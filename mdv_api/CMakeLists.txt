cmake_minimum_required(VERSION 3.2)
project(mdv_api)

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

use_c11()

add_library(mdv_api STATIC ${SOURCES})

add_library(mdv::api ALIAS mdv_api)

target_include_directories(mdv_api PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(mdv_api PUBLIC mdv::binn mdv::platform mdv::net mdv::types)
