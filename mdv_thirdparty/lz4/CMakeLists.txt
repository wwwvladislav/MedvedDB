cmake_minimum_required(VERSION 3.2)
project(lz4)

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c) 

use_c11()

add_library(lz4 STATIC ${SOURCES})
