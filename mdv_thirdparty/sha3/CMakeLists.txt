cmake_minimum_required(VERSION 3.2)
project(sha3)

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

use_c11()

add_library(sha3 STATIC ${SOURCES})

add_library(mdv::sha3 ALIAS sha3)

target_include_directories(sha3 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
