cmake_minimum_required(VERSION 3.2)
project(lz4)

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

use_c11()

add_library(lz4 STATIC ${SOURCES})

add_library(mdv::lz4 ALIAS lz4)

target_include_directories(lz4 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
