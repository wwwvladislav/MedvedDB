cmake_minimum_required(VERSION 3.2)
project(inih)

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c) 

use_c99()

add_library(inih STATIC ${SOURCES})
