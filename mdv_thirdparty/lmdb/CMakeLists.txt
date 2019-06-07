cmake_minimum_required(VERSION 3.2)
project(lmdb)

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c) 

use_c11()

add_library(lmdb STATIC ${SOURCES})
