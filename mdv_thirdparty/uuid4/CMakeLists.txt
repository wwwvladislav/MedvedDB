cmake_minimum_required(VERSION 3.2)
project(uuid4)

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

use_c11()

add_library(uuid4 STATIC ${SOURCES})

add_library(mdv::uuid4 ALIAS uuid4)

target_include_directories(uuid4 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
