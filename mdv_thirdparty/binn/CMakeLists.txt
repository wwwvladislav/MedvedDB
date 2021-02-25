cmake_minimum_required(VERSION 3.2)
project(binn)

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

use_c11()

add_library(binn STATIC ${SOURCES})

add_library(mdv::binn ALIAS binn)

target_include_directories(binn PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
