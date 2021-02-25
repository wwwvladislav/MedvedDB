cmake_minimum_required(VERSION 3.2)
project(rpmalloc)

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

use_c11()

add_library(rpmalloc STATIC ${SOURCES})

add_library(mdv::rpmalloc ALIAS rpmalloc)

target_include_directories(rpmalloc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
